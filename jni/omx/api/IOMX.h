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

#ifndef LIYL_IOMX_H_

#define LIYL_IOMX_H_

#include "../../foundation/system/core/utils/List.h"
#include "../../foundation/media/include/AString.h"
#include "../../foundation/media/include/ABuffer.h"
#include "../../common/include/IInterface.h"

#include <list>


#include "../../prebuild/openmax/OMX_Core.h"
#include "../../prebuild/openmax/OMX_Video.h"
#include "../../prebuild/openmax/OMX_Audio.h"

LIYL_NAMESPACE_START

class IOMXObserver;

class IOMX : public IInterface {
public:

    typedef uint32_t buffer_id;
    typedef uint32_t node_id;

	LIYL_DECLARE_META_INTERFACE(OMX);

    // Given a node_id and the calling process' pid, returns true iff
    // the implementation of the OMX interface lives in the same
    // process.
    virtual bool livesLocally(node_id node, pid_t pid) = 0;

    struct ComponentInfo {
        AString mName;
        List<AString> mRoles;
    };
    virtual status_t listNodes(List<ComponentInfo> *list) = 0;

    virtual status_t allocateNode(
            const char *name, const sp<IOMXObserver> &observer,
            node_id *node) = 0;

    virtual status_t freeNode(node_id node) = 0;

    virtual status_t sendCommand(
            node_id node, OMX_COMMANDTYPE cmd, OMX_S32 param) = 0;

    virtual status_t getParameter(
            node_id node, OMX_INDEXTYPE index,
            void *params, size_t size) = 0;

    virtual status_t setParameter(
            node_id node, OMX_INDEXTYPE index,
            const void *params, size_t size) = 0;

    virtual status_t getConfig(
            node_id node, OMX_INDEXTYPE index,
            void *params, size_t size) = 0;

    virtual status_t setConfig(
            node_id node, OMX_INDEXTYPE index,
            const void *params, size_t size) = 0;

    virtual status_t getState(
            node_id node, OMX_STATETYPE* state) = 0;

    virtual status_t prepareForAdaptivePlayback(
            node_id node, OMX_U32 portIndex, OMX_BOOL enable,
            OMX_U32 maxFrameWidth, OMX_U32 maxFrameHeight) = 0;

       // Use |params| as an OMX buffer, but limit the size of the OMX buffer to |allottedSize|.
    virtual status_t useBuffer(
            node_id node, OMX_U32 port_index, const sp<ABuffer> &params,
            buffer_id *buffer, OMX_U32 allottedSize) = 0;

     virtual status_t signalEndOfInputStream(node_id node) = 0;

    // Allocate an OMX buffer of size |allotedSize|. Use |params| as the backup buffer, which
    // may be larger.
    virtual status_t allocateBufferWithBackup(
            node_id node, OMX_U32 port_index, const sp<ABuffer> &params,
            buffer_id *buffer, OMX_U32 allottedSize) = 0;

    virtual status_t freeBuffer(
            node_id node, OMX_U32 port_index, buffer_id buffer) = 0;

    enum {
        kFenceTimeoutMs = 1000
    };
    // Calls OMX_FillBuffer on buffer, and passes |fenceFd| to component if it supports
    // fences. Otherwise, it waits on |fenceFd| before calling OMX_FillBuffer.
    // Takes ownership of |fenceFd| even if this call fails.
    virtual status_t fillBuffer(node_id node, buffer_id buffer, int fenceFd = -1) = 0;

    // Calls OMX_EmptyBuffer on buffer (after updating buffer header with |range_offset|,
    // |range_length|, |flags| and |timestamp|). Passes |fenceFd| to component if it
    // supports fences. Otherwise, it waits on |fenceFd| before calling OMX_EmptyBuffer.
    // Takes ownership of |fenceFd| even if this call fails.
    virtual status_t emptyBuffer(
            node_id node,
            buffer_id buffer,
            OMX_U32 range_offset, OMX_U32 range_length,
            OMX_U32 flags, OMX_TICKS timestamp, int fenceFd = -1) = 0;

    virtual status_t getExtensionIndex(
            node_id node,
            const char *parameter_name,
            OMX_INDEXTYPE *index) = 0;


};

struct omx_message {
    enum {
        EVENT,
        EMPTY_BUFFER_DONE,
        FILL_BUFFER_DONE,
        FRAME_RENDERED,
    } type;

    IOMX::node_id node;
    int fenceFd; // used for EMPTY_BUFFER_DONE and FILL_BUFFER_DONE; client must close this

    union {
        // if type == EVENT
        struct {
            OMX_EVENTTYPE event;
            OMX_U32 data1;
            OMX_U32 data2;
        } event_data;

        // if type == EMPTY_BUFFER_DONE
        struct {
            IOMX::buffer_id buffer;
        } buffer_data;

        // if type == FILL_BUFFER_DONE
        struct {
            IOMX::buffer_id buffer;
            OMX_U32 range_offset;
            OMX_U32 range_length;
            OMX_U32 flags;
            OMX_TICKS timestamp;
        } extended_buffer_data;

    } u;
};

class IOMXObserver : public IInterface {
public:
    LIYL_DECLARE_META_INTERFACE(OMXObserver);

    // Handle (list of) messages.
    virtual void onMessages(const std::list<omx_message> &messages) = 0;
};


struct CodecProfileLevel {
    OMX_U32 mProfile;
    OMX_U32 mLevel;
};


LIYL_NAMESPACE_END

#endif  // LIYL_IOMX_H_
