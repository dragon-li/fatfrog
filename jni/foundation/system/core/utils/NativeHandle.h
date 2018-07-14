/*
 * Copyright 2014 The Android Open Source Project
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

#ifndef LIYL_NATIVE_HANDLE_H
#define LIYL_NATIVE_HANDLE_H

#include "RefBase.h"
#include "StrongPointer.h"

typedef struct native_handle native_handle_t;

LIYL_NAMESPACE_START

class NativeHandle: public LightRefBase<NativeHandle> {
public:
    // Create a refcounted wrapper around a native_handle_t, and declare
    // whether the wrapper owns the handle (so that it should clean up the
    // handle upon destruction) or not.
    // If handle is NULL, no NativeHandle will be created.
    static sp<NativeHandle> create(native_handle_t* handle, bool ownsHandle);

    const native_handle_t* handle() const {
        return mHandle;
    }

private:
    // for access to the destructor
    friend class LightRefBase<NativeHandle>;

    NativeHandle(native_handle_t* handle, bool ownsHandle);
    virtual ~NativeHandle();

    native_handle_t* mHandle;
    bool mOwnsHandle;

    // non-copyable
    NativeHandle(const NativeHandle&);
    NativeHandle& operator=(const NativeHandle&);
};

LIYL_NAMESPACE_END

#endif // LIYL_NATIVE_HANDLE_H
