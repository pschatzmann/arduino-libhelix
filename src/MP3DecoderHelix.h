#pragma once

#include "Arduino.h"
#include "CommonHelix.h"
#include "libhelix-mp3/mp3dec.h"
#include "libhelix-mp3/mp3common.h"

#define MP3_MAX_OUTPUT_SIZE 1024 * 2
#define MP3_MAX_FRAME_SIZE 1600 

typedef void (*MP3InfoCallback)(MP3FrameInfo &info);
typedef void (*MP3DataCallback)(MP3FrameInfo &info,short *pwm_buffer, size_t len);

enum MP3Type {MP3Normal=0, MP3SelfContaind=1};


/**
 * @brief A simple Arduino API for the libhelix MP3 decoder. The data is provided with the help of write() calls.
 * The decoded result is available either via a callback method or via an output stream.
 */
class MP3DecoderHelix : public CommonHelix {
    public:
        MP3DecoderHelix(Print &output, MP3Type mp3Type=MP3Normal, MP3InfoCallback infoCallback=nullptr){
            this->out = &output;
            this->infoCallback = infoCallback;
            this->mp3_type = mp3Type;
        }

        MP3DecoderHelix(MP3DataCallback dataCallback, MP3Type mp3Type=MP3Normal){
            this->pwmCallback = dataCallback;
            this->mp3_type = mp3Type;
        }


         /// Starts the processing
        void begin(){
            LOG(Debug, "begin");
            CommonHelix::begin();
            decoder = MP3InitDecoder();
            if (decoder==nullptr){
                LOG(Error, "MP3InitDecoder has failed");
                active = false;
                return;
            }
        }

        /// Releases the reserved memory
        void end(){
            LOG(Debug, "end");
            MP3FreeDecoder(decoder);
            CommonHelix::end();
        }

        /// Provides the last available MP3FrameInfo
        MP3FrameInfo audioInfo(){
            return mp3FrameInfo;
        }


    protected:
        HMP3Decoder decoder = nullptr;
        MP3DataCallback pwmCallback = nullptr;
        MP3InfoCallback infoCallback = nullptr;
        MP3Type mp3_type;
        MP3FrameInfo mp3FrameInfo;

        /// determines the frame buffer size that will be allocated
        size_t maxFrameSize(){
            return max_frame_size == 0 ? MP3_MAX_FRAME_SIZE : max_frame_size;
        }

        /// Determines the pwm buffer size that will be allocated
        size_t maxPWMSize() {
            return max_pwm_size == 0 ? MP3_MAX_OUTPUT_SIZE : max_pwm_size;
        }

        /// Finds the synch word in the available buffer data starting from the indicated offset
        int findSynchWord(int offset=0) {
            int result = MP3FindSyncWord(frame_buffer+offset, buffer_size);
            return result < 0 ? result : result + offset;
        }

        /// decods the data 
        void decode(Range r) {
            LOG(Debug, "decode %d", r.end);
            int len = r.end;
            int bytesLeft =  r.end; //r.end; //r.end; // buffer_size

            int result = MP3Decode(decoder, &frame_buffer + r.start, &bytesLeft, pwm_buffer, mp3_type);
            checkMemory();

            
            if (result==0){
                int decoded = len - bytesLeft;
                LOG(Debug, "-> bytesLeft %d -> %d  = %d ", buffer_size, bytesLeft, decoded);
                LOG(Debug, "-> End of frame (%d) vs end of decoding (%d)", r.end, decoded)

                // return the decoded result
                MP3FrameInfo info;
                MP3GetLastFrameInfo(decoder, &info);
                provideResult(info);

                // remove processed data from buffer 
                buffer_size -= decoded;
                memmove(frame_buffer, frame_buffer+r.start+decoded, buffer_size);
                checkMemory();
                LOG(Debug, " -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
            } else {
                // decoding error
                LOG(Debug, " -> decode error: %d - removing frame!", result);
                // We advance to the next synch world
                if (r.end>0){
                    buffer_size -= r.end;
                    memmove(frame_buffer, frame_buffer+r.end, buffer_size);
                    checkMemory();
                }
            }
        }

        // return the resulting PWM data
        void provideResult(MP3FrameInfo &info){
            LOG(Debug, "=> provideResult: %d", info.outputSamps);

            // provide result
            if(pwmCallback!=nullptr){
                // output via callback
                pwmCallback(info, pwm_buffer, info.outputSamps);
            } else {
                // output to stream
                if (info.samprate!=mp3FrameInfo.samprate  && infoCallback!=nullptr){
                    infoCallback(mp3FrameInfo);
                }
                out->write((uint8_t*)pwm_buffer, info.outputSamps);
            }
            mp3FrameInfo = info;
            checkMemory();
        }

        /// checks the consistency of the memory
        virtual void checkMemory(){
            //assert(frame_buffer[maxFrameSize()+1]==0);
            //assert(pwm_buffer[maxPWMSize()+1]==-1);
        }


};