/*
 * Copyright (C) 2007-2016 The Android Open Source Project
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

#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "log.h"

#define LOG_BUF_SIZE 1024







LIBLOG_ABI_PUBLIC int __liyl_log_write(int prio, const char *tag, const char *msg)
{
    return __liyl_log_buf_write(0/*LOG_ID_MAIN*/, prio, tag, msg);
}

LIBLOG_ABI_PUBLIC int __liyl_log_buf_write(int bufID, int prio, const char *tag, const char *msg)
{

    if (!tag)
        tag = "";

#if defined(__ANDROID__)
    if(prio == LIYL_LOG_FATAL){
        //TODO FIXME
        __android_log_write(ANDROID_LOG_ERROR,tag,msg);
    }else if(prio == LIYL_LOG_VERBOSE) {
        __android_log_write(ANDROID_LOG_VERBOSE,tag,msg);
    }else if(prio == LIYL_LOG_INFO) {
        __android_log_write(ANDROID_LOG_INFO,tag,msg);
    }else if(prio == LIYL_LOG_DEBUG) {
        __android_log_write(ANDROID_LOG_DEBUG,tag,msg);
    }else if(prio == LIYL_LOG_WARN) {
        __android_log_write(ANDROID_LOG_WARN,tag,msg);
    }else if(prio == LIYL_LOG_ERROR) {
        __android_log_write(ANDROID_LOG_ERROR,tag,msg);
    }else {
    }
#endif

    return 0; 
}

LIBLOG_ABI_PUBLIC int __liyl_log_vprint(int prio, const char *tag,
        const char *fmt, va_list ap)
{
    char buf[LOG_BUF_SIZE];

    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);

    return __liyl_log_write(prio, tag, buf);
}

LIBLOG_ABI_PUBLIC int __liyl_log_print(int prio, const char *tag,
        const char *fmt, ...)
{
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    return __liyl_log_write(prio, tag, buf);
}

LIBLOG_ABI_PUBLIC int __liyl_log_buf_print(int bufID, int prio,
        const char *tag,
        const char *fmt, ...)
{
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    return __liyl_log_buf_write(bufID, prio, tag, buf);
}

LIBLOG_ABI_PUBLIC void __liyl_log_assert(const char *cond, const char *tag,
        const char *fmt, ...)
{
    char buf[LOG_BUF_SIZE];

    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
        va_end(ap);
    } else {
        /* Msg not provided, log condition.  N.B. Do not use cond directly as
         * format string as it could contain spurious '%' syntax (e.g.
         * "%d" in "blocks%devs == 0").
         */
        if (cond)
            snprintf(buf, LOG_BUF_SIZE, "Assertion failed: %s", cond);
        else
            strcpy(buf, "Unspecified assertion failed");
    }

    __liyl_log_write(LIYL_LOG_FATAL, tag, buf);
    abort(); /* abort so we have a chance to debug the situation */
    /* NOTREACHED */
}

LIBLOG_ABI_PUBLIC int __liyl_log_bwrite(int32_t tag,
        const void *payload, size_t len)
{

    return 0; 
}

LIBLOG_ABI_PUBLIC int __liyl_log_security_bwrite(int32_t tag,
        const void *payload,
        size_t len)
{
    return 0; 
}

/*
 * Like __liyl_log_bwrite, but takes the type as well.  Doesn't work
 * for the general case where we're generating lists of stuff, but very
 * handy if we just want to dump an integer into the log.
 */
LIBLOG_ABI_PUBLIC int __liyl_log_btwrite(int32_t tag, char type,
        const void *payload, size_t len)
{
    return 0; 
}

/*
 * Like __liyl_log_bwrite, but used for writing strings to the
 * event log.
 */
LIBLOG_ABI_PUBLIC int __liyl_log_bswrite(int32_t tag, const char *payload)
{
    return 0; 
}

/*
 * Like __liyl_log_security_bwrite, but used for writing strings to the
 * security log.
 */
LIBLOG_ABI_PUBLIC int __liyl_log_security_bswrite(int32_t tag,
        const char *payload)
{

    return 0; 
}
