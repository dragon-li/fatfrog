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

#ifndef __LIYL_A_LOOPER_ROSTER_H_

#define __LIYL_A_LOOPER_ROSTER_H_

#include "../../system/core/utils/KeyedVector.h"
#include "ALooper.h"

LIYL_NAMESPACE_START

struct ALooperRoster {
    ALooperRoster();

    ALooper::handler_id registerHandler(
            const sp<ALooper> looper, const sp<AHandler> &handler);

    void unregisterHandler(ALooper::handler_id handlerID);
    void unregisterStaleHandlers();

    void dump(int fd, const std::list<std::string>& args);

    //liyl add
    static ALooperRoster* getLocalLooperRoster();
private:
    struct HandlerInfo {
        wp<ALooper> mLooper;
        wp<AHandler> mHandler;
    };

    Mutex mLock;
    KeyedVector<ALooper::handler_id, HandlerInfo> mHandlers;
    ALooper::handler_id mNextHandlerID;

    DISALLOW_EVIL_CONSTRUCTORS(ALooperRoster);
};

LIYL_NAMESPACE_END

#endif  // __LIYL_A_LOOPER_ROSTER_H_
