#pragma once

// User Settings: Activate/Deactivate logging
#ifndef HELIX_LOGGING_ACTIVE
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
    static char log_buffer[512];
    enum LogLevel {Debug, Info, Warning, Error};
    static LogLevel minLogLevel = HELIX_LOG_LEVEL;
    // We print the log based on the log level
    #define LOG(level,...) { if(level>=minLogLevel) { int l = snprintf(log_buffer,512, __VA_ARGS__); HELIX_LOGGING_OUT.write("libhelix - "); HELIX_LOGGING_OUT.write(log_buffer,l); HELIX_LOGGING_OUT.println(); } }
#else
    // Remove all log statments from the code
    #define LOG(...) 
#endif
