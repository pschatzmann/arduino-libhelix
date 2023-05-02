#pragma once

#include "CommonHelix.h"
#include "libhelix-mp3/mp3dec.h"
#include "libhelix-mp3/mp3common.h"

#define MP3_MAX_OUTPUT_SIZE 1024 * 5
#define MP3_MAX_FRAME_SIZE 1600 

namespace libhelix {


typedef void (*MP3InfoCallback)(MP3FrameInfo &info, void* caller);
typedef void (*MP3DataCallback)(MP3FrameInfo &info,short *pwm_buffer, size_t len, void* caller);

enum MP3Type {MP3Normal=0, MP3SelfContaind=1};


/**
 * @brief A simple Arduino API for the libhelix MP3 decoder. The data is provided with the help of write() calls.
 * The decoded result is available either via a callback method or via an output stream.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class MP3DecoderHelix : public CommonHelix {

    public:
        MP3DecoderHelix(){
            this->mp3_type = MP3Normal;
        }

#ifdef ARDUINO
        MP3DecoderHelix(Print &output, MP3Type mp3Type=MP3Normal){
            this->out = &output;
            this->mp3_type = mp3Type;
        }
#endif
        MP3DecoderHelix(MP3DataCallback dataCallback, MP3Type mp3Type=MP3Normal){
            this->pcmCallback = dataCallback;
            this->mp3_type = mp3Type;
        }

        MP3DecoderHelix(MP3Type mp3Type){
            this->mp3_type = mp3Type;
        }

        virtual ~MP3DecoderHelix(){
            end();
        }

        void setInfoCallback(MP3InfoCallback cb, void* caller=nullptr){
            this->infoCallback = cb;
            p_caller_info = caller;
        }

        void setDataCallback(MP3DataCallback cb){
            this->pcmCallback = cb;
        }

        /// Provides the last available MP3FrameInfo
        MP3FrameInfo audioInfo(){
            return mp3FrameInfo;
        }

        /// Releases the reserved memory
        virtual void end() override {
            LOG_HELIX(Debug, "end");
            flush();

            if (decoder!=nullptr){
                MP3FreeDecoder(decoder);
                decoder = nullptr;
            }
            CommonHelix::end();
            memset(&mp3FrameInfo,0,sizeof(MP3FrameInfo));
        }

        /// determines the frame buffer size that will be allocated
        size_t maxFrameSize() override {
            return max_frame_size == 0 ? MP3_MAX_FRAME_SIZE : max_frame_size;
        }

        /// Determines the pcm buffer size that will be allocated
        size_t maxPCMSize() override {
            return max_pcm_size == 0 ? MP3_MAX_OUTPUT_SIZE : max_pcm_size;
        }

    protected:
        HMP3Decoder decoder = nullptr;
        MP3DataCallback pcmCallback = nullptr;
        MP3InfoCallback infoCallback = nullptr;
        MP3Type mp3_type;
        MP3FrameInfo mp3FrameInfo;
        void *p_caller_info = nullptr;
        void *p_caller_data = nullptr;

        /// Allocate the decoder
        virtual void allocateDecoder() override {
            if (decoder==nullptr){
                decoder = MP3InitDecoder();
            }
        }


        /// Finds the synch word in the available buffer data starting from the indicated offset
        int findSynchWord(int offset=0) override {
            int result = MP3FindSyncWord(frame_buffer+offset, buffer_size);
            return result < 0 ? result : result + offset;
        }

        /// decods the data 
        void decode(Range r) override {
            LOG_HELIX(Debug, "decode %d", r.end);
            int len = buffer_size - r.start;
            int bytesLeft =  len; 
            uint8_t* ptr = frame_buffer + r.start;

            int result = MP3Decode(decoder, &ptr, &bytesLeft, pcm_buffer, mp3_type);            
            int decoded = len - bytesLeft;

            if (result==0){
                LOG_HELIX(Debug, "-> bytesLeft %d -> %d  = %d ", buffer_size, bytesLeft, decoded);
                LOG_HELIX(Debug, "-> End of frame (%d) vs end of decoding (%d)", r.end, decoded)

                // return the decoded result
                MP3FrameInfo info;
                MP3GetLastFrameInfo(decoder, &info);
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

        // return the resulting PCM data
        void provideResult(MP3FrameInfo &info){
            // increase PCM size if this fails
            assert(info.outputSamps<maxPCMSize());

            LOG_HELIX(Debug, "=> provideResult: %d", info.outputSamps);
            if (info.outputSamps>0){
                // provide result
                if(pcmCallback!=nullptr){
                    // output via callback
                    pcmCallback(info, pcm_buffer, info.outputSamps, p_caller_data);
                } else {
                    // output to stream
                    if (info.samprate!=mp3FrameInfo.samprate  && infoCallback!=nullptr){
                        infoCallback(info, p_caller_info);
                    }
#ifdef ARDUINO
                    int sampleSize = info.bitsPerSample / 8;
                    out->write((uint8_t*)pcm_buffer, info.outputSamps*sampleSize);
#endif
                }
                mp3FrameInfo = info;
            }
        }

};

}