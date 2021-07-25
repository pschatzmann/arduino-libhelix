#pragma once
#include "Arduino.h"

// User Settings: Activate/Deactivate logging
#define LOGGING_ACTIVE false
#define SYNCH_WORD_LEN 4


#ifndef ARDUINO
void yield() {}
#endif

#if LOGGING_ACTIVE == true
static char log_buffer[512];
enum LogLevel {Debug, Info, Warning, Error};
LogLevel minLogLevel = Warning;
// We print the log based on the log level
#define LOG(level,...) { if(level>=minLogLevel) { int l = snprintf(log_buffer,512, __VA_ARGS__);  Serial.write(log_buffer,l); Serial.println(); } }
#else
// Remove all log statments from the code
#define LOG(Debug, ...) 
#endif

/**
 * @brief Range with a start and an end
 * 
 */
struct Range {
    int start;
    int end;
};

/**
 * @brief Common Simple Arduino API 
 * 
 */
class CommonHelix   {
    public:

        ~CommonHelix(){
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

        /**
         * @brief Starts the processing
         * 
         */
        virtual void begin(){
            if (active){
                end();
            }

            pwm_buffer = new short[maxPWMSize()];
            frame_buffer = new uint8_t[maxFrameSize()];
            if (pwm_buffer==nullptr && frame_buffer==nullptr){
                LOG(Error, "Not enough memory for buffers");
                return;
            }
            first = true;
            active = true;
            ignoreHeader = true;
            buffer_size = 0;
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
            LOG(Debug, "write %zu", in_size);
            size_t result_size=0;            
            if (active){
                close_on_no_data_counter = 0;
                int start = 0;
                // we can not write more then the AAC_MAX_FRAME_SIZE 
                size_t write_len = min(in_size, maxFrameSize()-buffer_size);
                while(result_size<in_size){
                    if (write_len>=0){
                        // we have some space left in the buffer
                        uint8_t* ptr8 = (uint8_t* )in_ptr;
                        int written_len = writeFrames(ptr8+start, write_len);
                        result_size += written_len;
                    } 
                    start+=result_size;
                    write_len = min(in_size - start, maxFrameSize()-buffer_size);
                    yield();
                }
            }

            // automatically close when we received no data more then 2 times
            if (in_size==0){
                close_on_no_data_counter++;
                if (close_on_no_data_counter==2){
                    end();
                }
            }
            return result_size;
        }

        /// returns true if active
        operator bool() {
            return active;
        }       

    protected:
        bool first = true;
        bool ignoreHeader = true; 
        bool active = false;
        Print *out = nullptr;
        Stream *in = nullptr;
        uint8_t close_on_no_data_counter = 0;
        uint32_t buffer_size; // actually filled sized
        uint8_t *frame_buffer = nullptr;
        short *pwm_buffer = nullptr;
        size_t max_frame_size = 0;
        size_t max_pwm_size = 0;
   
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

        /// Finds the synchronization word in the frame buffer (starting from the indicated offset)
        virtual int findSynchWord(int offset=0) = 0;   

        /// Decodes a frame
        virtual int decode(Range r) = 0;   

        /// we add the data to the buffer until it is full
        size_t appendToBuffer(const void *in_ptr, size_t in_size){
            int buffer_size_old = buffer_size;
            uint32_t processed_size = min(maxFrameSize() - buffer_size, in_size);
            memmove(frame_buffer+buffer_size, in_ptr, processed_size);
            buffer_size += processed_size;
            LOG(Debug, "appendToBuffer %u -> %u  (%u)", buffer_size_old, buffer_size, processed_size);
            return processed_size;
        }

        /// appends the data to the frame buffer and decodes 
        size_t writeFrames(const void *in_ptr, size_t in_size){
            LOG(Debug, "writeFrames %zu", in_size);
            size_t result = 0;
            if (in_size>0) {
                // in the beginning we ingnore all data until we found the first synch word
                result = appendToBuffer(in_ptr, in_size);
                Range r = frameRange(); //synchronizeFrame();
                while(r.start>=0 && r.end>=0){
                    int decode_result = decode(r);
                    if (decode_result==-1){
                        // ERR_MP3_INDATA_UNDERFLOW -> get more data
                        r.start = 0; 
                        r.end = -1;
                    } else if (decode_result<0){
                        // when we have an error we try to resynchronize
                        r = synchronizeFrame();
                    } else {
                        // we can start from 0 - but we need to determine the end synch
                        r.start = 0; 
                        r.end = findSynchWord(SYNCH_WORD_LEN);
                    }
                    yield();
                } 
            } else{
                // we can not write any more data -> find the next synch word 
                synchronizeFrame();
            }
            return result;
        }

        /// returns true if we have a valid start and end synch word.
        Range synchronizeFrame() {
            LOG(Debug, "synchronizeFrame");
            Range range = frameRange();
            if (range.start<0){
                // there is no Synch in the buffer at all -> we can ignore all data
                range.end = -1;
                LOG(Debug, "-> no synronization info")
                if (buffer_size==maxFrameSize()) {
                    buffer_size = 0;
                    LOG(Debug, "-> buffer cleared");
                }
            } else if (range.start>0) {
                // make sure that buffer starts with a synch word
                LOG(Debug, "-> moving to new start %d",range.start);
                buffer_size -= range.start;
                memmove(frame_buffer, frame_buffer + range.start, buffer_size);
                range.end -= range.start;
                range.start = 0;
                LOG(Debug, "-> we are at beginning of synch word");
            } else if (range.start==0) {
                LOG(Debug, "-> we are at beginning of synch word");
                if (range.end<0 && buffer_size == maxFrameSize()){
                    buffer_size = 0;
                    LOG(Debug, "-> buffer cleared");
                }
            }
            return range;
        }

        /// determines the next start and end synch word in the buffer
        Range frameRange(){
            Range result;
            result.start = findSynchWord(0);
            // when we have subsequent synch words we take the last
            int pos = 0;
            for (int j=result.start;j<buffer_size && pos==0;j++){
                pos = findSynchWord(j);
                if (pos==0){
                    result.start = j;
                } 
            }
            
            result.end = findSynchWord(result.start+SYNCH_WORD_LEN);
            if (result.end>0){
                // use end position counted from start
                result.end += result.start + SYNCH_WORD_LEN;
            }
            LOG(Debug, "frameRange -> %d - %d with buffer_size: %d", result.start, result.end, buffer_size);
            return result;
        }

};
