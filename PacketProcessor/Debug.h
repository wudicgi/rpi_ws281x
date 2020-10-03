#ifndef _DEBUG_H_
#define _DEBUG_H_

extern int ets_uart_printf(const char *fmt, ...);

// #define DEBUG


// None: Show no log messages.
#define DEBUG_LOG_LEVEL_NONE        0

// Assert: Show issues that the developer expects should never happen.
#define DEBUG_LOG_LEVEL_ASSERT      1

// Error: Show issues that have caused errors.
#define DEBUG_LOG_LEVEL_ERROR       2

// Warn: Show possible issues that are not yet errors.
#define DEBUG_LOG_LEVEL_WARN        3

// Info: Show expected log messages for regular usage.
#define DEBUG_LOG_LEVEL_INFO        4

// Debug: Show debug log messages that are useful during development only.
#define DEBUG_LOG_LEVEL_DEBUG       5

// Verbose: Show all log messages.
#define DEBUG_LOG_LEVEL_VERBOSE     6


#ifndef DEBUG_LOG_LEVEL
    #define DEBUG_LOG_LEVEL         DEBUG_LOG_LEVEL_VERBOSE
#endif



#undef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL             DEBUG_LOG_LEVEL_NONE
// #define DEBUG_LOG_LEVEL             DEBUG_LOG_LEVEL_INFO
// #define DEBUG_LOG_LEVEL             DEBUG_LOG_LEVEL_DEBUG




#if (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_ASSERT)
    #define LOG_ASSERT(fmt, ...)    Debug_printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_ASSERT(fmt, ...)
#endif

#if (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_ERROR)
    #define LOG_ERROR(fmt, ...)     Debug_printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_ERROR(fmt, ...)
#endif

#if (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_WARN)
    #define LOG_WARN(fmt, ...)      Debug_printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_WARN(fmt, ...)
#endif

#if (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_INFO)
    #define LOG_INFO(fmt, ...)      Debug_printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_INFO(fmt, ...)
#endif

#if (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_DEBUG)
    #define LOG_DEBUG(fmt, ...)     Debug_printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)
#endif

#if (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_VERBOSE)
    #define LOG_VERBOSE(fmt, ...)   Debug_printf(fmt, ##__VA_ARGS__)
#else
    #define LOG_VERBOSE(fmt, ...)
#endif


#ifdef DEBUG
    #define Debug_printf(fmt, ...)  \
        do {    \
            printf("%s #%d %s(): " fmt,    \
                    __FILE__, __LINE__, __func__, ##__VA_ARGS__);   \
        } while (0);
#else
    #define Debug_printf(fmt, ...)
#endif

#ifdef DEBUG
    void Debug_printHex(const char *variableName, uint8_t *buffer, int length);
#else
    #define Debug_printHex()
#endif

#endif
