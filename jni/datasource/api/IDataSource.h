/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef LIYL_IDATASOURCE_H
#define LIYL_IDATASOURCE_H

#include "../../common/include/IInterface.h"
#include "../../foundation/media/include/ABase.h"
#include "../../foundation/media/include/AString.h"
#include "../../foundation/system/core/utils/Errors.h"
#include "../../foundation/system/core/utils/KeyedVector.h"

LIYL_NAMESPACE_START


// A binder interface for implementing a stagefright DataSource remotely.
class IDataSource : public IInterface {
public:
    LIYL_DECLARE_META_INTERFACE(DataSource);


	//open handle
	virtual void open(const char* uri,const KeyedVector<AString,AString> *header = NULL,AString* contentType = NULL) = 0;

	//connect session
	virtual status_t connectAtOffset(off64_t offset,size_t size) = 0; 
   // Read up to |size| bytes into the memory returned by getIMemory(). Returns
    // the number of bytes read, or -1 on error. |size| must not be larger than
    // the buffer.
    virtual ssize_t readAt(off64_t offset,void* data, size_t size) = 0;
    // Get the size, or -1 if the size is unknown.
    virtual status_t getSize(off64_t* size) = 0;

	//disconnect session
	virtual status_t disconnect() = 0;
	//close handle
    virtual void close() = 0;
    // Get the flags of the source.
    // Refer to DataSource:Flags for the definition of the flags.
    virtual uint32_t getFlags() = 0;
    // get a description of the source, e.g. the url or filename it is based on
    virtual AString toString() const = 0;

private:
    DISALLOW_EVIL_CONSTRUCTORS(IDataSource);
};

LIYL_NAMESPACE_END

#endif // LIYL_IDATASOURCE_H
