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

#ifndef OMX_NODE_INSTANCE_H_

#define OMX_NODE_INSTANCE_H_

#include "OMX.h"

#include "../foundation/system/core/utils/RefBase.h"
#include "../foundation/system/core/utils/SortedVector.h"
#include "../foundation/system/core/threads.h"
#include "../foundation/media/include/ABuffer.h"

LIYL_NAMESPACE_START

class IOMXObserver;
struct OMXMaster;

struct OMXNodeInstance {
    OMXNodeInstance(
            OMX *owner, const sp<IOMXObserver> &observer, const char *name);

    void setHandle(OMX::node_id node_id, OMX_HANDLETYPE handle);

    OMX *owner();
    sp<IOMXObserver> observer();
    OMX::node_id nodeID();

    status_t freeNode(OMXMaster *master);

    status_t sendCommand(OMX_COMMANDTYPE cmd, OMX_S32 param);
    status_t getParameter(OMX_INDEXTYPE index, void *params, size_t size);

    status_t setParameter(
            OMX_INDEXTYPE index, const void *params, size_t size);

    status_t getConfig(OMX_INDEXTYPE index, void *params, size_t size);
    status_t setConfig(OMX_INDEXTYPE index, const void *params, size_t size);

    status_t getState(OMX_STATETYPE* state);

    status_t prepareForAdaptivePlayback(
            OMX_U32 portIndex, OMX_BOOL enable,
            OMX_U32 maxFrameWidth, OMX_U32 maxFrameHeight);

    status_t useBuffer(
            OMX_U32 portIndex, const sp<ABuffer> &params,
            OMX::buffer_id *buffer, OMX_U32 allottedSize);

    status_t signalEndOfInputStream();

    void signalEvent(OMX_EVENTTYPE event, OMX_U32 arg1, OMX_U32 arg2);

    status_t allocateBufferWithBackup(
            OMX_U32 portIndex, const sp<ABuffer> &params,
            OMX::buffer_id *buffer, OMX_U32 allottedSize);

    status_t freeBuffer(OMX_U32 portIndex, OMX::buffer_id buffer);

    status_t fillBuffer(OMX::buffer_id buffer, int fenceFd);

    status_t emptyBuffer(
            OMX::buffer_id buffer,
            OMX_U32 rangeOffset, OMX_U32 rangeLength,
            OMX_U32 flags, OMX_TICKS timestamp, int fenceFd);


    status_t getExtensionIndex(
            const char *parameterName, OMX_INDEXTYPE *index);

    // handles messages and removes them from the list
    void onMessages(std::list<omx_message> &messages);
    void onMessage(const omx_message &msg);
    void onObserverDied(OMXMaster *master);
    void onGetHandleFailed();
    void onEvent(OMX_EVENTTYPE event, OMX_U32 arg1, OMX_U32 arg2);

    static OMX_CALLBACKTYPE kCallbacks;

private:
    Mutex mLock;

    OMX *mOwner;
    OMX::node_id mNodeID;
    OMX_HANDLETYPE mHandle;
    sp<IOMXObserver> mObserver;
    bool mDying;
    bool mSailed;  // configuration is set (no more meta-mode changes)
    bool mQueriedProhibitedExtensions;
    SortedVector<OMX_INDEXTYPE> mProhibitedExtensions;

    struct ActiveBuffer {
        OMX_U32 mPortIndex;
        OMX::buffer_id mID;
    };
    Vector<ActiveBuffer> mActiveBuffers;
    // for buffer ptr to buffer id translation
    Mutex mBufferIDLock;
    uint32_t mBufferIDCount;
    KeyedVector<OMX::buffer_id, OMX_BUFFERHEADERTYPE *> mBufferIDToBufferHeader;
    KeyedVector<OMX_BUFFERHEADERTYPE *, OMX::buffer_id> mBufferHeaderToBufferID;

	// For debug support
	char *mName;
	int DEBUG;
	size_t mNumPortBuffers[2]; //modify under mLock,read outside for debug

    ~OMXNodeInstance();

    void addActiveBuffer(OMX_U32 portIndex, OMX::buffer_id id);
    void removeActiveBuffer(OMX_U32 portIndex, OMX::buffer_id id);
    void freeActiveBuffers();

    // For buffer id management
    OMX::buffer_id makeBufferID(OMX_BUFFERHEADERTYPE *bufferHeader);
    OMX_BUFFERHEADERTYPE *findBufferHeader(OMX::buffer_id buffer, OMX_U32 portIndex);
    OMX::buffer_id findBufferID(OMX_BUFFERHEADERTYPE *bufferHeader);
    void invalidateBufferID(OMX::buffer_id buffer);

    bool isProhibitedIndex_l(OMX_INDEXTYPE index);

   static OMX_ERRORTYPE OnEvent(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData);

    static OMX_ERRORTYPE OnEmptyBufferDone(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);

    static OMX_ERRORTYPE OnFillBufferDone(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_BUFFERHEADERTYPE *pBuffer);

    status_t emptyBuffer_l(
            OMX_BUFFERHEADERTYPE *header,
            OMX_U32 flags, OMX_TICKS timestamp, intptr_t debugAddr, int fenceFd);

   // Handles |msg|, and may modify it. Returns true iff completely handled it and
    // |msg| does not need to be sent to the event listener.
    bool handleMessage(omx_message &msg);

    OMXNodeInstance(const OMXNodeInstance &);
    OMXNodeInstance &operator=(const OMXNodeInstance &);
};

LIYL_NAMESPACE_END

#endif  // OMX_NODE_INSTANCE_H_
