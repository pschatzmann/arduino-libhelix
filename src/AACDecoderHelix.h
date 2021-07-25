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
        }

        AACDecoderHelix(AACDataCallback dataCallback){
            this->pwmCallback = dataCallback;
        }

        ~AACDecoderHelix(){
            if (active){
                end();
            }
        }
         /// Starts the processing
        void begin(){
            LOG(Debug, "begin");
            CommonHelix::begin();
            decoder = AACInitDecoder();
        }

        /// Releases the reserved memory
        void end(){
            LOG(Debug, "end");
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

        size_t maxFrameSize(){
            return max_frame_size == 0 ? AAC_MAX_FRAME_SIZE : max_frame_size;

        }

        size_t maxPWMSize() {
            return max_pwm_size == 0 ? AAC_MAX_OUTPUT_SIZE : max_pwm_size;
        }

        int findSynchWord(int offset=0) {
            return AACFindSyncWord(frame_buffer+offset, buffer_size);
        }

        /// decods the data and removes the decoded frame from the buffer
        int decode(Range r) {
            LOG(Debug, "decode %d", r.end);
            int bytesLeft = buffer_size; //r.end;
            int decoded = r.end;

            int result = AACDecode(decoder, &frame_buffer, &bytesLeft, pwm_buffer);
            decoded = buffer_size - bytesLeft;
            LOG(Debug, "bytesLeft %d -> %d  = %d ", buffer_size, bytesLeft, decoded);
            if (result==0){
                LOG(Debug, "End of frame (%d) vs end of decoding (%d)", r.end, decoded)

                // return the decoded result
                AACFrameInfo info;
                AACGetLastFrameInfo(decoder, &info);
                provideResult(info);

                // remove processed data from buffer 
                buffer_size -= decoded;
                memmove(frame_buffer, frame_buffer+decoded, buffer_size);
                LOG(Debug, " -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
            } else {
                if (result==-1){
                    LOG(Debug, " -> in data underflow - we need more data");
                } else if (result<-1){
                    // in the case of error we remove the frame
                    LOG(Debug, " -> decode error: %d - removing frame!", result);
                    // to prevent an endless loop when the decoded is not advancing
                    if (decoded==0){
                        decoded = r.end;
                    }
                    buffer_size -= decoded;
                    memmove(frame_buffer, frame_buffer+decoded, buffer_size);
                }
            }
            return result;
        }

        // return the result PWM data
        void provideResult(AACFrameInfo &info){
            LOG(Debug, "provideResult: %d samples",info.outputSamps);
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