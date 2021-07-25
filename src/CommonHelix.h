#pragma once
#include "Arduino.h"

// Activate/Deactivate logging
#define LOGGING_ACTIVE true
#define SYNCH_WORD_LEN 4
#define PROFILE

// Do not change the following logic

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

#if LOGGING_ACTIVE == true
static char log_buffer[512];
#define LOG(...) { int l = snprintf(log_buffer,512, __VA_ARGS__);  Serial.write(log_buffer,l); Serial.println(); }
#else
#define LOG(...) 
#endif

#ifndef ARDUINO
void yield() {}
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

        /**
         * @brief Starts the processing
         * 
         */
        virtual void begin(){
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
            LOG("write %zu", in_size);
            size_t result_size=0;            
            if (active){
                close_on_no_data_counter = 0;
                int start = 0;
                // we can not write more then the AAC_MAX_FRAME_SIZE 
                size_t write_len = MIN(in_size, maxFrameSize()-buffer_size);
                while(result_size<in_size){
                    if (write_len>=0){
                        // we have some space left in the buffer
                        uint8_t* ptr8 = (uint8_t* )in_ptr;
                        int written_len = writeFrames(ptr8+start, write_len);
                        result_size += written_len;
                    } 
                    start+=result_size;
                    write_len = MIN(in_size - start, maxFrameSize()-buffer_size);
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
        unsigned char *frame_buffer = nullptr;
   
        virtual size_t maxFrameSize() = 0 ;
        virtual int findSynchWord(int offset=0) = 0;   
        virtual int decode(Range r) = 0;   


        /// we add the data to the buffer until it is full
        size_t appendToBuffer(const void *in_ptr, size_t in_size){
            int buffer_size_old = buffer_size;
            uint32_t processed_size = MIN(maxFrameSize() - buffer_size, in_size);
            memmove(frame_buffer+buffer_size, in_ptr, processed_size);
            buffer_size += processed_size;
            LOG("appendToBuffer %u -> %u  (%u)", buffer_size_old, buffer_size, processed_size);
            return processed_size;
        }

        /// appends the data to the frame buffer and decodes 
        size_t writeFrames(const void *in_ptr, size_t in_size){
            LOG("writeFrames %zu", in_size);
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

        // returns true if we have a valid start and end synch word.
        Range synchronizeFrame() {
            LOG("synchronizeFrame");
            Range range = frameRange();
            if (range.start<0){
                // there is no Synch in the buffer at all -> we can ignore all data
                range.end = -1;
                LOG("-> no synronization info")
                if (buffer_size==maxFrameSize()) {
                    buffer_size = 0;
                    LOG("-> buffer cleared");
                }
            } else if (range.start>0) {
                // make sure that buffer starts with a synch word
                LOG("-> moving to new start %d",range.start);
                buffer_size -= range.start;
                memmove(frame_buffer, frame_buffer + range.start, buffer_size);
                range.end -= range.start;
                range.start = 0;
                LOG("-> we are at beginning of synch word");
            } else if (range.start==0) {
                LOG("-> we are at beginning of synch word");
            }
            return range;
        }

        // determines the next start and end synch word in the buffer
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
            LOG("frameRange -> %d - %d with buffer_size: %d", result.start, result.end, buffer_size);
            return result;
        }

};
