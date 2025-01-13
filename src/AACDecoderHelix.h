#pragma once

#include "CommonHelix.h"
#include "libhelix-aac/aacdec.h"

namespace libhelix {

typedef void (*AACInfoCallback)(_AACFrameInfo &info, void *ref);
typedef void (*AACDataCallback)(_AACFrameInfo &info, short *pcm_buffer,
                                size_t len, void *ref);

/**
 * @brief A simple Arduino API for the libhelix AAC decoder. The data us
 * provided with the help of write() calls. The decoded result is available
 * either via a callback method or via an output stream.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class AACDecoderHelix : public CommonHelix {
 public:
  AACDecoderHelix() {
    setMinFrameBufferSize(AAC_MIN_FRAME_SIZE);
  }

#if defined(ARDUINO) || defined(HELIX_PRINT)
  AACDecoderHelix(Print &output) { 
    setMinFrameBufferSize(AAC_MIN_FRAME_SIZE);
    this->out = &output; 
  }
#endif

  AACDecoderHelix(AACDataCallback dataCallback) {
    setMinFrameBufferSize(AAC_MIN_FRAME_SIZE);
    this->pcmCallback = dataCallback;
  }

  virtual ~AACDecoderHelix() { end(); }

  void setInfoCallback(AACInfoCallback cb, void *caller = nullptr) {
    this->infoCallback = cb;
    if (caller!=nullptr)
      p_caller_ref = caller;
  }

  void setDataCallback(AACDataCallback cb) { this->pcmCallback = cb; }

  /// Provides the last available _AACFrameInfo_t
  _AACFrameInfo audioInfo() { return aacFrameInfo; }

  /// Releases the reserved memory
  virtual void end() override {
    LOG_HELIX(LogLevelHelix::Debug, "end");
    if (decoder != nullptr) {
      flush();
      AACFreeDecoder(decoder);
      decoder = nullptr;
    }
    CommonHelix::end();
    memset(&aacFrameInfo, 0, sizeof(_AACFrameInfo));
  }

  size_t maxFrameSize() override {
    return max_frame_size == 0 ? AAC_MAX_FRAME_SIZE : max_frame_size;
  }

  size_t maxPCMSize() override {
    return max_pcm_size == 0 ? AAC_MAX_OUTPUT_SIZE : max_pcm_size;
  }

  /// Used by M3A format
  void setAudioInfo(int channels, int samplerate) {
    memset(&aacFrameInfo, 0, sizeof(AACFrameInfo));
    aacFrameInfo.nChans = channels;
    // aacFrameInfo.bitsPerSample = bits; not used
    aacFrameInfo.sampRateCore = samplerate;
    aacFrameInfo.profile = AAC_PROFILE_LC;
    AACSetRawBlockParams(decoder, 0, &aacFrameInfo);
  }

 protected:
  HAACDecoder decoder = nullptr;
  AACDataCallback pcmCallback = nullptr;
  AACInfoCallback infoCallback = nullptr;
  _AACFrameInfo aacFrameInfo;
  void *p_caller_data = nullptr;

  /// Allocate the decoder
  virtual bool allocateDecoder() override {
    if (decoder == nullptr) {
      decoder = AACInitDecoder();
    }
    memset(&aacFrameInfo, 0, sizeof(_AACFrameInfo));
    return decoder != nullptr;
  }

  /// finds the sync word in the buffer
  int findSynchWord(int offset = 0) override {
    if (offset > frame_buffer.available()) return -1;
    int result = AACFindSyncWord(frame_buffer.data() + offset,
                                 frame_buffer.available() - offset);
    if (result < 0) return result;
    return offset == 0 ? result : result - offset;
  }

  /// decods the data and removes the decoded frame from the buffer
  int decode() override {
    int processed = 0;
    int available = frame_buffer.available();
    int bytes_left = frame_buffer.available();
    uint8_t *data = frame_buffer.data();
    int rc = AACDecode(decoder, &data, &bytes_left, (short *)pcm_buffer.data());
    if (rc == 0) {    
      processed = data - frame_buffer.data();
      // return the decoded result
      _AACFrameInfo info;
      AACGetLastFrameInfo(decoder, &info);
      provideResult(info);
    }
    return processed;
  }

  // return the result PCM data
  void provideResult(_AACFrameInfo &info) {
    // increase PCM size if this fails
    int sampleSize = info.bitsPerSample / 8;
    assert(info.outputSamps * sampleSize <= maxPCMSize());
    
    LOG_HELIX(LogLevelHelix::Debug, "==> provideResult: %d samples", info.outputSamps);
    if (info.outputSamps > 0) {
      // provide result
      if (pcmCallback != nullptr) {
        // output via callback
        pcmCallback(info, (short *)pcm_buffer.data(), info.outputSamps,
                    p_caller_data);
      } else {
        // output to stream
        if (info.sampRateOut != aacFrameInfo.sampRateOut &&
            infoCallback != nullptr) {
          infoCallback(info, p_caller_ref);
        }
#if defined(ARDUINO) || defined(HELIX_PRINT)
        if (out != nullptr){
          size_t to_write = info.outputSamps * sampleSize;
          size_t written = out->write((uint8_t *)pcm_buffer.data(), to_write);
          assert(written == to_write);
        }
#endif
      }
      aacFrameInfo = info;
    }
  }
};
}  // namespace libhelix
