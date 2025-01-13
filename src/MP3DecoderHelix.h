#pragma once

#include "CommonHelix.h"
#include "libhelix-mp3/mp3common.h"
#include "libhelix-mp3/mp3dec.h"


namespace libhelix {

typedef void (*MP3InfoCallback)(MP3FrameInfo &info, void *ref);
typedef void (*MP3DataCallback)(MP3FrameInfo &info, short *pcm_buffer,
                                size_t len, void *ref);

enum MP3Type { MP3Normal = 0, MP3SelfContaind = 1 };

/**
 * @brief A simple Arduino API for the libhelix MP3 decoder. The data is
 * provided with the help of write() calls. The decoded result is available
 * either via a callback method or via an output stream.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class MP3DecoderHelix : public CommonHelix {
 public:
  MP3DecoderHelix() { 
    this->mp3_type = MP3Normal; 
    setMinFrameBufferSize(MP3_MIN_FRAME_SIZE);
  }

#if defined(ARDUINO) || defined(HELIX_PRINT)
  MP3DecoderHelix(Print &output, MP3Type mp3Type = MP3Normal) {
    this->out = &output;
    this->mp3_type = mp3Type;
    setMinFrameBufferSize(MP3_MIN_FRAME_SIZE);
  }
#endif
  MP3DecoderHelix(MP3DataCallback dataCallback, MP3Type mp3Type = MP3Normal) {
    this->pcmCallback = dataCallback;
    this->mp3_type = mp3Type;
    setMinFrameBufferSize(MP3_MIN_FRAME_SIZE);
  }

  MP3DecoderHelix(MP3Type mp3Type) { 
    this->mp3_type = mp3Type;
    setMinFrameBufferSize(MP3_MIN_FRAME_SIZE);
  }

  virtual ~MP3DecoderHelix() { end(); }

  void setInfoCallback(MP3InfoCallback cb, void *caller = nullptr) {
    this->infoCallback = cb;
    if (caller!=nullptr)
      p_caller_ref = caller;
  }

  void setDataCallback(MP3DataCallback cb) { this->pcmCallback = cb; }

  /// Provides the last available MP3FrameInfo
  MP3FrameInfo audioInfo() { return mp3FrameInfo; }

  /// Releases the reserved memory
  void end() override {
    LOG_HELIX(LogLevelHelix::Debug, "end");

    if (decoder != nullptr) {
      flush();
      MP3FreeDecoder(decoder);
      decoder = nullptr;
    }
    CommonHelix::end();
    memset(&mp3FrameInfo, 0, sizeof(MP3FrameInfo));
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
  void *p_caller_data = nullptr;

  /// Allocate the decoder
  virtual bool allocateDecoder() override {
    if (decoder == nullptr) {
      decoder = MP3InitDecoder();
    }
    memset(&mp3FrameInfo, 0, sizeof(MP3FrameInfo));
    return decoder != nullptr;
  }

  /// Finds the synch word in the available buffer data starting from the
  /// indicated offset
  int findSynchWord(int offset = 0) override {
    if (offset > frame_buffer.available()) return -1;
    int result = MP3FindSyncWord(frame_buffer.data() + offset,
                                 frame_buffer.available() - offset);
    if (result < 0) return result;
    return offset == 0 ? result : result - offset;
  }

  /// decods the data and removes the decoded frame from the buffer
  int decode() override {
    int processed = 0;
    int available = frame_buffer.available();
    int bytes_left = frame_buffer.available();
    LOG_HELIX(LogLevelHelix::Info, "decode: %d", frame_buffer.available());
    uint8_t *data = frame_buffer.data();
    int rc = MP3Decode(decoder, &data, &bytes_left, (short *)pcm_buffer.data(),
                       mp3_type);
    if (rc == 0) {
      processed = data - frame_buffer.data();
      // return the decoded result
      MP3FrameInfo info;
      MP3GetLastFrameInfo(decoder, &info);
      provideResult(info);
    } else {
      LOG_HELIX(LogLevelHelix::Info, "MP3Decode rc: %d", rc);
    }
    return processed;
  }

  // return the resulting PCM data
  void provideResult(MP3FrameInfo &info) {
    // increase PCM size if this fails
    assert(info.outputSamps * sizeof(short) < maxPCMSize());

    LOG_HELIX(LogLevelHelix::Debug, "=> provideResult: %d", info.outputSamps);
    if (info.outputSamps > 0) {
      // provide result
      if (pcmCallback != nullptr) {
        // output via callback
        pcmCallback(info, (short *)pcm_buffer.data(), info.outputSamps,
                    p_caller_data);
      } else {
        // output to stream
        if (info.samprate != mp3FrameInfo.samprate && infoCallback != nullptr) {
          infoCallback(info, p_caller_ref);
        }
#if defined(ARDUINO) || defined(HELIX_PRINT)
        if (out != nullptr){
          int sampleSize = info.bitsPerSample / 8;
          int toWrite = info.outputSamps * sampleSize;
          int written = out->write((uint8_t *)pcm_buffer.data(), toWrite);
          // assume blocking write
          assert(written == toWrite); 
        }
#endif
      }
      mp3FrameInfo = info;
    }
  }
};

}  // namespace libhelix