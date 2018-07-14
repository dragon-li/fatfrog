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

#ifndef LIYL_OMX_H_
#define LIYL_OMX_H_

#include "api/IOMX.h"
#include "../foundation/system/core/threads.h"
#include "../foundation/system/core/utils/KeyedVector.h"

LIYL_NAMESPACE_START

struct OMXMaster;
struct OMXNodeInstance;

class OMX : public IOMX {
public:
    OMX();

    virtual bool livesLocally(node_id node, pid_t pid);

    virtual status_t listNodes(List<ComponentInfo> *list);

    virtual status_t allocateNode(
            const char *name, const sp<IOMXObserver> &observer,
            node_id *node);

    virtual status_t freeNode(node_id node);

    virtual status_t sendCommand(
            node_id node, OMX_COMMANDTYPE cmd, OMX_S32 param);

    virtual status_t getParameter(
            node_id node, OMX_INDEXTYPE index,
            void *params, size_t size);

    virtual status_t setParameter(
            node_id node, OMX_INDEXTYPE index,
            const void *params, size_t size);

    virtual status_t getConfig(
            node_id node, OMX_INDEXTYPE index,
            void *params, size_t size);

    virtual status_t setConfig(
            node_id node, OMX_INDEXTYPE index,
            const void *params, size_t size);

    virtual status_t getState(
            node_id node, OMX_STATETYPE* state);


    virtual status_t prepareForAdaptivePlayback(
            node_id node, OMX_U32 portIndex, OMX_BOOL enable,
            OMX_U32 max_frame_width, OMX_U32 max_frame_height);


    virtual status_t useBuffer(
            node_id node, OMX_U32 port_index, const sp<ABuffer> &params,
            buffer_id *buffer, OMX_U32 allottedSize);

    virtual status_t signalEndOfInputStream(node_id node);

    virtual status_t allocateBufferWithBackup(
            node_id node, OMX_U32 port_index, const sp<ABuffer> &params,
            buffer_id *buffer, OMX_U32 allottedSize);

    virtual status_t freeBuffer(
            node_id node, OMX_U32 port_index, buffer_id buffer);

    virtual status_t fillBuffer(node_id node, buffer_id buffer, int fenceFd);

    virtual status_t emptyBuffer(
            node_id node,
            buffer_id buffer,
            OMX_U32 range_offset, OMX_U32 range_length,
            OMX_U32 flags, OMX_TICKS timestamp, int fenceFd);

    virtual status_t getExtensionIndex(
            node_id node,
            const char *parameter_name,
            OMX_INDEXTYPE *index);


    OMX_ERRORTYPE OnEvent(
            node_id node,
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData);

    OMX_ERRORTYPE OnEmptyBufferDone(
            node_id node, buffer_id buffer, OMX_IN OMX_BUFFERHEADERTYPE *pBuffer, int fenceFd);

    OMX_ERRORTYPE OnFillBufferDone(
            node_id node, buffer_id buffer, OMX_IN OMX_BUFFERHEADERTYPE *pBuffer, int fenceFd);

    void invalidateNodeID(node_id node);

protected:
    virtual ~OMX();

private:
    struct CallbackDispatcherThread;
    struct CallbackDispatcher;

    Mutex mLock;
    OMXMaster *mMaster;
    size_t mNodeCounter;

    KeyedVector<node_id, OMXNodeInstance *> mNodeIDToInstance;
    KeyedVector<node_id, sp<CallbackDispatcher> > mDispatchers;

    node_id makeNodeID_l(OMXNodeInstance *instance);
    OMXNodeInstance *findInstance(node_id node);
    sp<CallbackDispatcher> findDispatcher(node_id node);

    void invalidateNodeID_l(node_id node);

    DISALLOW_EVIL_CONSTRUCTORS(OMX);
};

LIYL_NAMESPACE_END
#endif  // LIYL_OMX_H_
