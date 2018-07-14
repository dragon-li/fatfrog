/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef LIYL_IMEDIACODECLIST_H
#define LIYL_IMEDIACODECLIST_H

#include "../../foundation/system/core/utils/Errors.h" //for status_t
#include "../../common/include/IInterface.h"
#include "../../foundation/media/include/ABase.h"

#include "../../foundation/media/include/AMessage.h"

LIYL_NAMESPACE_START

class MediaCodecInfo;

class IMediaCodecList: public IInterface
{
public:
    LIYL_DECLARE_META_INTERFACE(MediaCodecList);

    virtual size_t countCodecs() const = 0;
    virtual sp<MediaCodecInfo> getCodecInfo(size_t index) const = 0;

    virtual const sp<AMessage> getGlobalSettings() const = 0;

    virtual ssize_t findCodecByType(
            const char *type, bool encoder, size_t startIndex = 0) const = 0;

    virtual ssize_t findCodecByName(const char *name) const = 0;
};


LIYL_NAMESPACE_END

#endif // LIYL_IMEDIACODECLIST_H
