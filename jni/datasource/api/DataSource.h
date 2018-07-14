/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef LIYL_DATA_SOURCE_H_

#define LIYL_DATA_SOURCE_H_

#include "../../foundation/system/core/types.h"
#include "../../foundation/media/include/ADebug.h"
#include "../../foundation/media/include/AMessage.h"
#include "../../common/include/MediaErrors.h"
#include "../../foundation/system/core/utils/List.h"
#include "../../foundation/system/core/threads.h"
#include "IDataSource.h"

LIYL_NAMESPACE_START


class DataSource : public IDataSource {
public:
    enum Flags {
        kWantsPrefetching      = 1,
        kStreamedFromLocalHost = 2,
        kIsCachingDataSource   = 4,
        kIsHTTPBasedSource     = 8,
    };

    DataSource() {}

	virtual void open(const char* uri,const KeyedVector<AString,AString> *header = NULL,AString* contentType = NULL) {}

	virtual status_t connectAtOffset(off64_t offset,size_t size){ return ERROR_UNSUPPORTED; } 

    // Returns the number of bytes read, or -1 on failure. It's not an error if
    // this returns zero; it just means the given offset is equal to, or
    // beyond, the end of the source.
    virtual ssize_t readAt(off64_t offset, void *data, size_t size) = 0;

    virtual status_t disconnect() { return ERROR_UNSUPPORTED; }

    // Convenience methods:
    bool getUInt16(off64_t offset, uint16_t *x);
    bool getUInt24(off64_t offset, uint32_t *x); // 3 byte int, returned as a 32-bit int
    bool getUInt32(off64_t offset, uint32_t *x);
    bool getUInt64(off64_t offset, uint64_t *x);

    // May return ERROR_UNSUPPORTED.
    virtual status_t getSize(off64_t *size);

    virtual uint32_t getFlags() {
        return 0;
    }

    virtual AString toString() const {
        return AString("<unspecified>");
    }

    virtual AString getMIMEType() const;

    virtual void close() {}

protected:
    virtual ~DataSource() {}

private:

    DataSource(const DataSource &);
    DataSource &operator=(const DataSource &);
};

LIYL_NAMESPACE_END

#endif  // LIYL_DATA_SOURCE_H_
