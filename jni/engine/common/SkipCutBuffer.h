/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef LIYL_SKIP_CUT_BUFFER_H_

#define LIYL_SKIP_CUT_BUFFER_H_

#include "../../foundation/media/include/ABuffer.h"

LIYL_NAMESPACE_START

/**
 * utility class to cut the start and end off a stream of data in MediaBuffers
 *
 */
class SkipCutBuffer: public RefBase {
 public:
    // 'skip' is the number of frames to skip from the beginning
    // 'cut' is the number of frames to cut from the end
    // 'num16BitChannels' is the number of channels, which are assumed to be 16 bit wide each
    SkipCutBuffer(size_t skip, size_t cut, size_t num16Channels);

    // Submit one ABuffer for skipping and cutting. This may consume all or
    // some of the data in the buffer, or it may add data to it.
    // After this, the caller should continue processing the buffer as usual.
    void submit(const sp<ABuffer>& buffer);
    void clear();
    size_t size(); // how many bytes are currently stored in the buffer

 protected:
    virtual ~SkipCutBuffer();

 private:
    void write(const char *src, size_t num);
    size_t read(char *dst, size_t num);
    int32_t mSkip;
    int32_t mFrontPadding;
    int32_t mBackPadding;
    int32_t mWriteHead;
    int32_t mReadHead;
    int32_t mCapacity;
    char* mCutBuffer;
    DISALLOW_EVIL_CONSTRUCTORS(SkipCutBuffer);
};


LIYL_NAMESPACE_END
#endif  // LIYL_SKIP_CUT_BUFFER_H_ 
