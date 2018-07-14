/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef _UTILS_LIYL_THREAD_DEFS_H
#define _UTILS_LIYL_THREAD_DEFS_H

#include <stdint.h>
#include <sys/types.h>
#include "../define.h"
#include "../../android/thread_defs.h"

// ---------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef uint32_t liyl_thread_id_t;
#else
typedef void* liyl_thread_id_t;
#endif

typedef int (*liyl_thread_func_t)(void*);

#ifdef __cplusplus
} // extern "C"
#endif

// ---------------------------------------------------------------------------
// C++ API
#ifdef __cplusplus
LIYL_NAMESPACE_START
// ---------------------------------------------------------------------------

typedef liyl_thread_id_t thread_id_t;
typedef liyl_thread_func_t thread_func_t;

enum {
    PRIORITY_LOWEST         = LIYL_PRIORITY_LOWEST,
    PRIORITY_BACKGROUND     = LIYL_PRIORITY_BACKGROUND,
    PRIORITY_NORMAL         = LIYL_PRIORITY_NORMAL,
    PRIORITY_FOREGROUND     = LIYL_PRIORITY_FOREGROUND,
    PRIORITY_DISPLAY        = LIYL_PRIORITY_DISPLAY,
    PRIORITY_URGENT_DISPLAY = LIYL_PRIORITY_URGENT_DISPLAY,
    PRIORITY_AUDIO          = LIYL_PRIORITY_AUDIO,
    PRIORITY_URGENT_AUDIO   = LIYL_PRIORITY_URGENT_AUDIO,
    PRIORITY_HIGHEST        = LIYL_PRIORITY_HIGHEST,
    PRIORITY_DEFAULT        = LIYL_PRIORITY_DEFAULT,
    PRIORITY_MORE_FAVORABLE = LIYL_PRIORITY_MORE_FAVORABLE,
    PRIORITY_LESS_FAVORABLE = LIYL_PRIORITY_LESS_FAVORABLE,
};

// ---------------------------------------------------------------------------
LIYL_NAMESPACE_END
#endif  // __cplusplus
// ---------------------------------------------------------------------------


#endif // __UTILS_LIYL_THREAD_DEFS_H
