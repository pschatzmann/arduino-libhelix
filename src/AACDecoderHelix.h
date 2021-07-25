#pragma once

#include "Arduino.h"
#include "CommonHelix.h"
#include "libhelix-aac/aacdec.h"

#define AAC_MAX_OUTPUT_SIZE 1024*2 //AAC_MAX_NSAMPS
#define AAC_MAX_FRAME_SIZE 1600 //AAC_MAINBUF_SIZE

typedef void (*AACInfoCallback)(AACFrameInfo &info);
typedef void (*AACDataCallback)(AACFrameInfo &info,short *pwm_buffer, size_t len);

/**
 * @brief A simple Arduino API for the libhelix AAC decoder. The data us provided with the help of write() calls.
 * The decoded result is available either via a callback method or via an output stream.
 */
class AACDecoderHelix : public CommonHelix {
    public:
        AACDecoderHelix(Print &output, AACInfoCallback infoCallback=nullptr){
            this->out = &output;
            this->infoCallback = infoCallback;
            this->frame_buffer = frame_buffer_impl;
        }

        AACDecoderHelix(AACDataCallback dataCallback){
            this->pwmCallback = dataCallback;
            this->frame_buffer = frame_buffer_impl;
        }

        ~AACDecoderHelix(){
            if (active){
                end();
            }
        }
         /// Starts the processing
        void begin(){
            LOG("begin");
            if (active){
                end();
            }
            decoder = AACInitDecoder();
            CommonHelix::begin();
        }

        /// Releases the reserved memory
        void end(){
            LOG("end");
            AACFreeDecoder(decoder);
            CommonHelix::end();
        }

        /// Provides the last available aacFrameInfo
        AACFrameInfo audioInfo(){
            return aacFrameInfo;
        }

    protected:
        HAACDecoder decoder;
        AACFrameInfo aacFrameInfo;
        AACDataCallback pwmCallback = nullptr;
        AACInfoCallback infoCallback = nullptr;
        unsigned char frame_buffer_impl[AAC_MAX_FRAME_SIZE];
        short pwm_buffer[AAC_MAX_OUTPUT_SIZE];

        size_t maxFrameSize(){
            return AAC_MAX_FRAME_SIZE;
        }

        int findSynchWord(int offset=0) {
            return AACFindSyncWord(frame_buffer+offset, buffer_size);
        }

        /// decods the data and removes the decoded frame from the buffer
        int decode(Range r) {
            LOG("decode %d",r.end);
            int bytesLeft = r.end;
            int decoded = r.end;

            int result = AACDecode(decoder, &frame_buffer, &bytesLeft, pwm_buffer);

            if (result==0){
                decoded -= bytesLeft;
                // get info from last frame
                AACFrameInfo info;
                AACGetLastFrameInfo(decoder, &info);
                provideResult(info);

                // remove processed data from buffer 
                buffer_size -= decoded;
                memmove(pwm_buffer, pwm_buffer+decoded, buffer_size);
                LOG(" -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
            } else {
                LOG(" -> decode error: %d", result);
            }

            return result;
        }

        // return the result PWM data
        void provideResult(AACFrameInfo &info){
            LOG("provideResult: %d samples",info.outputSamps);
            // provide result
            if(pwmCallback!=nullptr){
                // output via callback
                pwmCallback(info, pwm_buffer,info.outputSamps);
            } else {
                // output to stream
                if (info.sampRateOut!=aacFrameInfo.sampRateOut && infoCallback!=nullptr){
                    infoCallback(info);
                }
                out->write((uint8_t*)pwm_buffer, info.outputSamps);
            }
            aacFrameInfo = info;

        }

};