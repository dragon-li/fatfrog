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

//#define LOG_NDEBUG 0
#define LOG_TAG "ALooperRoster"
#include "../../system/core/utils/Log.h"

#include "../include/ALooperRoster.h"

#include "../include/ADebug.h"
#include "../include/AHandler.h"
#include "../include/AMessage.h"
LIYL_NAMESPACE_START

    static bool verboseStats = false;

    ALooperRoster::ALooperRoster()
        : mNextHandlerID(1) {
        }

    ALooper::handler_id ALooperRoster::registerHandler(
            const sp<ALooper> looper, const sp<AHandler> &handler) {
        Mutex::Autolock autoLock(mLock);

        if (handler->id() != 0) {
            CHECK(!"A handler must only be registered once.");
            return INVALID_OPERATION;
        }

        HandlerInfo info;
        info.mLooper = looper;
        info.mHandler = handler;
        ALooper::handler_id handlerID = mNextHandlerID++;
        std::pair<tHandlerInfoMap::iterator,bool> ret = mHandlers.insert(std::pair<ALooper::handler_id,HandlerInfo>(handlerID,info));
        if (ret.second == false) {
            LLOGE("insert failed,the element already exist:with a value of %p",&(ret.first->second));
        }
        handler->setID(handlerID, looper);

        return handlerID;
    }

    void ALooperRoster::unregisterHandler(ALooper::handler_id handlerID) {
        Mutex::Autolock autoLock(mLock);
        /*is not an element of map*/
        if(mHandlers.count(handlerID) <= 0) {
            return;
        }

        const HandlerInfo &info = mHandlers[handlerID];

        sp<AHandler> handler = info.mHandler.promote();

        if (handler != NULL) {
            handler->setID(0, NULL);
        }

        mHandlers.erase(handlerID);
    }

    void ALooperRoster::unregisterStaleHandlers() {

        //TODO FIXME vector replace with list
        std::list<sp<ALooper> > activeLoopers;
        {
            Mutex::Autolock autoLock(mLock);


            for(tHandlerInfoMap::iterator it  = mHandlers.begin();it != mHandlers.end();it++) {
                const HandlerInfo &info = it->second;

                sp<ALooper> looper = info.mLooper.promote();
                if (looper == NULL) {
                    LLOGV("Unregistering stale handler %d", (int32_t)(it->first));
                    mHandlers.erase(it);
                } else {
                    // At this point 'looper' might be the only sp<> keeping
                    // the object alive. To prevent it from going out of scope
                    // and having ~ALooper call this method again recursively
                    // and then deadlocking because of the Autolock above, add
                    // it to a Vector which will go out of scope after the lock
                    // has been released.
                    activeLoopers.push_back(looper);
                }
            }



        }
    }


    static void makeFourCC(uint32_t fourcc, char *s) {
        s[0] = (fourcc >> 24) & 0xff;
        if (s[0]) {
            s[1] = (fourcc >> 16) & 0xff;
            s[2] = (fourcc >> 8) & 0xff;
            s[3] = fourcc & 0xff;
            s[4] = 0;
        } else {
            sprintf(s, "%u", fourcc);
        }
    }

    void ALooperRoster::dump(int fd, const std::list<std::string>& args) {
        bool clear = false;
        bool oldVerbose = verboseStats;
        for(std::list<std::string>::const_iterator it  = args.cbegin();it != args.cend();it++) {
            const std::string& str = *it; 
            if(str.compare(std::string("-c")) == 0) {
                clear = true;
            }else if (str.compare(std::string("-von")) == 0) {
                verboseStats = true;
            }else if (str.compare(std::string("-voff")) == 0) {
                verboseStats = false;
            }

        }
        std::string s;
        if (verboseStats && !oldVerbose) {
            s.append("(verbose stats collection enabled, stats will be cleared)\n");
        }

        Mutex::Autolock autoLock(mLock);

        char formatBuf[512] = {0};
        int32_t i_offset = 0;
        size_t n = mHandlers.size();

        i_offset = snprintf(formatBuf,sizeof(formatBuf) - 1," %zu registered handlers:\n", n);

        for(tHandlerInfoMap::iterator it  = mHandlers.begin();it != mHandlers.end();it++) {
            i_offset = snprintf(formatBuf+i_offset,sizeof(formatBuf) - 1-i_offset,"  %d: ", it->first);
            HandlerInfo &info = it->second;
            sp<ALooper> looper = info.mLooper.promote();
            if (looper != NULL) {
                s.append(looper->getName());
                sp<AHandler> handler = info.mHandler.promote();
                if (handler != NULL) {
                    handler->mVerboseStats = verboseStats;
                    i_offset = snprintf(formatBuf+i_offset,sizeof(formatBuf) - 1-i_offset,": %u messages processed", handler->mMessageCounter);
                    if (verboseStats) {
                        for(std::map<uint32_t,uint32_t>::iterator it  = handler->mMessages.begin();it != handler->mMessages.end();it++) {
                            char fourcc[15];
                            makeFourCC(it->second, fourcc);
                            i_offset = snprintf(formatBuf+i_offset,sizeof(formatBuf) - 1-i_offset,"\n    %s: %u",
                                    fourcc, it->second);
                        }
                    } else {
                        handler->mMessages.clear();
                    }
                    if (clear || (verboseStats && !oldVerbose)) {
                        handler->mMessageCounter = 0;
                        handler->mMessages.clear();
                    }
                } else {
                    s.append(": <stale handler>");
                }
            } else {
                s.append("<stale>");
            }
            s.append("\n");
        }
        s = s + formatBuf;
        LLOGV("Dump: %s",s.c_str());
        write(fd, s.c_str(), s.size());
    }

LIYL_NAMESPACE_END
