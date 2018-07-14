/*
 * Copyright (C) 2005-2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIYL_LOG_LOG_H
#define _LIYL_LOG_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "define.h"

#ifdef __cplusplus
extern "C" {
#endif

    // ---------------------------------------------------------------------

    /*
     * Normally we strip LLOGV (VERBOSE messages) from release builds.
     * You can modify this (for example with "#define LOG_NDEBUG 0"
     * at the top of your source file) to change that behavior.
     */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

    /*
     * This is the local tag used for the following simplified
     * logging macros.  You can change this preprocessor definition
     * before using the other macros to change the tag.
     */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

    //=======================================================================================
    /*
     *  * Android log priority values, in ascending priority order.
     *   */
    typedef enum liyl_LogPriority {
        LIYL_LOG_UNKNOWN = 0,
        LIYL_LOG_DEFAULT,    /* only for SetMinPriority() */
        LIYL_LOG_VERBOSE,
        LIYL_LOG_DEBUG,
        LIYL_LOG_INFO,
        LIYL_LOG_WARN,
        LIYL_LOG_ERROR,
        LIYL_LOG_FATAL,
        LIYL_LOG_SILENT,     /* only for SetMinPriority(); must be last */
    } liyl_LogPriority;
    /*
     *  * Send a simple string to the log.
     *   */
    int __liyl_log_write(int prio, const char *tag, const char *text);

    /*
     *  * Send a formatted string to the log, used like printf(fmt,...)
     *   */
    int __liyl_log_print(int prio, const char *tag,  const char *fmt, ...);

    /*
     *  * A variant of __liyl_log_print() that takes a va_list to list
     *   * additional parameters.
     *    */
    int __liyl_log_vprint(int prio, const char *tag,
            const char *fmt, va_list ap);

    /*
     *  * Log an assertion failure and abort the process to have a chance
     *   * to inspect it if a debugger is attached. This uses the FATAL priority.
     *    */
    void __liyl_log_assert(const char *cond, const char *tag,
            const char *fmt, ...);
    //=======================================================================================


    typedef enum log_id {
        LOG_ID_MIN = 0,
        LOG_ID_MAIN = 0,
        LOG_ID_EVENTS = 2,
        LOG_ID_SYSTEM = 3,
        LOG_ID_CRASH = 4,
        LOG_ID_SECURITY = 5,
        LOG_ID_KERNEL = 6, /* place last, third-parties can not use it */
        LOG_ID_MAX
    } log_id_t;
    // ---------------------------------------------------------------------

#ifndef __predict_false
#define __predict_false(exp) __builtin_expect((exp) != 0, 0)
#endif

    /*
     *      -DLINT_RLOG in sources that you want to enforce that all logging
     * goes to the radio log buffer. If any logging goes to any of the other
     * log buffers, there will be a compile or link error to highlight the
     * problem. This is not a replacement for a full audit of the code since
     * this only catches compiled code, not ifdef'd debug code. Options to
     * defining this, either temporarily to do a spot check, or permanently
     * to enforce, in all the communications trees; We have hopes to ensure
     * that by supplying just the radio log buffer that the communications
     * teams will have their one-stop shop for triaging issues.
     */

    /*
     * Simplified macro to send a verbose log message using the current LOG_TAG.
     */
#ifndef LLOGV
#define __LLOGV(...) ((void)LLOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define LLOGV(...) do { if (0) { __LLOGV(__VA_ARGS__); } } while (0)
#else
#define LLOGV(...) __LLOGV(__VA_ARGS__)
#endif
#endif

#ifndef LLOGV_IF
#if LOG_NDEBUG
#define LLOGV_IF(cond, ...)   ((void)0)
#else
#define LLOGV_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)LLOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif
#endif

    /*
     * Simplified macro to send a debug log message using the current LOG_TAG.
     */
#ifndef LLOGD
#define LLOGD(...) ((void)LLOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LLOGD_IF
#define LLOGD_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)LLOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    /*
     * Simplified macro to send an info log message using the current LOG_TAG.
     */
#ifndef LLOGI
#define LLOGI(...) ((void)LLOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LLOGI_IF
#define LLOGI_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)LLOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    /*
     * Simplified macro to send a warning log message using the current LOG_TAG.
     */
#ifndef LLOGW
#define LLOGW(...) ((void)LLOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LLOGW_IF
#define LLOGW_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)LLOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    /*
     * Simplified macro to send an error log message using the current LOG_TAG.
     */
#ifndef LLOGE
#define LLOGE(...) ((void)LLOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef LLOGE_IF
#define LLOGE_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)LLOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    // ---------------------------------------------------------------------

    /*
     * Conditional based on whether the current LOG_TAG is enabled at
     * verbose priority.
     */
#ifndef IF_LLOGV
#if LOG_NDEBUG
#define IF_LLOGV() if (false)
#else
#define IF_LLOGV() IF_LLOG(LOG_VERBOSE, LOG_TAG)
#endif
#endif

    /*
     * Conditional based on whether the current LOG_TAG is enabled at
     * debug priority.
     */
#ifndef IF_LLOGD
#define IF_LLOGD() IF_LLOG(LOG_DEBUG, LOG_TAG)
#endif

    /*
     * Conditional based on whether the current LOG_TAG is enabled at
     * info priority.
     */
#ifndef IF_LLOGI
#define IF_LLOGI() IF_LLOG(LOG_INFO, LOG_TAG)
#endif

    /*
     * Conditional based on whether the current LOG_TAG is enabled at
     * warn priority.
     */
#ifndef IF_LLOGW
#define IF_LLOGW() IF_LLOG(LOG_WARN, LOG_TAG)
#endif

    /*
     * Conditional based on whether the current LOG_TAG is enabled at
     * error priority.
     */
#ifndef IF_LLOGE
#define IF_LLOGE() IF_LLOG(LOG_ERROR, LOG_TAG)
#endif


    // ---------------------------------------------------------------------

    /*
     * Simplified macro to send a verbose system log message using the current LOG_TAG.
     */
#ifndef SLOGV
#define __SLOGV(...) \
    ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define SLOGV(...) do { if (0) { __SLOGV(__VA_ARGS__); } } while (0)
#else
#define SLOGV(...) __SLOGV(__VA_ARGS__)
#endif
#endif

#ifndef SLOGV_IF
#if LOG_NDEBUG
#define SLOGV_IF(cond, ...)   ((void)0)
#else
#define SLOGV_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif
#endif

    /*
     * Simplified macro to send a debug system log message using the current LOG_TAG.
     */
#ifndef SLOGD
#define SLOGD(...) \
    ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGD_IF
#define SLOGD_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    /*
     * Simplified macro to send an info system log message using the current LOG_TAG.
     */
#ifndef SLOGI
#define SLOGI(...) \
    ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGI_IF
#define SLOGI_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_INFO, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    /*
     * Simplified macro to send a warning system log message using the current LOG_TAG.
     */
#ifndef SLOGW
#define SLOGW(...) \
    ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGW_IF
#define SLOGW_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_WARN, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif

    /*
     * Simplified macro to send an error system log message using the current LOG_TAG.
     */
#ifndef SLOGE
#define SLOGE(...) \
    ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGE_IF
#define SLOGE_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)__liyl_log_buf_print(LOG_ID_SYSTEM, LIYL_LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
      : (void)0 )
#endif



    // ---------------------------------------------------------------------

    /*
     * Log a fatal error.  If the given condition fails, this stops program
     * execution like a normal assertion, but also generating the given message.
     * It is NOT stripped from release builds.  Note that the condition test
     * is -inverted- from the normal assert() semantics.
     */
#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
    ( (__predict_false(cond)) \
      ? ((void)liyl_printAssert(#cond, LOG_TAG, ## __VA_ARGS__)) \
      : (void)0 )
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) \
    ( ((void)liyl_printAssert(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif

    /*
     * Versions of LOG_ALWAYS_FATAL_IF and LOG_ALWAYS_FATAL that
     * are stripped out of release builds.
     */
#if LOG_NDEBUG

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) ((void)0)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) ((void)0)
#endif

#else

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif

    /*
     * Assertion that generates a log message when the assertion fails.
     * Stripped out of release builds.  Uses the current LOG_TAG.
     */
#ifndef LLOG_ASSERT
#define LLOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
    //#define LLOG_ASSERT(cond) LOG_FATAL_IF(!(cond), "Assertion failed: " #cond)
#endif

    // ---------------------------------------------------------------------

    /*
     * Basic log message macro.
     *
     * Example:
     *  LLOG(LOG_WARN, NULL, "Failed with error %d", errno);
     *
     * The second argument may be NULL or "" to indicate the "global" tag.
     */
#ifndef LLOG
#define LLOG(priority, tag, ...) \
    LOG_PRI(LIYL_##priority, tag, __VA_ARGS__)
#endif

    /*
     * Log macro that allows you to specify a number for the priority.
     */
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
    liyl_printLog(priority, tag, __VA_ARGS__)
#endif

    /*
     * Log macro that allows you to pass in a varargs ("args" is a va_list).
     */
#ifndef LOG_PRI_VA
#define LOG_PRI_VA(priority, tag, fmt, args) \
    liyl_vprintLog(priority, NULL, tag, fmt, args)
#endif

    /*
     * Conditional given a desired logging priority and tag.
     */
#ifndef IF_LLOG
#define IF_LLOG(priority, tag) \
    if (liyl_testLog(LIYL_##priority, tag))
#endif


    /*
     * ===========================================================================
     *
     * The stuff in the rest of this file should not be used directly.
     */

#define liyl_printLog(prio, tag, fmt...) \
    __liyl_log_print(prio, tag, fmt)

#define liyl_vprintLog(prio, cond, tag, fmt...) \
    __liyl_log_vprint(prio, tag, fmt)

    /* XXX Macros to work around syntax errors in places where format string
     * arg is not passed to LLOG_ASSERT, LOG_ALWAYS_FATAL or LOG_ALWAYS_FATAL_IF
     * (happens only in debug builds).
     */

    /* Returns 2nd arg.  Used to substitute default value if caller's vararg list
     * is empty.
     */
#define __liyl_second(dummy, second, ...)     second

    /* If passed multiple args, returns ',' followed by all but 1st arg, otherwise
     * returns nothing.
     */
#define __liyl_rest(first, ...)               , ## __VA_ARGS__

#define liyl_printAssert(cond, tag, fmt...) \
    __liyl_log_assert(cond, tag, \
            __liyl_second(0, ## fmt, NULL) __liyl_rest(fmt))

#define liyl_writeLog(prio, tag, text) \
    __liyl_log_write(prio, tag, text)

#define liyl_bWriteLog(tag, payload, len) \
    __liyl_log_bwrite(tag, payload, len)
#define liyl_btWriteLog(tag, type, payload, len) \
    __liyl_log_btwrite(tag, type, payload, len)

#define liyl_errorWriteLog(tag, subTag) \
    __liyl_log_error_write(tag, subTag, -1, NULL, 0)

#define liyl_errorWriteWithInfoLog(tag, subTag, uid, data, dataLen) \
    __liyl_log_error_write(tag, subTag, uid, data, dataLen)

    /*
     *    IF_LLOG uses liyl_testLog, but IF_LLOG can be overridden.
     *    liyl_testLog will remain constant in its purpose as a wrapper
     *        for Android logging filter policy, and can be subject to
     *        change. It can be reused by the developers that override
     *        IF_LLOG as a convenient means to reimplement their policy
     *        over Android.
     */
#if LOG_NDEBUG /* Production */
#define liyl_testLog(prio, tag) \
    (__liyl_log_is_loggable(prio, tag, LIYL_LOG_DEBUG) != 0)
#else
#define liyl_testLog(prio, tag) \
    (__liyl_log_is_loggable(prio, tag, LIYL_LOG_VERBOSE) != 0)
#endif

    /*
     * Use the per-tag properties "log.tag.<tagname>" to generate a runtime
     * result of non-zero to expose a log. prio is LIYL_LOG_VERBOSE to
     * LIYL_LOG_FATAL. default_prio if no property. Undefined behavior if
     * any other value.
     */
    int __liyl_log_is_loggable(int prio, const char *tag, int default_prio);

    int __liyl_log_security(); /* Device Owner is present */

    int __liyl_log_error_write(int tag, const char *subTag, int32_t uid, const char *data,
            uint32_t dataLen);

    /*
     * Send a simple string to the log.
     */
    int __liyl_log_buf_write(int bufID, int prio, const char *tag, const char *text);
    int __liyl_log_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...)
#if defined(__GNUC__)
        __attribute__((__format__(printf, 4, 5)))
#endif
        ;

#ifdef __cplusplus
}
#endif

#endif /* _LIYL_LOG_LOG_H */
