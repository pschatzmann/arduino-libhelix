#pragma once

#ifdef ARDUINO
#include "Arduino.h"
#else
// remove yield statment if used outside of arduino
#define yield()
//#define delay(ms)
#include <stdint.h>
#endif

// Not all processors support assert
#ifndef assert
#ifdef NDEBUG 
#  define assert(condition) ((void)0)
#else
#  define assert(condition) /*implementation defined*/
#endif
#endif

#include "helix_log.h"
#include <string.h>

#define SYNCH_WORD_LEN 4


namespace libhelix {

/**
 * @brief Range with a start and an end
 * @author Phil Schatzmann
 * @copyright GPLv3
 * 
 */
struct Range {
    int start;
    int end;
    bool isValid(uint32_t max) {
        return start>=0 && end>start && (end - start)<=max;
    }
};

/**
 * @brief Common Simple Arduino API 
 * @author Phil Schatzmann
 * @copyright GPLv3
 * 
 */
class CommonHelix   {
    public:

        virtual ~CommonHelix(){
            if (active){
                end();
            }
            if (pwm_buffer!=nullptr){
                delete[] pwm_buffer;
            }
            if (frame_buffer!=nullptr){
                delete[] frame_buffer;
            }
        }

#ifdef ARDUINO
        void setOutput(Print &output){
            this->out = &output;
        }
#endif

        /**
         * @brief Starts the processing
         * 
         */
        virtual void begin(){
            buffer_size = 0;
            frame_counter = 0;

            if (active){
                end();
            }

            allocateDecoder();

            if (frame_buffer == nullptr) {
                LOG_HELIX(Info,"allocating frame_buffer with %zu bytes", maxFrameSize());
                frame_buffer = new uint8_t[maxFrameSize()];
            }
            if (pwm_buffer == nullptr) {
                LOG_HELIX(Info,"allocating pwm_buffer with %zu bytes", maxPWMSize());
                pwm_buffer = new short[maxPWMSize()];
            }
            if (pwm_buffer==nullptr || frame_buffer==nullptr){
                LOG_HELIX(Error, "Not enough memory for buffers");
                active = false;
                return;
            }
            memset(frame_buffer,0, maxFrameSize());
            memset(pwm_buffer,0, maxPWMSize());
            active = true;
        }

        /// Releases the reserved memory
        virtual void end(){
            active = false;
        }

        /**
         * @brief decodes the next segments from the intput. 
         * The data can be provided in one short or in small incremental pieces.
         * It is suggested to be called in the Arduino Loop. If the provided data does
         * not fit into the buffer it is split up into small pieces that fit
         */
        
        virtual size_t write(const void *in_ptr, size_t in_size) {
            LOG_HELIX(Info, "write %zu", in_size);
            time_last_write = millis();
            size_t start = 0;
            if (active){
                uint8_t* ptr8 = (uint8_t* )in_ptr;
                // we can not write more then the AAC_MAX_FRAME_SIZE 
                size_t write_len = min(in_size, static_cast<size_t>(maxFrameSize()-buffer_size));
                while(start<in_size){
                    // we have some space left in the buffer
                    int written_len = writeFrame(ptr8+start, write_len);
                    start += written_len;
                    LOG_HELIX(Info,"-> Written %zu of %zu - Counter %zu", start, in_size, frame_counter);
                    write_len = min(in_size - start, static_cast<size_t>(maxFrameSize()-buffer_size));
                    // add delay - e.g. needed by esp32 and esp8266
                    if (delay_ms>0){
                        delay(delay_ms);
                    }
                }
            } else {
                LOG_HELIX(Warning, "CommonHelix not active");
            }

            return start;
        }

        /// returns true if active
        operator bool() {
            return active;
        }   

        /// Defines the delay that is added at each segment
        virtual void setDelay(int delayMs){
            delay_ms = delayMs;
        }    

        /// Provides the timestamp in ms of last write
        uint64_t timeOfLastWrite() {
            return time_last_write;
        }

        /// Provides the timestamp in ms of last decoded result
        uint64_t timeOfLastResult() {
            return time_last_result;
        }

        /// Decode open packets
        void flush() {
            Range r = synchronizeFrame();
            // Decode if we have a valid start and end synch word
            while(r.isValid(maxFrameSize())){
                decode(r);
                r = synchronizeFrame();
            } 
        }

        /// Provides the maximum frame size - this is allocated on the heap and you can reduce the heap size my minimizing this value
        virtual size_t maxFrameSize() = 0;

        /// Define your optimized maximum frame size
        void setMaxFrameSize(size_t len){
            max_frame_size = len;
        }

        /// Provides the maximum pwm buffer size - this is allocated on the heap and you can reduce the heap size my minimizing this value
        virtual size_t maxPWMSize() = 0 ;

        /// Define your optimized maximum pwm buffer size
        void setMaxPWMSize(size_t len) {
            max_pwm_size = len;
        }

    protected:
        bool active = false;
        uint32_t buffer_size = 0; // actually filled sized
        uint8_t *frame_buffer = nullptr;
        short *pwm_buffer = nullptr;
        size_t max_frame_size = 0;
        size_t max_pwm_size = 0;
        size_t frame_counter = 0;
        int delay_ms = -1;
        uint64_t time_last_write=0;
        uint64_t time_last_result=0;

#ifdef ARDUINO
        Print *out = nullptr;
#endif
   
        virtual void allocateDecoder() = 0;


        /// Finds the synchronization word in the frame buffer (starting from the indicated offset)
        virtual int findSynchWord(int offset=0) = 0;   

        /// Decodes a frame
        virtual void decode(Range r) = 0;   

        /// we add the data to the buffer until it is full
        size_t appendToBuffer(const void *in_ptr, int in_size){
            LOG_HELIX(Info, "appendToBuffer: %d (at %p)", in_size, frame_buffer);
            int buffer_size_old = buffer_size;
            int process_size = min((int)(maxFrameSize() - buffer_size), in_size);
            memmove(frame_buffer+buffer_size, in_ptr, process_size); 
            buffer_size += process_size;
            if (buffer_size>maxFrameSize()){
                LOG_HELIX(Error, "Increase MAX_FRAME_SIZE > %u", (unsigned int)buffer_size);
            }
            assert(buffer_size<=maxFrameSize());

            LOG_HELIX(Debug, "appendToBuffer %d + %d  -> %u", buffer_size_old,  process_size,  (unsigned int)buffer_size );
            return process_size;
        }

        /// appends the data to the frame buffer and decodes 
        size_t writeFrame(const void *in_ptr, size_t in_size){
            LOG_HELIX(Info, "writeFrame %zu", in_size);
            size_t result = 0;
            // in the beginning we ingnore all data until we found the first synch word
            result = appendToBuffer(in_ptr, in_size);
            Range r = synchronizeFrame();
            // Decode if we have a valid start and end synch word
            if(r.isValid(maxFrameSize())){
                decode(r);
            } else {
                int size =  r.end-r.start;
                if (size>0){
                    LOG_HELIX(Warning, " -> invalid frame size: %d / max: %d", (int) r.end-r.start, (int) maxFrameSize());
                } else {
                    LOG_HELIX(Info, " -> invalid frame size: %d / max: %d", (int) r.end-r.start, (int) maxFrameSize());
                }
            }
            frame_counter++;
            return result;
        }

        /// returns valid start and end synch word.
        Range synchronizeFrame() {
            LOG_HELIX(Debug, "synchronizeFrame");
            Range range = frameRange();
            if (range.start<0){
                // there is no Synch in the buffer at all -> we can ignore all data
                range.end = -1;
                LOG_HELIX(Debug, "-> no synch")
                if (buffer_size==maxFrameSize()) {
                    buffer_size = 0;
                    LOG_HELIX(Debug, "-> buffer cleared");
                }
            } else if (range.start>0) {
                // make sure that buffer starts with a synch word
                LOG_HELIX(Debug, "-> moving to new start %d",range.start);
                buffer_size -= range.start;
                assert(buffer_size<=maxFrameSize());

                memmove(frame_buffer, frame_buffer + range.start, buffer_size);
                range.end -= range.start;
                range.start = 0;
                LOG_HELIX(Debug, "-> we are at beginning of synch word");
            } else if (range.start==0) {
                LOG_HELIX(Debug, "-> we are at beginning of synch word");
                if (range.end<0 && buffer_size == maxFrameSize()){
                    buffer_size = 0;
                    LOG_HELIX(Debug, "-> buffer cleared");
                }
            }
            return range;
        }

        /// determines the next start and end synch word in the buffer
        Range frameRange(){
            Range result;
            result.start = findSynchWord(0);
            result.end = findSynchWord(result.start+SYNCH_WORD_LEN);
            LOG_HELIX(Debug, "-> frameRange -> %d - %d", result.start, result.end);
            return result;
        }

        void advanceFrameBuffer(int offset){
            buffer_size -= offset;
            assert(buffer_size<=maxFrameSize());
            memmove(frame_buffer, frame_buffer+offset, buffer_size);
        }


};

}
