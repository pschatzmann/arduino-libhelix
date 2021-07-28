#pragma once

#include "Arduino.h"
#include "CommonHelix.h"
#include "libhelix-mp3/mp3dec.h"

#define MP3_MAX_OUTPUT_SIZE 2304 
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
            if (active){
                decoder = MP3InitDecoder();
                if (decoder==nullptr){
                    LOG(Error, "MP3InitDecoder has failed");
                    active = false;
                }
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
            return MP3FindSyncWord(frame_buffer+offset, buffer_size)+offset;
        }

        /// decods the data 
        int decode(Range r) {
            LOG(Debug, "decode %d", r.end);
            int bytesLeft = r.end; //r.end;
            int decoded = r.end;

            int result = MP3Decode(decoder, &frame_buffer + r.start, &bytesLeft, pwm_buffer, mp3_type);
            LOG(Debug, "bytesLeft %d -> %d  = %d ", r.end, bytesLeft, decoded);
            if (result==0){
                decoded = r.end - bytesLeft;
                LOG(Debug, "End of frame (%d) vs end of decoding (%d)", r.end, decoded)

                // return the decoded result
                MP3FrameInfo info;
                MP3GetLastFrameInfo(decoder, &info);
                provideResult(info);

                // remove processed data from buffer 
                buffer_size -= decoded;
                memmove(frame_buffer, frame_buffer+r.start+decoded, buffer_size);
                LOG(Debug, " -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
            } else {
                // decoding error
                if (result==-1){
                    LOG(Debug, " -> in data underflow - we need more data");
                } else if (result<-1){
                    // in the case of error we remove the frame
                    LOG(Debug, " -> decode error: %d - removing frame!", result);
                    // to prevent an endless loop when the decoded is not advancing
                    buffer_size -= r.end;
                    memmove(frame_buffer, frame_buffer+r.end, buffer_size);
                }
            }
            return decoded;
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
        }

};