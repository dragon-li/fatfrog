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

//#define LOG_NDEBUG 0
#define LOG_TAG "SkipCutBuffer"
#include "../../foundation/system/core/utils/Log.h"

#include "../../foundation/media/include/ADebug.h"
#include "SkipCutBuffer.h"

LIYL_NAMESPACE_START

SkipCutBuffer::SkipCutBuffer(size_t skip, size_t cut, size_t num16BitChannels) {

    mWriteHead = 0;
    mReadHead = 0;
    mCapacity = 0;
    mCutBuffer = NULL;

    if (num16BitChannels == 0 || num16BitChannels > INT32_MAX / 2) {
        LLOGW("# channels out of range: %zu, using passthrough instead", num16BitChannels);
        return;
    }
    size_t frameSize = num16BitChannels * 2;
    if (skip > INT32_MAX / frameSize || cut > INT32_MAX / frameSize
            || cut * frameSize > INT32_MAX - 4096) {
        LLOGW("out of range skip/cut: %zu/%zu, using passthrough instead",
                skip, cut);
        return;
    }
    skip *= frameSize;
    cut *= frameSize;

    mFrontPadding = mSkip = skip;
    mBackPadding = cut;
    mCapacity = cut + 4096;
    mCutBuffer = new (std::nothrow) char[mCapacity];
    LLOGV("skipcutbuffer %zu %zu %d", skip, cut, mCapacity);
}

SkipCutBuffer::~SkipCutBuffer() {
    delete[] mCutBuffer;
}

void SkipCutBuffer::submit(const sp<ABuffer>& buffer) {
    if (mCutBuffer == NULL) {
        // passthrough mode
        return;
    }

    int32_t offset = buffer->offset();
    int32_t buflen = buffer->size();

    // drop the initial data from the buffer if needed
    if (mFrontPadding > 0) {
        // still data left to drop
        int32_t to_drop = (buflen < mFrontPadding) ? buflen : mFrontPadding;
        offset += to_drop;
        buflen -= to_drop;
        buffer->setRange(offset, buflen);
        mFrontPadding -= to_drop;
    }


    // append data to cutbuffer
    char *src = (char*) buffer->data();
    write(src, buflen);


    // the mediabuffer is now empty. Fill it from cutbuffer, always leaving
    // at least mBackPadding bytes in the cutbuffer
    char *dst = (char*) buffer->base();
    size_t copied = read(dst, buffer->capacity());
    buffer->setRange(0, copied);
}

void SkipCutBuffer::clear() {
    mWriteHead = mReadHead = 0;
    mFrontPadding = mSkip;
}

void SkipCutBuffer::write(const char *src, size_t num) {
    int32_t sizeused = (mWriteHead - mReadHead);
    if (sizeused < 0) sizeused += mCapacity;

    // Everything must fit. Make sure the buffer is a little larger than needed,
    // so there is no ambiguity as to whether mWriteHead == mReadHead means buffer
    // full or empty
    size_t available = mCapacity - sizeused - 32;
    if (available < num) {
        int32_t newcapacity = mCapacity + (num - available);
        char * newbuffer = new char[newcapacity];
        memcpy(newbuffer, mCutBuffer, mCapacity);
        delete [] mCutBuffer;
        mCapacity = newcapacity;
        mCutBuffer = newbuffer;
        LLOGV("reallocated buffer at size %d", newcapacity);
    }

    size_t copyfirst = (mCapacity - mWriteHead);
    if (copyfirst > num) copyfirst = num;
    if (copyfirst) {
        memcpy(mCutBuffer + mWriteHead, src, copyfirst);
        num -= copyfirst;
        src += copyfirst;
        mWriteHead += copyfirst;
        CHECK_LE(mWriteHead, mCapacity);
        if (mWriteHead == mCapacity) mWriteHead = 0;
        if (num) {
            memcpy(mCutBuffer, src, num);
            mWriteHead += num;
        }
    }
}

size_t SkipCutBuffer::read(char *dst, size_t num) {
    int32_t available = (mWriteHead - mReadHead);
    if (available < 0) available += mCapacity;

    available -= mBackPadding;
    if (available <=0) {
        return 0;
    }
    if (available < int32_t(num)) {
        num = available;
    }

    size_t copyfirst = (mCapacity - mReadHead);
    if (copyfirst > num) copyfirst = num;
    if (copyfirst) {
        memcpy(dst, mCutBuffer + mReadHead, copyfirst);
        num -= copyfirst;
        dst += copyfirst;
        mReadHead += copyfirst;
        CHECK_LE(mReadHead, mCapacity);
        if (mReadHead == mCapacity) mReadHead = 0;
        if (num) {
            memcpy(dst, mCutBuffer, num);
            mReadHead += num;
        }
    }
    return available;
}

size_t SkipCutBuffer::size() {
    int32_t available = (mWriteHead - mReadHead);
    if (available < 0) available += mCapacity;
    return available;
}

LIYL_NAMESPACE_END
