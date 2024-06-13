#pragma once
#include "stdio.h"  // for snprintf

// User Settings: Activate/Deactivate logging
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

#if defined(ARDUINO) && HELIX_LOGGING_ACTIVE 
#  include  "Arduino.h" // for Serial; include Serial.h does not work
#endif

// Logging Implementation
#if HELIX_LOGGING_ACTIVE == true
    extern char log_buffer_helix[HELIX_LOG_SIZE];
    enum class LogLevelHelix {Debug, Info, Warning, Error};
    static LogLevelHelix LOGLEVEL_HELIX = HELIX_LOG_LEVEL;
    // We print the log based on the log level
    #define LOG_HELIX(level,...) { if(level>=LOGLEVEL_HELIX) { int l = snprintf(log_buffer_helix,512, __VA_ARGS__); HELIX_LOGGING_OUT.print("libhelix - "); HELIX_LOGGING_OUT.write(log_buffer_helix,l); HELIX_LOGGING_OUT.println(); } }
#else
    // Remove all log statments from the code
    #define LOG_HELIX(...) 
#endif
