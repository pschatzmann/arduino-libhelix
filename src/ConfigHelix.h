#pragma once

// Common
#ifndef SYNCH_WORD_LEN
#  define SYNCH_WORD_LEN 4
#endif 

#ifndef HELIX_CHUNK_SIZE
#  define HELIX_CHUNK_SIZE 1024
#endif

// mp3
#ifndef MP3_MAX_OUTPUT_SIZE
#  define MP3_MAX_OUTPUT_SIZE 1024 * 5
#endif
#ifndef MP3_MAX_FRAME_SIZE
#  define MP3_MAX_FRAME_SIZE 1024 * 2
#endif
#ifndef MP3_MIN_FRAME_SIZE
#  define MP3_MIN_FRAME_SIZE 1024
#endif

// aac
#ifndef AAC_MAX_OUTPUT_SIZE
#  define AAC_MAX_OUTPUT_SIZE 1024 * 8
#endif
#ifndef AAC_MAX_FRAME_SIZE
#  define AAC_MAX_FRAME_SIZE 2100
#endif
#ifndef AAC_MIN_FRAME_SIZE
#  define AAC_MIN_FRAME_SIZE 1024
#endif

// Allocation: define allocator to be used
#define ALLOCATOR libhelix::AllocatorExt

// Logging: Activate/Deactivate logging
#if ! defined(HELIX_LOGGING_ACTIVE) && defined(ARDUINO)
#  define HELIX_LOGGING_ACTIVE true
#endif

#ifndef HELIX_LOG_LEVEL
#  define HELIX_LOG_LEVEL LogLevelHelix::Warning
#endif

#ifndef LOG_METHOD
#  define LOG_METHOD __PRETTY_FUNCTION__
#endif

#ifndef HELIX_LOGGING_OUT
#  define HELIX_LOGGING_OUT Serial
#endif

#ifndef HELIX_LOG_SIZE
#  define HELIX_LOG_SIZE 256
#endif