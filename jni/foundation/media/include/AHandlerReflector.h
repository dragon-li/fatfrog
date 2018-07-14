/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef __LIYL_A_HANDLER_REFLECTOR_H_

#define __LIYL_A_HANDLER_REFLECTOR_H_

#include "AHandler.h"

LIYL_NAMESPACE_START

template<class T>
struct AHandlerReflector : public AHandler {
    AHandlerReflector(T *target)
        : mTarget(target) {
    }

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg) {
        sp<T> target = mTarget.promote();
        if (target != NULL) {
            target->onMessageReceived(msg);
        }
    }

private:
    wp<T> mTarget;

    AHandlerReflector(const AHandlerReflector<T> &);
    AHandlerReflector<T> &operator=(const AHandlerReflector<T> &);
};

LIYL_NAMESPACE_END

#endif  // __LIYL_A_HANDLER_REFLECTOR_H_
