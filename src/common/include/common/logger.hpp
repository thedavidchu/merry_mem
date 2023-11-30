#pragma once
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
/// LOGGING MACROS (not thread safe)
////////////////////////////////////////////////////////////////////////////////

/// Create my own sketchy logger
#define LOG_LEVEL_TRACE 6
#define LOG_LEVEL_DEBUG 5
#define LOG_LEVEL_INFO  4
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_FATAL 1
#define LOG_LEVEL_OFF   0

#ifndef LOG_LEVEL
    #define LOG_LEVEL       LOG_LEVEL_OFF
#endif

#define LOG_TRACE(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_TRACE) {                                                        \
            std::cout << "[TRACE]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_DEBUG(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {                                                        \
            std::cout << "[DEBUG]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_INFO(x)                                                                                \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_INFO) {                                                         \
            std::cout << "[INFO]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__      \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_WARN(x)                                                                                \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_WARN) {                                                         \
            std::cout << "[WARN]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__      \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_ERROR(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_ERROR) {                                                        \
            std::cout << "[ERROR]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_FATAL(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_FATAL) {                                                        \
            std::cout << "[FATAL]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
