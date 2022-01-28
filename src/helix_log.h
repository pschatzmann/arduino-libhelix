#pragma once

// User Settings: Activate/Deactivate logging
#if ! defined(HELIX_LOGGING_ACTIVE) && defined(ARDUINO)
    #define HELIX_LOGGING_ACTIVE true
#endif

#ifndef HELIX_LOG_LEVEL
    #define HELIX_LOG_LEVEL Warning
#endif

#ifndef HELIX_LOGGING_OUT
    #define HELIX_LOGGING_OUT Serial
#endif

// Logging Implementation
#if HELIX_LOGGING_ACTIVE == true
    static char log_buffer_helix[512];
    enum LogLevelHelix {Debug, Info, Warning, Error};
    static LogLevelHelix minLogLevelHelix = HELIX_LOG_LEVEL;
    // We print the log based on the log level
    #define LOG_HELIX(level,...) { if(level>=minLogLevelHelix) { int l = snprintf(log_buffer_helix,512, __VA_ARGS__); HELIX_LOGGING_OUT.print("libhelix - "); HELIX_LOGGING_OUT.write(log_buffer_helix,l); HELIX_LOGGING_OUT.println(); } }
#else
    // Remove all log statments from the code
    #define LOG_HELIX(...) 
#endif
