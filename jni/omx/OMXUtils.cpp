/*
 * Copyright (C) 2016 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "OMXUtils"

#include <string.h>
#include "../foundation/system/core/define.h"
#include "../common/include/HardwareAPI.h"
#include "../common/include/MediaErrors.h"
#include "OMXUtils.h"

LIYL_NAMESPACE_START

status_t StatusFromOMXError(OMX_ERRORTYPE err) {
    switch (err) {
        case OMX_ErrorNone:
            return OK;
        case OMX_ErrorUnsupportedSetting:
        case OMX_ErrorUnsupportedIndex:
            return ERROR_UNSUPPORTED; // this is a media specific error
        case OMX_ErrorInsufficientResources:
            return NO_MEMORY;
        case OMX_ErrorInvalidComponentName:
        case OMX_ErrorComponentNotFound:
            return NAME_NOT_FOUND;
        default:
            return UNKNOWN_ERROR;
    }
}

/**************************************************************************************************/

LIYL_NAMESPACE_END

