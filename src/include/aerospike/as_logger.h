/******************************************************************************
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/
#pragma once

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

/*****************************************************************************
 * MACROS
 *****************************************************************************/

/**
 * Test if logging of AS_LOG_TRACE message is enabled
 */
#define as_logger_trace_enabled(__logger) \
    as_logger_enabled(__logger, AS_LOGGER_LEVEL_TRACE)

/**
 * Test if logging of AS_LOG_DEBUG message is enabled
 */
#define as_logger_debug_enabled(__logger) \
    as_logger_enabled(__logger, AS_LOGGER_LEVEL_DEBUG)

/**
 * Test if logging of AS_LOG_INFO message is enabled
 */
#define as_logger_info_enabled(__logger) \
    as_logger_enabled(__logger, AS_LOGGER_LEVEL_INFO)

/**
 * Test if logging of AS_LOG_WARN message is enabled
 */
#define as_logger_warn_enabled(__logger) \
    as_logger_enabled(__logger, AS_LOGGER_LEVEL_WARN)

/**
 * Test if logging of AS_LOG_ERROR message is enabled
 */
#define as_logger_error_enabled(__logger) \
    as_logger_enabled(__logger, AS_LOGGER_LEVEL_ERROR)


/**
 * Log an AS_LOG_ERROR message
 */
#define as_logger_trace(__logger, __message, __args...) \
    as_logger_log(__logger, AS_LOGGER_LEVEL_TRACE, __FILE__, __LINE__, __message, ##__args)

/**
 * Log an AS_LOG_DEBUG message
 */
#define as_logger_debug(__logger, __message, __args...) \
    as_logger_log(__logger, AS_LOGGER_LEVEL_DEBUG, __FILE__, __LINE__, __message, ##__args)

/**
 * Log an AS_LOG_INFO message
 */
#define as_logger_info(__logger, __message, __args...) \
    as_logger_log(__logger, AS_LOGGER_LEVEL_INFO, __FILE__, __LINE__, __message, ##__args)

/**
 * Log an AS_LOG_WARN message
 */
#define as_logger_warn(__logger, __message, __args...) \
    as_logger_log(__logger, AS_LOGGER_LEVEL_WARN, __FILE__, __LINE__, __message, ##__args)

/**
 * Log an AS_LOG_ERROR message
 */
#define as_logger_error(__logger, __message, __args...) \
    as_logger_log(__logger, AS_LOGGER_LEVEL_ERROR, __FILE__, __LINE__, __message, ##__args)

/*****************************************************************************
 * TYPES
 *****************************************************************************/

/**
 * The supported logging levels
 */
typedef enum as_logger_level_e {
    AS_LOGGER_LEVEL_TRACE     = 0,
    AS_LOGGER_LEVEL_DEBUG     = 1,
    AS_LOGGER_LEVEL_INFO      = 2,
    AS_LOGGER_LEVEL_WARN      = 3,
    AS_LOGGER_LEVEL_ERROR     = 4
} as_logger_level;

struct as_logger_hooks_s;

/**
 * Logger handle
 */
typedef struct as_logger_s {
    bool	free;
    void *	source;
    const struct as_logger_hooks_s * hooks;
} as_logger;


/**
 * The interface which all loggers should implement.
 */
typedef struct as_logger_hooks_s {

    /**
     * The destroy should free resources associated with the logger's source.
     * The destroy should not free the logger itself.
     */
    int (* destroy)(as_logger *);

    /**
     * Test if the log level is enabled for the logger.
     */
    int (* enabled)(const as_logger *, const as_logger_level);

    /**
     * Get the current log level of the logger.
     */
    as_logger_level (* level)(const as_logger *);

    /**
     * Log a message using the logger.
     */
    int (* log)(const as_logger *, const as_logger_level, const char *, const int, const char *, va_list);

} as_logger_hooks;




/*****************************************************************************
 * FUNCTIONS
 *****************************************************************************/

/**
 * Initialize a stack allocated logger
 */
as_logger * as_logger_init(as_logger * logger, void * source, const as_logger_hooks * hooks);

/**
 * Heap allocate and initialize a logger
 */
as_logger * as_logger_new(void * source, const as_logger_hooks * hooks);

/**
 * Release resources associated with the logger.
 * Calls logger->destroy. If success and if this is a heap allocated
 * logger, then it will be freed.
 */
int as_logger_destroy(as_logger * logger);


/**
 * Test if the log level is enabled for the logger.
 *
 * For most purposes, you should use the macros:
 *   - as_logger_trace_enabled(logger)
 *   - as_logger_debug_enabled(logger)
 *   - as_logger_info_enabled(logger)
 *   - as_logger_warn_enabled(logger)
 *   - as_logger_error_enabled(logger)
 * 
 * Usage:
 *   if ( as_logger_enabled(logger, AS_LOG_DEBUG) ) {
 *       char * foo = tostring(x);
 *       as_logger_debug(logger, "foo = %s", foo);
 *       free(foo);
 *   }
 *
 */
bool as_logger_is_enabled(const as_logger * logger, const as_logger_level level);

/**
 * Get the current log level for the logger.
 */
as_logger_level as_logger_get_level(const as_logger * logger);

/**
 * Log a message using the logger.
 *
 * For most purposes, you should use the macros:
 *   - as_logger_trace(logger, message, ...)
 *   - as_logger_debug(logger, message, ...)
 *   - as_logger_info_(logger, message, ...)
 *   - as_logger_warn_(logger, message, ...)
 *   - as_logger_error(logger, message, ...)
 * 
 * Usage:
 *   as_logger_log(logger, AS_LOG_DEBUG, __FILE__, __LINE__, "Hello %s", "Bob");
 *
 */
int as_logger_log(const as_logger * logger, const as_logger_level level, const char * file, const int line, const char * format, ...);
