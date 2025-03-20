#pragma once
#if defined(ARDUINO) && __has_include("Arduino.h")
#include "Arduino.h"
#else
// remove delay statment if used outside of arduino
#include <stdint.h>
#define delay(ms)
#endif

// Not all processors support assert
#ifndef assert
#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) /*implementation defined*/
#endif
#endif

#include "ConfigHelix.h"
#include "utils/Allocator.h"
#include "utils/Buffers.h"
#include "utils/Vector.h"
#include "utils/helix_log.h"

namespace libhelix {

/**
 * @brief Common Simple Arduino API
 * @author Phil Schatzmann
 * @copyright GPLv3
 *
 */
class CommonHelix {
 public:
#if defined(ARDUINO) || defined(HELIX_PRINT)
  void setOutput(Print &output) { this->out = &output; }
#endif

  /**
   * @brief Starts the processing
   *
   */
  virtual bool begin() {
    frame_buffer.reset();
    frame_counter = 0;

    if (active) {
      end();
    }

    if (!allocateDecoder()) {
      return false;
    }
    frame_buffer.resize(maxFrameSize());
    pcm_buffer.resize(maxPCMSize());
    memset(pcm_buffer.data(), 0, maxPCMSize());
    memset(frame_buffer.data(), 0, maxFrameSize());
    active = true;
    return true;
  }

  /// Releases the reserved memory
  virtual void end() {
    frame_buffer.resize(0);
    pcm_buffer.resize(0);

    active = false;
  }

  /**
   * @brief decodes the next segments from the input.
   * The data can be provided in one big or in small incremental pieces.
   * It is suggested to be called in the Arduino Loop. If the provided data does
   * not fit into the buffer it is split up into small pieces that fit
   */
  virtual size_t write(const void *in_ptr, size_t in_size) {
    LOGI_HELIX("write %zu", in_size);
    int open = in_size;
    size_t processed = 0;
    uint8_t *data = (uint8_t *)in_ptr;
    while (open > 0) {
      int bytes = writeChunk(data, MIN(open, HELIX_CHUNK_SIZE));
      // if we did not advance we leave the loop
      if (bytes == 0) break;
      open -= bytes;
      data += bytes;
      processed += bytes;
    }
    return processed;
  }

  /// returns true if active
  operator bool() { return active; }

#ifdef ARDUINO
  /// Provides the timestamp in ms of last write
  uint64_t timeOfLastWrite() { return time_last_write; }

  /// Provides the timestamp in ms of last decoded result
  uint64_t timeOfLastResult() { return time_last_result; }
#endif

  /// Decode all open packets
  void flush() {
    int rc = 1;
    while (rc >= 0) {
      // we must start with sych word
      if (!presync()) break;
      // we must end with synch world
      if (findSynchWord(3) < 0) break;
      rc = decode();
      if (!resynch(rc)) break;
      // remove processed data
      if (rc > 0) frame_buffer.clearArray(rc);
    }
  }

  /// Provides the maximum frame size in bytes - this is allocated on the heap
  /// and you can reduce the heap size my minimizing this value
  virtual size_t maxFrameSize() = 0;

  /// Define your optimized maximum frame size in bytes
  void setMaxFrameSize(size_t len) { max_frame_size = len; }

  /// Provides the maximum pcm buffer size in bytes - this is allocated on the
  /// heap and you can reduce the heap size my minimizing this value
  virtual size_t maxPCMSize() = 0;

  /// Define your optimized maximum pcm buffer size in bytes
  void setMaxPCMSize(size_t len) { max_pcm_size = len; }

  /// Define some additional information which will be provided back in the
  /// callbacks
  void setReference(void *ref) { p_caller_ref = ref; }

#if defined(ARDUINO) || defined(HELIX_PRINT)
  /// Defines the max chunk size that is wrtten to out (Arduino only)
  void setMaxPCMWriteSize(int size) { max_write_size = size; }
#endif

 protected:
  bool active = false;
  bool is_raw = false;
  Vector<uint8_t> pcm_buffer{0};
  SingleBuffer<uint8_t> frame_buffer{0};
  size_t max_frame_size = 0;
  size_t max_pcm_size = 0;
  size_t frame_counter = 0;
  size_t max_write_size = 1024;
  int delay_ms = -1;
  int parse_0_count = 0;  // keep track of parser returning 0
  int min_frame_buffer_size = 0;
  uint64_t time_last_write = 0;
  uint64_t time_last_result = 0;
  void *p_caller_ref = nullptr;

#if defined(ARDUINO) || defined(HELIX_PRINT)
  Print *out = nullptr;
#endif

  /// make sure that we start with a valid sync: remove ID3 data
  bool presync() {
    LOGD_HELIX("presynch");
    bool rc = true;
    int pos = findSynchWord();
    if (pos > 3) rc = removeInvalidData(pos);

    return rc;
  }

  /// advance on invalid data, returns true if we need to continue the
  /// processing
  bool resynch(int rc) {
    LOGD_HELIX("resynch: %d", rc);
    // reset 0 result counter
    if (rc != 0) parse_0_count = 0;
    if (rc <= 0) {
      if (rc == 0) {
        parse_0_count++;
        int pos = findSynchWord(SYNCH_WORD_LEN);
        LOGD_HELIX("rc: %d - available %d - pos %d", rc,
                   frame_buffer.available(), pos);
        // if we are stuck, request more data and if this does not help we
        // remove the invalid data
        if (parse_0_count > 2) {
          return removeInvalidData(pos);
        }
        return false;
      } else if (rc == -1) {
        // underflow
        LOGD_HELIX("rc: %d - available %d", rc, frame_buffer.available());
        return false;
      } else {
        // generic error handling: remove the data until the next synch word
        int pos = findSynchWord(SYNCH_WORD_LEN + 1);
        removeInvalidData(pos);
      }
    }
    return true;
  }

  /// removes invalid data not starting with a synch word.
  /// @return Returns true if we still have data to be played
  bool removeInvalidData(int pos) {
    LOGD_HELIX("removeInvalidData: %d", pos);
    if (pos > 0) {
      LOGI_HELIX("removing: %d bytes", pos);
      frame_buffer.clearArray(pos);
      return true;
    } else if (pos <= 0) {
      frame_buffer.reset();
      return false;
    }
    return true;
  }

  /// Decoding Loop: We decode the procided data until we run out of data
  virtual size_t writeChunk(const void *in_ptr, size_t in_size) {
    LOGI_HELIX("writeChunk %zu", in_size);
#ifdef ARDUINO
    time_last_write = millis();
#endif
    size_t result = frame_buffer.writeArray((uint8_t *)in_ptr, in_size);

    while (frame_buffer.available() >= minFrameBufferSize()) {
      if (!presync()) break;
      int rc = decode();
      if (!resynch(rc)) break;
      // remove processed data
      frame_buffer.clearArray(rc);

      LOGI_HELIX("rc: %d - available %d", rc, frame_buffer.available());
    }

    return result;
  }

#if defined(ARDUINO) || defined(HELIX_PRINT)
  size_t writeToOut(uint8_t *data, size_t len) {
    size_t to_write = len;
    size_t written = 0;
    while (to_write > 0) {
      size_t result = out->write(data + written, MIN(to_write, 1024));
      to_write -= result;
      written += result;
    }
    if (len != written)
      LOGE_HELIX("Could not write result to out: %d of %d written", (int)written,
                 (int)len);
    return written;
  }
#endif

  /// Decode w/o parsing
  virtual int decode() = 0;

  /// Allocate the decoder
  virtual bool allocateDecoder() = 0;

  /// Finds the synchronization word in the frame buffer (starting from the
  /// indicated offset)
  virtual int findSynchWord(int offset = 0) = 0;

  /// Provides the actual minimum frame buffer size
  virtual int minFrameBufferSize() { return min_frame_buffer_size; }
  /// Defines the minimum frame buffer size which is required before starting
  /// the decoding
  virtual void setMinFrameBufferSize(int size) { min_frame_buffer_size = size; }
};

}  // namespace libhelix
