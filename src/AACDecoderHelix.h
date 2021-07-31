#pragma once

#include "Arduino.h"
#include "CommonHelix.h"
#include "libhelix-aac/aac_decoder.h"

#define AAC_MAX_OUTPUT_SIZE 1024 
#define AAC_MAX_FRAME_SIZE 256 

typedef void (*AACInfoCallback)(_AACFrameInfo_t &info);
typedef void (*AACDataCallback)(_AACFrameInfo_t &info,short *pwm_buffer, size_t len);

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

         /// Starts the processing
        void begin(){
            LOG(Debug, "begin");
            CommonHelix::begin();
            decoder = AACInitDecoder();
            if (decoder==nullptr){
                LOG(Error, "MP3InitDecoder has failed");
                active = false;
                return;
            }
        }

        /// Releases the reserved memory
        void end(){
            LOG(Debug, "end");
            AACFreeDecoder(decoder);
            CommonHelix::end();
        }

        /// Provides the last available _AACFrameInfo_t
        _AACFrameInfo_t audioInfo(){
            return _AACFrameInfo_t;
        }

    protected:
        AACDecoder decoder;
        _AACFrameInfo_t _AACFrameInfo_t;
        AACDataCallback pwmCallback = nullptr;
        AACInfoCallback infoCallback = nullptr;

        size_t maxFrameSize(){
            return max_frame_size == 0 ? AAC_MAX_FRAME_SIZE : max_frame_size;
        }

        size_t maxPWMSize() {
            return max_pwm_size == 0 ? AAC_MAX_OUTPUT_SIZE : max_pwm_size;
        }

        int findSynchWord(int offset=0) {
            return AACFindSyncWord(frame_buffer+offset, buffer_size)+offset;
        }

        /// decods the data and removes the decoded frame from the buffer
        void decode(Range r) {
            LOG(Debug, "decode %d", r.end);
            int len = r.end;
            int bytesLeft =  r.end; //r.end; //r.end; // buffer_size

            int result = AACDecode(decoder, &frame_buffer+r.start, &bytesLeft, pwm_buffer);
            decoded = buffer_size - bytesLeft;
            checkMemory();

            
            if (result==0){
                int decoded = len - bytesLeft;
                LOG(Debug, "-> bytesLeft %d -> %d  = %d ", buffer_size, bytesLeft, decoded);
                LOG(Debug, "-> End of frame (%d) vs end of decoding (%d)", r.end, decoded)

                // return the decoded result
                _AACFrameInfo_t info;
                AACGetLastFrameInfo(decoder, &info);
                provideResult(info);

                // remove processed data from buffer 
                buffer_size -= decoded;
                memmove(frame_buffer, frame_buffer+r.start+decoded, buffer_size);
                checkMemory();
                LOG(Debug, " -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
            } else {
                // in the case of error we remove the frame
                LOG(Debug, " -> decode error: %d - removing frame!", result);
                // to prevent an endless loop when the decoded is not advancing
                if (r.end>0){
                    buffer_size -= r.end;
                    memmove(frame_buffer, frame_buffer+r.end, buffer_size);
                    checkMemory();
                }
            }
        }

        // return the result PWM data
        void provideResult(_AACFrameInfo_t &info){
            LOG(Debug, "provideResult: %d samples",info.outputSamps);
            // provide result
            if(pwmCallback!=nullptr){
                // output via callback
                pwmCallback(info, pwm_buffer,info.outputSamps);
            } else {
                // output to stream
                if (info.sampRateOut!=_AACFrameInfo_t.sampRateOut && infoCallback!=nullptr){
                    infoCallback(info);
                }
                out->write((uint8_t*)pwm_buffer, info.outputSamps);
            }
            _AACFrameInfo_t = info;
            checkMemory();
        }            

        /// checks the consistency of the memory
        virtual void checkMemory(){
            //assert(frame_buffer[maxFrameSize()+1]==0);
            //assert(pwm_buffer[maxPWMSize()+1]==-1);
        }

};