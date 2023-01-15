#pragma once

#include "CommonHelix.h"
#include "libhelix-aac/aacdec.h"

#define AAC_MAX_OUTPUT_SIZE 1024 * 3 
#define AAC_MAX_FRAME_SIZE 2100 

namespace libhelix {

typedef void (*AACInfoCallback)(_AACFrameInfo &info);
typedef void (*AACDataCallback)(_AACFrameInfo &info,short *pwm_buffer, size_t len);

/**
 * @brief A simple Arduino API for the libhelix AAC decoder. The data us provided with the help of write() calls.
 * The decoded result is available either via a callback method or via an output stream.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class AACDecoderHelix : public CommonHelix {
    public:
        AACDecoderHelix() = default;

#ifdef ARDUINO
        AACDecoderHelix(Print &output, AACInfoCallback infoCallback=nullptr){
            this->out = &output;
            this->infoCallback = infoCallback;
        }
#endif
        AACDecoderHelix(AACDataCallback dataCallback){
            this->pwmCallback = dataCallback;
        }

        virtual ~AACDecoderHelix(){
            end();
        }


        void setInfoCallback(AACInfoCallback cb){
            this->infoCallback = cb;
        }

        void setDataCallback(AACDataCallback cb){
            this->pwmCallback = cb;
        }


        /// Releases the reserved memory
        virtual void end() override {
            LOG_HELIX(Debug, "end");
            if (decoder!=nullptr){
                flush();
                AACFreeDecoder(decoder);
                decoder = nullptr;
            }
            CommonHelix::end();
            memset(&aacFrameInfo,0,sizeof(_AACFrameInfo));
        }

        /// Provides the last available _AACFrameInfo_t
        _AACFrameInfo audioInfo(){
            return aacFrameInfo;
        }

        size_t maxFrameSize() override {
            return max_frame_size == 0 ? AAC_MAX_FRAME_SIZE : max_frame_size;
        }

        size_t maxPWMSize() override {
            return max_pwm_size == 0 ? AAC_MAX_OUTPUT_SIZE : max_pwm_size;
        }

    protected:
        HAACDecoder decoder = nullptr;
        AACDataCallback pwmCallback = nullptr;
        AACInfoCallback infoCallback = nullptr;
        _AACFrameInfo aacFrameInfo;

        /// Allocate the decoder
        virtual void allocateDecoder() override {
            if (decoder==nullptr){
                decoder = AACInitDecoder();
            }
        }


        int findSynchWord(int offset=0) override {
            int result = AACFindSyncWord(frame_buffer+offset, buffer_size)+offset;
            return result < 0 ? result : result + offset;
        }

        /// decods the data and removes the decoded frame from the buffer
        void decode(Range r) override {
            LOG_HELIX(Debug, "decode %d", r.end);
            int len = buffer_size - r.start;
            int bytesLeft =  len; 
            uint8_t* ptr = frame_buffer + r.start;

            int result = AACDecode(decoder, &ptr, &bytesLeft, pwm_buffer);
            int decoded = len - bytesLeft;
            assert(decoded == ptr-(frame_buffer + r.start));
            if (result==0){
                LOG_HELIX(Debug, "-> bytesLeft %d -> %d  = %d ", buffer_size, bytesLeft, decoded);
                LOG_HELIX(Debug, "-> End of frame (%d) vs end of decoding (%d)", r.end, decoded)

                // return the decoded result
                _AACFrameInfo info;
                AACGetLastFrameInfo(decoder, &info);
                provideResult(info);

                // remove processed data from buffer 
                if (decoded<=buffer_size) {
                    buffer_size -= decoded;
                    //assert(buffer_size<=maxFrameSize());
                    memmove(frame_buffer, frame_buffer+r.start+decoded, buffer_size);
                    LOG_HELIX(Debug, " -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
                } else {
                    LOG_HELIX(Warning, " -> decoded %d > buffersize %d", decoded, buffer_size);
                    buffer_size = 0;
                }
            } else {
                // decoding error
                LOG_HELIX(Debug, " -> decode error: %d - removing frame!", result);
                int ignore = decoded;
                if (ignore == 0) ignore = r.end;
                // We advance to the next synch world
                if (ignore<=buffer_size){
                    buffer_size -= ignore;
                    memmove(frame_buffer, frame_buffer+ignore, buffer_size);
                }  else {
                    buffer_size = 0;
                }
            }
        }

        // return the result PWM data
        void provideResult(_AACFrameInfo &info){
            LOG_HELIX(Debug, "provideResult: %d samples",info.outputSamps);
             if (info.outputSamps>0){
            // provide result
                if(pwmCallback!=nullptr){
                    // output via callback
                    pwmCallback(info, pwm_buffer,info.outputSamps);
                } else {
                    // output to stream
                    if (info.sampRateOut!=aacFrameInfo.sampRateOut && infoCallback!=nullptr){
                        infoCallback(info);
                    }
#ifdef ARDUINO
                    int sampleSize = info.bitsPerSample / 8;
                    out->write((uint8_t*)pwm_buffer, info.outputSamps*sampleSize);
#endif
                }
                aacFrameInfo = info;
            }
        }            
};
}
