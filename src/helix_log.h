#pragma once

// User Settings: Activate/Deactivate logging
#ifndef HELIX_LOGGING_ACTIVE
#define HELIX_LOGGING_ACTIVE false
#endif
#ifndef HELIX_LOG_LEVEL
#define HELIX_LOG_LEVEL Warning
#endif


#if HELIX_LOGGING_ACTIVE == true
static char log_buffer[512];
enum LogLevel {Debug, Info, Warning, Error};
static LogLevel minLogLevel = Debug;
// We print the log based on the log level
#define LOG(level,...) { if(level>=minLogLevel) { int l = snprintf(log_buffer,512, __VA_ARGS__);  Serial.write(log_buffer,l); Serial.println(); } }
#else
// Remove all log statments from the code
#define LOG(Debug, ...) 
#endif
