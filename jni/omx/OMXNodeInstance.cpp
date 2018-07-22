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

//#define LOG_NDEBUG 0
#define LOG_TAG "OMXNodeInstance"
#include "../foundation/system/core/utils/Log.h"

#include <inttypes.h>

#include "OMXNodeInstance.h"
#include "OMXMaster.h"
#include "OMXUtils.h"

#include "../prebuild/openmax/OMX_Component.h"
#include "../prebuild/openmax/OMX_IndexExt.h"
#include "../prebuild/openmax/OMX_AsString.h"

#include "../foundation/media/include/ADebug.h"
#include "../foundation/media/include/ABuffer.h"
#include "../common/include/MediaErrors.h"
#include "../common/include/HardwareAPI.h"
#include "../foundation/system/core/utils/NativeHandle.h"

static const OMX_U32 kPortIndexInput = 0;
static const OMX_U32 kPortIndexOutput = 1;

enum Level { 
	kDebugNone,             // no debug 
	kDebugLifeCycle,        // lifecycle events: creation/deletion 
	kDebugState,            // commands and events 
	kDebugConfig,           // configuration 
	kDebugInternalState,    // internal state changes 
	kDebugAll,              // all 
	kDebugMax = kDebugAll, 

};

#undef NELEM
#define NELEM(x)            (sizeof(x)/sizeof(*(x)))

#define CLOGW(fmt, ...) LLOGW("[%x:%s] " fmt, mNodeID, mName, ##__VA_ARGS__)

#define CLOG_ERROR_IF(cond, fn, err, fmt, ...) \
    LLOGE_IF(cond, #fn "(%x:%s, " fmt ") ERROR: %s(%#x)", \
            mNodeID, mName, ##__VA_ARGS__, asString(err), err)
#define CLOG_ERROR(fn, err, fmt, ...) CLOG_ERROR_IF(true, fn, err, fmt, ##__VA_ARGS__)
#define CLOG_IF_ERROR(fn, err, fmt, ...) \
    CLOG_ERROR_IF((err) != OMX_ErrorNone, fn, err, fmt, ##__VA_ARGS__)

#define CLOGI_(level, fn, fmt, ...) \
    LLOGI_IF(DEBUG >= (level), #fn "(%x:%s, " fmt ")", mNodeID, mName, ##__VA_ARGS__)
#define CLOGD_(level, fn, fmt, ...) \
    LLOGD_IF(DEBUG >= (level), #fn "(%x:%s, " fmt ")", mNodeID, mName, ##__VA_ARGS__)

#define CLOG_LIFE(fn, fmt, ...)     CLOGI_(kDebugLifeCycle,     fn, fmt, ##__VA_ARGS__)
#define CLOG_STATE(fn, fmt, ...)    CLOGI_(kDebugState,         fn, fmt, ##__VA_ARGS__)
#define CLOG_CONFIG(fn, fmt, ...)   CLOGI_(kDebugConfig,        fn, fmt, ##__VA_ARGS__)
#define CLOG_INTERNAL(fn, fmt, ...) CLOGD_(kDebugInternalState, fn, fmt, ##__VA_ARGS__)

#define CLOG_DEBUG_IF(cond, fn, fmt, ...) \
    LLOGD_IF(cond, #fn "(%x, " fmt ")", mNodeID, ##__VA_ARGS__)

#define CLOG_BUFFER(fn, fmt, ...) \
    CLOG_DEBUG_IF(DEBUG >= kDebugAll, fn, fmt, ##__VA_ARGS__)

/* buffer formatting */
#define BUFFER_FMT(port, fmt, ...) "%s:%u " fmt, portString(port), (port), ##__VA_ARGS__
#define NEW_BUFFER_FMT(buffer_id, port, fmt, ...) \
    BUFFER_FMT(port, fmt ") (#%zu => %#x", ##__VA_ARGS__, mActiveBuffers.size(), (buffer_id))

#define SIMPLE_BUFFER(port, size, data) BUFFER_FMT(port, "%zu@%p", (size), (data))
#define SIMPLE_NEW_BUFFER(buffer_id, port, size, data) \
    NEW_BUFFER_FMT(buffer_id, port, "%zu@%p", (size), (data))

#define EMPTY_BUFFER(addr, header, fenceFd) "%#x [%u@%p fc=%d]", \
    (addr), (header)->nAllocLen, (header)->pBuffer, (fenceFd)
#define FULL_BUFFER(addr, header, fenceFd) "%#" PRIxPTR " [%u@%p (%u..+%u) f=%x ts=%lld fc=%d]", \
    (intptr_t)(addr), (header)->nAllocLen, (header)->pBuffer, \
(header)->nOffset, (header)->nFilledLen, (header)->nFlags, (header)->nTimeStamp, (fenceFd)

#define WITH_STATS_WRAPPER(fmt, ...) fmt " { IN=%zu/%zu OUT=%zu/%zu }", ##__VA_ARGS__, \
    mInputBuffersWithCodec.size(), mNumPortBuffers[kPortIndexInput], \
mOutputBuffersWithCodec.size(), mNumPortBuffers[kPortIndexOutput]
// TRICKY: this is needed so formatting macros expand before substitution
#define WITH_STATS(fmt, ...) WITH_STATS_WRAPPER(fmt, ##__VA_ARGS__)

LIYL_NAMESPACE_START

struct BufferMeta {
    BufferMeta(
            const sp<ABuffer> &mem, OMX_U32 portIndex, bool copyToOmx,
            bool copyFromOmx)
        : mMem(mem),
        mCopyFromOmx(copyFromOmx),
        mCopyToOmx(copyToOmx),
        mPortIndex(portIndex){
        }

    BufferMeta(size_t size, OMX_U32 portIndex)
        : mSize(size),
        mCopyFromOmx(false),
        mCopyToOmx(false),
        mPortIndex(portIndex){
        }

    void CopyFromOMX(const OMX_BUFFERHEADERTYPE *header) {
        if (!mCopyFromOmx) {
            return;
        }

        // check component returns proper range
        sp<ABuffer> codec = getBuffer(header, true /* limit */);

        memcpy((OMX_U8*)(mMem->data()), codec->data(), codec->size());
    }

    void CopyToOMX(const OMX_BUFFERHEADERTYPE *header) {
        if (!mCopyToOmx) {
            return;
        }

        memcpy(header->pBuffer + header->nOffset, mMem->data(), mMem->size());
    }

    // return either the codec or the backup buffer
    sp<ABuffer> getBuffer(const OMX_BUFFERHEADERTYPE *header, bool limit) {
        sp<ABuffer> buf;
        buf = new ABuffer(header->pBuffer, header->nAllocLen);
        if (limit) {
            if (header->nOffset + header->nFilledLen > header->nOffset
                    && header->nOffset + header->nFilledLen <= header->nAllocLen) {
                buf->setRange(header->nOffset, header->nFilledLen);
            } else {
                buf->setRange(0, 0);
            }
        }
        return buf;
    }


    OMX_U32 getPortIndex() {
        return mPortIndex;
    }

    ~BufferMeta() {
    }

    private:
    sp<ABuffer> mMem;
    size_t mSize;
    bool mCopyFromOmx;
    bool mCopyToOmx;
    OMX_U32 mPortIndex;

    BufferMeta(const BufferMeta &);
    BufferMeta &operator=(const BufferMeta &);
};

// static
OMX_CALLBACKTYPE OMXNodeInstance::kCallbacks = {
    &OnEvent, &OnEmptyBufferDone, &OnFillBufferDone
};

static inline const char *portString(OMX_U32 portIndex) {
    switch (portIndex) {
        case kPortIndexInput:  return "Input";
        case kPortIndexOutput: return "Output";
        case ~0U:              return "All";
        default:               return "port";
    }
}

OMXNodeInstance::OMXNodeInstance(
        OMX *owner, const sp<IOMXObserver> &observer, const char *name)
    : mOwner(owner),
    mNodeID(0),
    mHandle(NULL),
    mObserver(observer),
    mDying(false),
    mSailed(false),
    mQueriedProhibitedExtensions(false),
    mBufferIDCount(0)
{
	mName = strdup(name);
    mNumPortBuffers[0] = 0;
    mNumPortBuffers[1] = 0;
}

OMXNodeInstance::~OMXNodeInstance() {
    free(mName);
    CHECK(mHandle == NULL);
}

void OMXNodeInstance::setHandle(OMX::node_id node_id, OMX_HANDLETYPE handle) {
    mNodeID = node_id;
    CLOG_LIFE(allocateNode, "handle=%p", handle);
    CHECK(mHandle == NULL);
    mHandle = handle;
}

OMX* OMXNodeInstance::owner() {
    return mOwner;
}

sp<IOMXObserver> OMXNodeInstance::observer() {
    return mObserver;
}

OMX::node_id OMXNodeInstance::nodeID() {
    return mNodeID;
}

status_t OMXNodeInstance::freeNode(OMXMaster *master) {
    CLOG_LIFE(freeNode, "handle=%p", mHandle);
    static int32_t kMaxNumIterations = 10;

    // exit if we have already freed the node
    if (mHandle == NULL) {
        return OK;
    }

    // Transition the node from its current state all the way down
  // to "Loaded".
  // This ensures that all active buffers are properly freed even
  // for components that don't do this themselves on a call to
  // "FreeHandle".

  // The code below may trigger some more events to be dispatched
  // by the OMX component - we want to ignore them as our client
  // does not expect them.
    mDying = true;

    OMX_STATETYPE state;
    CHECK_EQ(OMX_GetState(mHandle, &state), OMX_ErrorNone);
    switch (state) {
        case OMX_StateExecuting:
            {
          LLOGV("forcing Executing->Idle");
          sendCommand(OMX_CommandStateSet, OMX_StateIdle);
          OMX_ERRORTYPE err;
          int32_t iteration = 0;
          while ((err = OMX_GetState(mHandle, &state)) == OMX_ErrorNone
                  && state != OMX_StateIdle
                  && state != OMX_StateInvalid) {
              if (++iteration > kMaxNumIterations) {
                  CLOGW("failed to enter Idle state (now %s(%d), aborting.",
                          asString(state), state);
                  state = OMX_StateInvalid;
                  break;
              }

              usleep(100000);
          }
          CHECK_EQ(err, OMX_ErrorNone);

          if (state == OMX_StateInvalid) {
              break;
          }

          // fall through
      }

        case OMX_StateIdle:
            {
          LLOGV("forcing Idle->Loaded");
          sendCommand(OMX_CommandStateSet, OMX_StateLoaded);

          freeActiveBuffers();

          OMX_ERRORTYPE err;
          int32_t iteration = 0;
          while ((err = OMX_GetState(mHandle, &state)) == OMX_ErrorNone
                  && state != OMX_StateLoaded
                  && state != OMX_StateInvalid) {
              if (++iteration > kMaxNumIterations) {
                  CLOGW("failed to enter Loaded state (now %s(%d), aborting.",
                          asString(state), state);
                  state = OMX_StateInvalid;
                  break;
              }

              LLOGV("waiting for Loaded state...");
              usleep(100000);
          }
          CHECK_EQ(err, OMX_ErrorNone);

          // fall through
      }

        case OMX_StateLoaded:
        case OMX_StateInvalid:
            break;

        default:
            LOG_ALWAYS_FATAL("unknown state %s(%#x).", asString(state), state);
            break;
    }

    LLOGV("[%x:%s] calling destroyComponentInstance", mNodeID, mName);
    OMX_ERRORTYPE err = master->destroyComponentInstance(
            static_cast<OMX_COMPONENTTYPE *>(mHandle));

    mHandle = NULL;
    CLOG_IF_ERROR(freeNode, err, "");
    free(mName);
    mName = NULL;

    mOwner->invalidateNodeID(mNodeID);
    mNodeID = 0;

    LLOGV("OMXNodeInstance going away.");
    delete this;

    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::sendCommand(
        OMX_COMMANDTYPE cmd, OMX_S32 param) {
    if (cmd == OMX_CommandStateSet) {
        // There are no configurations past first StateSet command.
        mSailed = true;
    }

    Mutex::Autolock autoLock(mLock);

    const char *paramString =
        cmd == OMX_CommandStateSet ? asString((OMX_STATETYPE)param) : portString(param);
    CLOG_STATE(sendCommand, "%s(%d), %s(%d)", asString(cmd), cmd, paramString, param);
    OMX_ERRORTYPE err = OMX_SendCommand(mHandle, cmd, param, NULL);
    CLOG_IF_ERROR(sendCommand, err, "%s(%d), %s(%d)", asString(cmd), cmd, paramString, param);
    return StatusFromOMXError(err);
}

bool OMXNodeInstance::isProhibitedIndex_l(OMX_INDEXTYPE index) {
    // these extensions can only be used from OMXNodeInstance, not by clients directly.
    static const char *restricted_extensions[] = {
        "OMX.google.android.index.storeMetaDataInBuffers",
        "OMX.google.android.index.storeANWBufferInMetadata",
        "OMX.google.android.index.prepareForAdaptivePlayback",
        "OMX.google.android.index.configureVideoTunnelMode",
        "OMX.google.android.index.useAndroidNativeBuffer2",
        "OMX.google.android.index.useAndroidNativeBuffer",
        "OMX.google.android.index.enableAndroidNativeBuffers",
        "OMX.google.android.index.allocateNativeHandle",
        "OMX.google.android.index.getAndroidNativeBufferUsage",
    };

    if ((index > OMX_IndexComponentStartUnused && index <= OMX_IndexParamStandardComponentRole)
            || (index > OMX_IndexPortStartUnused && index <= OMX_IndexParamCompBufferSupplier)
            || (index > OMX_IndexAudioStartUnused && index <= OMX_IndexConfigAudioChannelVolume)
            || (index > OMX_IndexVideoStartUnused && index <= OMX_IndexConfigVideoNalSize)
            || (index > OMX_IndexCommonStartUnused
                && index <= OMX_IndexConfigCommonTransitionEffect)
            || (index > (OMX_INDEXTYPE)OMX_IndexExtAudioStartUnused
                && index <= (OMX_INDEXTYPE)OMX_IndexParamAudioProfileQuerySupported)
            || (index > (OMX_INDEXTYPE)OMX_IndexExtVideoStartUnused
                && index <= (OMX_INDEXTYPE)OMX_IndexConfigLiylVideoTemporalLayering)
            || (index > (OMX_INDEXTYPE)OMX_IndexExtOtherStartUnused
                && index <= (OMX_INDEXTYPE)OMX_IndexParamConsumerUsageBits)) {
        return false;
    }

    if (!mQueriedProhibitedExtensions) {
        for (size_t i = 0; i < NELEM(restricted_extensions); ++i) {
            OMX_INDEXTYPE ext;
            if (OMX_GetExtensionIndex(mHandle, (OMX_STRING)restricted_extensions[i], &ext) == OMX_ErrorNone) {
                mProhibitedExtensions.add(ext);
            }
        }
        mQueriedProhibitedExtensions = true;
    }

    return mProhibitedExtensions.indexOf(index) >= 0;
}

status_t OMXNodeInstance::getParameter(
        OMX_INDEXTYPE index, void *params, size_t /* size */) {
    Mutex::Autolock autoLock(mLock);

    if (isProhibitedIndex_l(index)) {
        LLOGE("0x534e4554, 29422020");
        return BAD_INDEX;
    }

    OMX_ERRORTYPE err = OMX_GetParameter(mHandle, index, params);
    OMX_INDEXEXTTYPE extIndex = (OMX_INDEXEXTTYPE)index;
    // some errors are expected for getParameter
    if (err != OMX_ErrorNoMore) {
        CLOG_IF_ERROR(getParameter, err, "%s(%#x)", asString(extIndex), index);
    }
    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::setParameter(
        OMX_INDEXTYPE index, const void *params, size_t size) {
    Mutex::Autolock autoLock(mLock);
    OMX_INDEXEXTTYPE extIndex = (OMX_INDEXEXTTYPE)index;
    CLOG_CONFIG(setParameter, "%s(%#x), %zu@%p)", asString(extIndex), index, size, params);

    if (isProhibitedIndex_l(index)) {
        LLOGE("0x534e4554, 29422020");
        return BAD_INDEX;
    }

    OMX_ERRORTYPE err = OMX_SetParameter(
            mHandle, index, const_cast<void *>(params));
    CLOG_IF_ERROR(setParameter, err, "%s(%#x)", asString(extIndex), index);
    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::getConfig(
        OMX_INDEXTYPE index, void *params, size_t /* size */) {
    Mutex::Autolock autoLock(mLock);

    if (isProhibitedIndex_l(index)) {
        LLOGE("0x534e4554, 29422020");
        return BAD_INDEX;
    }

    OMX_ERRORTYPE err = OMX_GetConfig(mHandle, index, params);
    OMX_INDEXEXTTYPE extIndex = (OMX_INDEXEXTTYPE)index;
    // some errors are expected for getConfig
    if (err != OMX_ErrorNoMore) {
        CLOG_IF_ERROR(getConfig, err, "%s(%#x)", asString(extIndex), index);
    }
    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::setConfig(
        OMX_INDEXTYPE index, const void *params, size_t size) {
    Mutex::Autolock autoLock(mLock);
    OMX_INDEXEXTTYPE extIndex = (OMX_INDEXEXTTYPE)index;
    CLOG_CONFIG(setConfig, "%s(%#x), %zu@%p)", asString(extIndex), index, size, params);

    if (isProhibitedIndex_l(index)) {
        LLOGE("0x534e4554, 29422020");
        return BAD_INDEX;
    }

    OMX_ERRORTYPE err = OMX_SetConfig(
            mHandle, index, const_cast<void *>(params));
    CLOG_IF_ERROR(setConfig, err, "%s(%#x)", asString(extIndex), index);
    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::getState(OMX_STATETYPE* state) {
    Mutex::Autolock autoLock(mLock);

    OMX_ERRORTYPE err = OMX_GetState(mHandle, state);
    CLOG_IF_ERROR(getState, err, "");
    return StatusFromOMXError(err);
}


status_t OMXNodeInstance::prepareForAdaptivePlayback(
        OMX_U32 portIndex, OMX_BOOL enable, OMX_U32 maxFrameWidth,
        OMX_U32 maxFrameHeight) {
    Mutex::Autolock autolock(mLock);
    if (mSailed) {
        LLOGE("0x534e4554, 29422020");
        return INVALID_OPERATION;
    }
    CLOG_CONFIG(prepareForAdaptivePlayback, "%s:%u en=%d max=%ux%u",
            portString(portIndex), portIndex, enable, maxFrameWidth, maxFrameHeight);

    OMX_INDEXTYPE index;
    OMX_STRING name = const_cast<OMX_STRING>(
            "OMX.google.android.index.prepareForAdaptivePlayback");

    OMX_ERRORTYPE err = OMX_GetExtensionIndex(mHandle, name, &index);
    if (err != OMX_ErrorNone) {
        CLOG_ERROR_IF(enable, getExtensionIndex, err, "%s", name);
        return StatusFromOMXError(err);
    }

    PrepareForAdaptivePlaybackParams params;
    InitOMXParams(&params);
    params.nPortIndex = portIndex;
    params.bEnable = enable;
    params.nMaxFrameWidth = maxFrameWidth;
    params.nMaxFrameHeight = maxFrameHeight;

    err = OMX_SetParameter(mHandle, index, &params);
    CLOG_IF_ERROR(setParameter, err, "%s(%#x): %s:%u en=%d max=%ux%u", name, index,
            portString(portIndex), portIndex, enable, maxFrameWidth, maxFrameHeight);
    return StatusFromOMXError(err);
}


status_t OMXNodeInstance::useBuffer(
        OMX_U32 portIndex, const sp<ABuffer> &params,
        OMX::buffer_id *buffer, OMX_U32 allottedSize) {
    if (params == NULL || buffer == NULL) {
        LLOGE("b/25884056");
        return BAD_VALUE;
    }

    Mutex::Autolock autoLock(mLock);
    if (allottedSize > params->size() || portIndex >= NELEM(mNumPortBuffers)) {
        return BAD_VALUE;
    }

    // metadata buffers are not connected cross process
    BufferMeta *buffer_meta;
    OMX_U8 *data = static_cast<OMX_U8 *>(params->data());
	buffer_meta = new BufferMeta( params, portIndex, false /* copyToOmx */, false /* copyFromOmx */);

    OMX_BUFFERHEADERTYPE *header;

    OMX_ERRORTYPE err = OMX_UseBuffer(
            mHandle, &header, portIndex, buffer_meta,
            allottedSize, data);

    if (err != OMX_ErrorNone) {
        CLOG_ERROR(useBuffer, err, SIMPLE_BUFFER(
                    portIndex, (size_t)allottedSize, data));

        delete buffer_meta;
        buffer_meta = NULL;

        *buffer = 0;

        return StatusFromOMXError(err);
    }

    CHECK_EQ(header->pAppPrivate, buffer_meta);

    *buffer = makeBufferID(header);

    addActiveBuffer(portIndex, *buffer);


    CLOG_BUFFER(useBuffer, NEW_BUFFER_FMT( *buffer, portIndex, "%u(%zu)@%p", allottedSize, params->size(), params->data()));
    return OK;
}


void OMXNodeInstance::signalEvent(OMX_EVENTTYPE event, OMX_U32 arg1, OMX_U32 arg2) {
    mOwner->OnEvent(mNodeID, event, arg1, arg2, NULL);
}

status_t OMXNodeInstance::signalEndOfInputStream() {
    // For non-Surface input, the MediaCodec should convert the call to a
  // pair of requests (dequeue input buffer, queue input buffer with EOS
  // flag set).  Seems easier than doing the equivalent from here.
    CLOGW("signalEndOfInputStream can only be used with Surface input");
    return INVALID_OPERATION;
}


status_t OMXNodeInstance::allocateBufferWithBackup(
        OMX_U32 portIndex, const sp<ABuffer> &params,
        OMX::buffer_id *buffer, OMX_U32 allottedSize) {
    if (params == NULL || buffer == NULL) {
        LLOGE("b/25884056");
        return BAD_VALUE;
    }

    Mutex::Autolock autoLock(mLock);
    if (allottedSize > params->size() || portIndex >= NELEM(mNumPortBuffers)) {
        return BAD_VALUE;
    }

	//TODO
    // metadata buffers are not connected cross process; only copy if not meta
    bool copy = true;

    BufferMeta *buffer_meta = new BufferMeta(
            params, portIndex,
            (portIndex == kPortIndexInput) && copy /* copyToOmx */,
            (portIndex == kPortIndexOutput) && copy /* copyFromOmx */);

    OMX_BUFFERHEADERTYPE *header;

    OMX_ERRORTYPE err = OMX_AllocateBuffer( mHandle, &header, portIndex, buffer_meta, allottedSize);
    if (err != OMX_ErrorNone) {
        CLOG_ERROR(allocateBufferWithBackup, err,
                SIMPLE_BUFFER(portIndex, (size_t)allottedSize, params->data()));
        delete buffer_meta;
        buffer_meta = NULL;

        *buffer = 0;

        return StatusFromOMXError(err);
    }

    CHECK_EQ(header->pAppPrivate, buffer_meta);

    *buffer = makeBufferID(header);

    addActiveBuffer(portIndex, *buffer);

    CLOG_BUFFER(allocateBufferWithBackup, NEW_BUFFER_FMT(*buffer, portIndex, "%zu@%p :> %u@%p",
                params->size(), params->data(), allottedSize, header->pBuffer));

    return OK;
}

status_t OMXNodeInstance::freeBuffer(
        OMX_U32 portIndex, OMX::buffer_id buffer) {
    Mutex::Autolock autoLock(mLock);
    CLOG_BUFFER(freeBuffer, "%s:%u %#x", portString(portIndex), portIndex, buffer);

    removeActiveBuffer(portIndex, buffer);

    OMX_BUFFERHEADERTYPE *header = findBufferHeader(buffer, portIndex);
    if (header == NULL) {
        LLOGE("b/25884056");
        return BAD_VALUE;
    }
    BufferMeta *buffer_meta = static_cast<BufferMeta *>(header->pAppPrivate);

    OMX_ERRORTYPE err = OMX_FreeBuffer(mHandle, portIndex, header);
    CLOG_IF_ERROR(freeBuffer, err, "%s:%u %#x", portString(portIndex), portIndex, buffer);

    delete buffer_meta;
    buffer_meta = NULL;
    invalidateBufferID(buffer);

    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::fillBuffer(OMX::buffer_id buffer, int fenceFd) {
    Mutex::Autolock autoLock(mLock);

    OMX_BUFFERHEADERTYPE *header = findBufferHeader(buffer, kPortIndexOutput);
    if (header == NULL) {
        LLOGE("b/25884056");
        return BAD_VALUE;
    }
    header->nFilledLen = 0;
    header->nOffset = 0;
    header->nFlags = 0;


    
    OMX_ERRORTYPE err = OMX_FillThisBuffer(mHandle, header);
    if (err != OMX_ErrorNone) {
        CLOG_ERROR(fillBuffer, err, EMPTY_BUFFER(buffer, header, fenceFd));
    }
    return StatusFromOMXError(err);
}

status_t OMXNodeInstance::emptyBuffer(
        OMX::buffer_id buffer,
        OMX_U32 rangeOffset, OMX_U32 rangeLength,
        OMX_U32 flags, OMX_TICKS timestamp, int fenceFd) {
    Mutex::Autolock autoLock(mLock);


    OMX_BUFFERHEADERTYPE *header = findBufferHeader(buffer, kPortIndexInput);
    if (header == NULL) {
        LLOGE("b/25884056");
        return BAD_VALUE;
    }
    BufferMeta *buffer_meta = static_cast<BufferMeta *>(header->pAppPrivate);

    // rangeLength and rangeOffset must be a subset of the allocated data in the buffer.
  // corner case: we permit rangeOffset == end-of-buffer with rangeLength == 0.
    if (rangeOffset > header->nAllocLen
            || rangeLength > header->nAllocLen - rangeOffset) {
        CLOG_ERROR(emptyBuffer, OMX_ErrorBadParameter, FULL_BUFFER(NULL, header, fenceFd));
        if (fenceFd >= 0) {
            ::close(fenceFd);
        }
        return BAD_VALUE;
    }
    header->nFilledLen = rangeLength;
    header->nOffset = rangeOffset;

    buffer_meta->CopyToOMX(header);

    return emptyBuffer_l(header, flags, timestamp, (intptr_t)buffer, fenceFd);
}




status_t OMXNodeInstance::emptyBuffer_l(
        OMX_BUFFERHEADERTYPE *header, OMX_U32 flags, OMX_TICKS timestamp,
        intptr_t debugAddr, int fenceFd) {
    header->nFlags = flags;
    header->nTimeStamp = timestamp;

    OMX_ERRORTYPE err = OMX_EmptyThisBuffer(mHandle, header);
    CLOG_IF_ERROR(emptyBuffer, err, FULL_BUFFER(debugAddr, header, fenceFd));

    
    return StatusFromOMXError(err);
}


status_t OMXNodeInstance::getExtensionIndex(
        const char *parameterName, OMX_INDEXTYPE *index) {
    Mutex::Autolock autoLock(mLock);

    OMX_ERRORTYPE err = OMX_GetExtensionIndex(
            mHandle, const_cast<char *>(parameterName), index);

    return StatusFromOMXError(err);
}

bool OMXNodeInstance::handleMessage(omx_message &msg) {

    if (msg.type == omx_message::FILL_BUFFER_DONE) {
        OMX_BUFFERHEADERTYPE *buffer =
            findBufferHeader(msg.u.extended_buffer_data.buffer, kPortIndexOutput);
        if (buffer == NULL) {
            LLOGE("b/25884056");
            return false;
        }


        BufferMeta *buffer_meta =
            static_cast<BufferMeta *>(buffer->pAppPrivate);

        if (buffer->nOffset + buffer->nFilledLen < buffer->nOffset
                || buffer->nOffset + buffer->nFilledLen > buffer->nAllocLen) {
            CLOG_ERROR(onFillBufferDone, OMX_ErrorBadParameter,
                    FULL_BUFFER(NULL, buffer, msg.fenceFd));
        }
        buffer_meta->CopyFromOMX(buffer);

    } else if (msg.type == omx_message::EMPTY_BUFFER_DONE) {
        OMX_BUFFERHEADERTYPE *buffer =
            findBufferHeader(msg.u.buffer_data.buffer, kPortIndexInput);
        if (buffer == NULL) {
            return false;
        }


    }

    return false;
}

void OMXNodeInstance::onMessages(std::list<omx_message> &messages) {
    for (std::list<omx_message>::iterator it = messages.begin(); it != messages.end(); ) {
        if (handleMessage(*it)) {
            messages.erase(it++);
        } else {
            ++it;
        }
    }

    if (!messages.empty()) {
        mObserver->onMessages(messages);
    }
}

void OMXNodeInstance::onObserverDied(OMXMaster *master) {
    LLOGE("!!! Observer died. Quickly, do something, ... anything...");

    // Try to force shutdown of the node and hope for the best.
    freeNode(master);
}

void OMXNodeInstance::onGetHandleFailed() {
    delete this;
}

// OMXNodeInstance::OnEvent calls OMX::OnEvent, which then calls here.
// Don't try to acquire mLock here -- in rare circumstances this will hang.
void OMXNodeInstance::onEvent(
        OMX_EVENTTYPE event, OMX_U32 arg1, OMX_U32 arg2) {
    const char *arg1String = "??";
    const char *arg2String = "??";
    Level level = kDebugInternalState;

    switch (event) {
        case OMX_EventCmdComplete:
            arg1String = asString((OMX_COMMANDTYPE)arg1);
            switch (arg1) {
                case OMX_CommandStateSet:
                    arg2String = asString((OMX_STATETYPE)arg2);
                    level = kDebugState;
                    break;
                case OMX_CommandFlush:
                case OMX_CommandPortEnable:
                        // fall through
                default:
                    arg2String = portString(arg2);
            }
            break;
        case OMX_EventError:
            arg1String = asString((OMX_ERRORTYPE)arg1);
            level = kDebugLifeCycle;
            break;
        case OMX_EventPortSettingsChanged:
            arg2String = asString((OMX_INDEXEXTTYPE)arg2);
            // fall through
        default:
            arg1String = portString(arg1);
    }

    CLOGI_(level, onEvent, "%s(%x), %s(%x), %s(%x)",
            asString(event), event, arg1String, arg1, arg2String, arg2);


    // allow configuration if we return to the loaded state
    if (event == OMX_EventCmdComplete
            && arg1 == OMX_CommandStateSet
            && arg2 == OMX_StateLoaded) {
        mSailed = false;
    }
}

// static
OMX_ERRORTYPE OMXNodeInstance::OnEvent(
        OMX_IN OMX_HANDLETYPE /* hComponent */,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData) {
    if (pAppData == NULL) {
        LLOGE("b/25884056");
        return OMX_ErrorBadParameter;
    }
    OMXNodeInstance *instance = static_cast<OMXNodeInstance *>(pAppData);
    if (instance->mDying) {
        return OMX_ErrorNone;
    }
    return instance->owner()->OnEvent(
            instance->nodeID(), eEvent, nData1, nData2, pEventData);
}

// static
OMX_ERRORTYPE OMXNodeInstance::OnEmptyBufferDone(
        OMX_IN OMX_HANDLETYPE /* hComponent */,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {
    if (pAppData == NULL) {
        LLOGE("b/25884056");
        return OMX_ErrorBadParameter;
    }
    OMXNodeInstance *instance = static_cast<OMXNodeInstance *>(pAppData);
    if (instance->mDying) {
        return OMX_ErrorNone;
    }
    return instance->owner()->OnEmptyBufferDone(instance->nodeID(),
            instance->findBufferID(pBuffer), pBuffer, -1/*fenceFd*/);
}

// static
OMX_ERRORTYPE OMXNodeInstance::OnFillBufferDone(
        OMX_IN OMX_HANDLETYPE /* hComponent */,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {
    if (pAppData == NULL) {
        LLOGE("b/25884056");
        return OMX_ErrorBadParameter;
    }
    OMXNodeInstance *instance = static_cast<OMXNodeInstance *>(pAppData);
    if (instance->mDying) {
        return OMX_ErrorNone;
    }
    return instance->owner()->OnFillBufferDone(instance->nodeID(),
            instance->findBufferID(pBuffer), pBuffer, -1/*fenceFd*/);
}

void OMXNodeInstance::addActiveBuffer(OMX_U32 portIndex, OMX::buffer_id id) {
    ActiveBuffer active;
    active.mPortIndex = portIndex;
    active.mID = id;
    mActiveBuffers.push(active);

    if (portIndex < NELEM(mNumPortBuffers)) {
        ++mNumPortBuffers[portIndex];
    }
}

void OMXNodeInstance::removeActiveBuffer(
        OMX_U32 portIndex, OMX::buffer_id id) {
    for (size_t i = 0; i < mActiveBuffers.size(); ++i) {
        if (mActiveBuffers[i].mPortIndex == portIndex
                && mActiveBuffers[i].mID == id) {
            mActiveBuffers.removeItemsAt(i);

            if (portIndex < NELEM(mNumPortBuffers)) {
                --mNumPortBuffers[portIndex];
            }
            return;
        }
    }

    CLOGW("Attempt to remove an active buffer [%#x] we know nothing about...", id);
}

void OMXNodeInstance::freeActiveBuffers() {
    // Make sure to count down here, as freeBuffer will in turn remove
  // the active buffer from the vector...
    for (size_t i = mActiveBuffers.size(); i > 0;) {
        i--;
        freeBuffer(mActiveBuffers[i].mPortIndex, mActiveBuffers[i].mID);
    }
}

OMX::buffer_id OMXNodeInstance::makeBufferID(OMX_BUFFERHEADERTYPE *bufferHeader) {
    if (bufferHeader == NULL) {
        return 0;
    }
    Mutex::Autolock autoLock(mBufferIDLock);
    OMX::buffer_id buffer;
    do { // handle the very unlikely case of ID overflow
        if (++mBufferIDCount == 0) {
            ++mBufferIDCount;
        }
        buffer = (OMX::buffer_id)mBufferIDCount;
    } while (mBufferIDToBufferHeader.indexOfKey(buffer) >= 0);
    mBufferIDToBufferHeader.add(buffer, bufferHeader);
    mBufferHeaderToBufferID.add(bufferHeader, buffer);
    return buffer;
}

OMX_BUFFERHEADERTYPE *OMXNodeInstance::findBufferHeader(
        OMX::buffer_id buffer, OMX_U32 portIndex) {
    if (buffer == 0) {
        return NULL;
    }
    Mutex::Autolock autoLock(mBufferIDLock);
    ssize_t index = mBufferIDToBufferHeader.indexOfKey(buffer);
    if (index < 0) {
        CLOGW("findBufferHeader: buffer %u not found", buffer);
        return NULL;
    }
    OMX_BUFFERHEADERTYPE *header = mBufferIDToBufferHeader.valueAt(index);
    BufferMeta *buffer_meta =
        static_cast<BufferMeta *>(header->pAppPrivate);
    if (buffer_meta->getPortIndex() != portIndex) {
        CLOGW("findBufferHeader: buffer %u found but with incorrect port index.", buffer);
        LLOGE("0x534e4554, 28816827");
        return NULL;
    }
    return header;
}

OMX::buffer_id OMXNodeInstance::findBufferID(OMX_BUFFERHEADERTYPE *bufferHeader) {
    if (bufferHeader == NULL) {
        return 0;
    }
    Mutex::Autolock autoLock(mBufferIDLock);
    ssize_t index = mBufferHeaderToBufferID.indexOfKey(bufferHeader);
    if (index < 0) {
        CLOGW("findBufferID: bufferHeader %p not found", bufferHeader);
        return 0;
    }
    return mBufferHeaderToBufferID.valueAt(index);
}

void OMXNodeInstance::invalidateBufferID(OMX::buffer_id buffer) {
    if (buffer == 0) {
        return;
    }
    Mutex::Autolock autoLock(mBufferIDLock);
    ssize_t index = mBufferIDToBufferHeader.indexOfKey(buffer);
    if (index < 0) {
        CLOGW("invalidateBufferID: buffer %u not found", buffer);
        return;
    }
    mBufferHeaderToBufferID.removeItem(mBufferIDToBufferHeader.valueAt(index));
    mBufferIDToBufferHeader.removeItemsAt(index);
}

LIYL_NAMESPACE_END
