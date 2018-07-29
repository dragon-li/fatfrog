/*
 * Copyright (C) 2010 The Liyl Open Source Project
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
#define LOG_TAG "ACodec"

#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

#include <inttypes.h>


#include "ACodec.h"

#include "../../foundation/media/include/hexdump.h"
#include "../../foundation/media/include/ABuffer.h"
#include "../../foundation/media/include/ADebug.h"
#include "../../foundation/media/include/AMessage.h"
#include "../../foundation/media/include/AUtils.h"

#include "MediaWindow.h"
#include "MediaCodec.h"
#include "MediaCodecList.h"
#include "../../common/include/MediaDefs.h"
#include "../../common/include/MediaErrors.h"
#include "../../common/include/HardwareAPI.h"

#include "../../prebuild/openmax/OMX_AudioExt.h"
#include "../../prebuild/openmax/OMX_VideoExt.h"
#include "../../prebuild/openmax/OMX_Component.h"
#include "../../prebuild/openmax/OMX_IndexExt.h"
#include "../../prebuild/openmax/OMX_AsString.h"

#include "avc_utils.h"
#include "../../omx/OMXUtils.h"
#include "../../omx/OMX.h"

#define KCODEC_ALIGN 32u

LIYL_NAMESPACE_START

enum {
    kMaxIndicesToCheck = 32, // used when enumerating supported formats and profiles
};

// OMX errors are directly mapped into status_t range if
// there is no corresponding MediaError status code.
// Use the statusFromOMXError(int32_t omxError) function.
//
// Currently this is a direct map.
// See frameworks/native/include/media/openmax/OMX_Core.h
//
// Vendor OMX errors     from 0x90000000 - 0x9000FFFF
// Extension OMX errors  from 0x8F000000 - 0x90000000
// Standard OMX errors   from 0x80001000 - 0x80001024 (0x80001024 current)
//

// returns true if err is a recognized OMX error code.
// as OMX error is OMX_S32, this is an int32_t type
static inline bool isOMXError(int32_t err) {
    return (ERROR_CODEC_MIN <= err && err <= ERROR_CODEC_MAX);
}

// converts an OMX error to a status_t
static inline status_t statusFromOMXError(int32_t omxError) {
    switch (omxError) {
    case OMX_ErrorInvalidComponentName:
    case OMX_ErrorComponentNotFound:
        return NAME_NOT_FOUND; // can trigger illegal argument error for provided names.
    default:
        return isOMXError(omxError) ? omxError : 0; // no translation required
    }
}

// checks and converts status_t to a non-side-effect status_t
static inline status_t makeNoSideEffectStatus(status_t err) {
    switch (err) {
    // the following errors have side effects and may come
    // from other code modules. Remap for safety reasons.
    case INVALID_OPERATION:
    case DEAD_OBJECT:
        return UNKNOWN_ERROR;
    default:
        return err;
    }
}

struct MessageList : public RefBase {
    MessageList() {
    }
    virtual ~MessageList() {
    }
    std::list<sp<AMessage> > &getList() { return mList; }
private:
    std::list<sp<AMessage> > mList;

    DISALLOW_EVIL_CONSTRUCTORS(MessageList);
};


struct CodecObserver : public IOMXObserver {
    CodecObserver() {}

    void setNotificationMessage(const sp<AMessage> &msg) {
        mNotify = msg;
    }

    // from IOMXObserver
    virtual void onMessages(const std::list<omx_message> &messages) {
        if (messages.empty()) {
            return;
        }

        sp<AMessage> notify = mNotify->dup();
        bool first = true;
        sp<MessageList> msgList = new MessageList();
        for (std::list<omx_message>::const_iterator it = messages.cbegin();
              it != messages.cend(); ++it) {
            const omx_message &omx_msg = *it;
            if (first) {
                notify->setInt32("node", omx_msg.node);
                first = false;
            }

            sp<AMessage> msg = new AMessage;
            msg->setInt32("type", omx_msg.type);
            switch (omx_msg.type) {
                case omx_message::EVENT:
                {
                    msg->setInt32("event", omx_msg.u.event_data.event);
                    msg->setInt32("data1", omx_msg.u.event_data.data1);
                    msg->setInt32("data2", omx_msg.u.event_data.data2);
                    break;
                }

                case omx_message::EMPTY_BUFFER_DONE:
                {
                    msg->setInt32("buffer", omx_msg.u.buffer_data.buffer);
                    break;
                }

                case omx_message::FILL_BUFFER_DONE:
                {
                    msg->setInt32(
                            "buffer", omx_msg.u.extended_buffer_data.buffer);
                    msg->setInt32(
                            "range_offset",
                            omx_msg.u.extended_buffer_data.range_offset);
                    msg->setInt32(
                            "range_length",
                            omx_msg.u.extended_buffer_data.range_length);
                    msg->setInt32(
                            "flags",
                            omx_msg.u.extended_buffer_data.flags);
                    msg->setInt64(
                            "timestamp",
                            omx_msg.u.extended_buffer_data.timestamp);
                    break;
                }

                case omx_message::FRAME_RENDERED:
                {
					//TODO
                    LLOGW("omx_message::FRAME_RENDERED");
                    break;
                }

                default:
                    LLOGE("Unrecognized message type: %d", omx_msg.type);
                    break;
            }
            msgList->getList().push_back(msg);
        }
        notify->setObject("messages", msgList);
        notify->post();
    }

protected:
    virtual ~CodecObserver() {}

private:
    sp<AMessage> mNotify;

    DISALLOW_EVIL_CONSTRUCTORS(CodecObserver);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::BaseState : public AState {
    BaseState(ACodec *codec, const sp<AState> &parentState = NULL);

protected:
    enum PortMode {
        KEEP_BUFFERS,
        RESUBMIT_BUFFERS,
        FREE_BUFFERS,
    };

    ACodec *mCodec;

    virtual PortMode getPortMode(OMX_U32 portIndex);

    virtual bool onMessageReceived(const sp<AMessage> &msg);

    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);

    virtual void onOutputBufferDrained(const sp<AMessage> &msg);
    virtual void onInputBufferFilled(const sp<AMessage> &msg);

    void postFillThisBuffer(BufferInfo *info);

private:
    // Handles an OMX message. Returns true iff message was handled.
    bool onOMXMessage(const sp<AMessage> &msg);

    // Handles a list of messages. Returns true iff messages were handled.
    bool onOMXMessageList(const sp<AMessage> &msg);

    // returns true iff this message is for this component and the component is alive
    bool checkOMXMessage(const sp<AMessage> &msg);

    bool onOMXEmptyBufferDone(IOMX::buffer_id bufferID);

    bool onOMXFillBufferDone(
            IOMX::buffer_id bufferID,
            size_t rangeOffset, size_t rangeLength,
            OMX_U32 flags,
            int64_t timeUs);

    virtual bool onOMXFrameRendered(int64_t mediaTimeUs, nsecs_t systemNano);

    void getMoreInputDataIfPossible();

    DISALLOW_EVIL_CONSTRUCTORS(BaseState);
};

////////////////////////////////////////////////////////////////////////////////


struct ACodec::UninitializedState : public ACodec::BaseState {
    UninitializedState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

private:
    void onSetup(const sp<AMessage> &msg);
    bool onAllocateComponent(const sp<AMessage> &msg);


    DISALLOW_EVIL_CONSTRUCTORS(UninitializedState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::LoadedState : public ACodec::BaseState {
    LoadedState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

private:
    friend struct ACodec::UninitializedState;

    bool onConfigureComponent(const sp<AMessage> &msg);
    void onStart();
    void onShutdown(bool keepComponentAllocated);


    DISALLOW_EVIL_CONSTRUCTORS(LoadedState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::LoadedToIdleState : public ACodec::BaseState {
    LoadedToIdleState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);
    virtual void stateEntered();

private:
    status_t allocateBuffers();

    DISALLOW_EVIL_CONSTRUCTORS(LoadedToIdleState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::IdleToExecutingState : public ACodec::BaseState {
    IdleToExecutingState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);
    virtual void stateEntered();

private:
    DISALLOW_EVIL_CONSTRUCTORS(IdleToExecutingState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::ExecutingState : public ACodec::BaseState {
    ExecutingState(ACodec *codec);

    void submitRegularOutputBuffers();
    void submitOutputBuffers();

    // Submit output buffers to the decoder, submit input buffers to client
    // to fill with data.
    void resume();

    // Returns true iff input and output buffers are in play.
    bool active() const { return mActive; }

protected:
    virtual PortMode getPortMode(OMX_U32 portIndex);
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);
    virtual bool onOMXFrameRendered(int64_t mediaTimeUs, nsecs_t systemNano);

private:
    bool mActive;

    DISALLOW_EVIL_CONSTRUCTORS(ExecutingState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::OutputPortSettingsChangedState : public ACodec::BaseState {
    OutputPortSettingsChangedState(ACodec *codec);

protected:
    virtual PortMode getPortMode(OMX_U32 portIndex);
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);
    virtual bool onOMXFrameRendered(int64_t mediaTimeUs, nsecs_t systemNano);

private:
    DISALLOW_EVIL_CONSTRUCTORS(OutputPortSettingsChangedState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::ExecutingToIdleState : public ACodec::BaseState {
    ExecutingToIdleState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);

    virtual void onOutputBufferDrained(const sp<AMessage> &msg);
    virtual void onInputBufferFilled(const sp<AMessage> &msg);

private:
    void changeStateIfWeOwnAllBuffers();

    bool mComponentNowIdle;

    DISALLOW_EVIL_CONSTRUCTORS(ExecutingToIdleState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::IdleToLoadedState : public ACodec::BaseState {
    IdleToLoadedState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);

private:
    DISALLOW_EVIL_CONSTRUCTORS(IdleToLoadedState);
};

////////////////////////////////////////////////////////////////////////////////

struct ACodec::FlushingState : public ACodec::BaseState {
    FlushingState(ACodec *codec);

protected:
    virtual bool onMessageReceived(const sp<AMessage> &msg);
    virtual void stateEntered();

    virtual bool onOMXEvent(OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2);

    virtual void onOutputBufferDrained(const sp<AMessage> &msg);
    virtual void onInputBufferFilled(const sp<AMessage> &msg);

private:
    bool mFlushComplete[2];

    void changeStateIfWeOwnAllBuffers();

    DISALLOW_EVIL_CONSTRUCTORS(FlushingState);
};

////////////////////////////////////////////////////////////////////////////////


ACodec::ACodec()
    : mQuirks(0),
      mNode(0),
      mUsingNativeWindow(false),
      mIsVideo(false),
      mIsEncoder(false),
      mFatalError(false),
      mShutdownInProgress(false),
      mExplicitShutdown(false),
      mChannelMaskPresent(false),
      mChannelMask(0),
      mDequeueCounter(0),
      mDescribeColorAspectsIndex((OMX_INDEXTYPE)0) {
    mUninitializedState = new UninitializedState(this);
    mLoadedState = new LoadedState(this);
    mLoadedToIdleState = new LoadedToIdleState(this);
    mIdleToExecutingState = new IdleToExecutingState(this);
    mExecutingState = new ExecutingState(this);

    mOutputPortSettingsChangedState =
        new OutputPortSettingsChangedState(this);

    mExecutingToIdleState = new ExecutingToIdleState(this);
    mIdleToLoadedState = new IdleToLoadedState(this);
    mFlushingState = new FlushingState(this);

    mPortEOS[kPortIndexInput] = mPortEOS[kPortIndexOutput] = false;
    mInputEOSResult = OK;


    changeState(mUninitializedState);
}

ACodec::~ACodec() {
}

void ACodec::setNotificationMessage(const sp<AMessage> &msg) {
    mNotify = msg;
}

void ACodec::initiateSetup(const sp<AMessage> &msg) {
    msg->setWhat(kWhatSetup);
    msg->setTarget(this);
    msg->post();
}

void ACodec::signalSetParameters(const sp<AMessage> &params) {
    sp<AMessage> msg = new AMessage(kWhatSetParameters, this);
    msg->setMessage("params", params);
    msg->post();
}

void ACodec::initiateAllocateComponent(const sp<AMessage> &msg) {
    msg->setWhat(kWhatAllocateComponent);
    msg->setTarget(this);
    msg->post();
}

void ACodec::initiateConfigureComponent(const sp<AMessage> &msg) {
    msg->setWhat(kWhatConfigureComponent);
    msg->setTarget(this);
    msg->post();
}

status_t ACodec::setSurface(const MediaWindow* surface) {

    return INVALID_OPERATION;
}



void ACodec::signalEndOfInputStream() {
    (new AMessage(kWhatSignalEndOfInputStream, this))->post();
}

void ACodec::initiateStart() {
    (new AMessage(kWhatStart, this))->post();
}

void ACodec::signalFlush() {
    LLOGV("[%s] signalFlush", mComponentName.c_str());
    (new AMessage(kWhatFlush, this))->post();
}

void ACodec::signalResume() {
    (new AMessage(kWhatResume, this))->post();
}

void ACodec::initiateShutdown(bool keepComponentAllocated) {
    sp<AMessage> msg = new AMessage(kWhatShutdown, this);
    msg->setInt32("keepComponentAllocated", keepComponentAllocated);
    msg->post();
    if (!keepComponentAllocated) {
        // ensure shutdown completes in 3 seconds
        (new AMessage(kWhatReleaseCodecInstance, this))->post(3000000);
    }
}




status_t ACodec::allocateBuffersOnPort(OMX_U32 portIndex) {
    CHECK(portIndex == kPortIndexInput || portIndex == kPortIndexOutput);

    CHECK(mBuffers[portIndex].isEmpty());

    status_t err;
        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParams(&def);
        def.nPortIndex = portIndex;

        err = mOMX->getParameter(
                mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

        if (err == OK) {
            size_t bufSize = def.nBufferSize;

			//TODO
            size_t allottedSize = bufSize;


            size_t alignment = KCODEC_ALIGN;

            LLOGV("[%s] Allocating %u buffers of size %zu/%zu (from %u using %s) on %s port",
                    mComponentName.c_str(),
                    def.nBufferCountActual, bufSize, allottedSize, def.nBufferSize, "ABuffer",
                    portIndex == kPortIndexInput ? "input" : "output");

            // verify buffer sizes to avoid overflow in align()
            if (bufSize == 0 || bufSize > kMaxCodecBufferSize) {
                LLOGE("b/22885421");
                return NO_MEMORY;
            }

            // don't modify bufSize as OMX may not expect it to increase after negotiation
            size_t alignedSize = align(bufSize, alignment);
            if (def.nBufferCountActual > SIZE_MAX / alignedSize) {
                LLOGE("b/22885421");
                return NO_MEMORY;
            }

            for (OMX_U32 i = 0; i < def.nBufferCountActual && err == OK; ++i) {
                sp<ABuffer> mem = new ABuffer(bufSize);
                if (mem == NULL || mem->base() == NULL) {
                    return NO_MEMORY;
                }

                BufferInfo info;
                info.mStatus = BufferInfo::OWNED_BY_US;
                info.mData = mem;
             
                 err = mOMX->useBuffer(mNode, portIndex, mem, &info.mBufferID, allottedSize);
                mBuffers[portIndex].push(info);
            }
        }

    if (err != OK) {
        return err;
    }

    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", CodecBase::kWhatBuffersAllocated);

    notify->setInt32("portIndex", portIndex);

    sp<PortDescription> desc = new PortDescription;

    for (size_t i = 0; i < mBuffers[portIndex].size(); ++i) {
        const BufferInfo &info = mBuffers[portIndex][i];
        desc->addBuffer(info.mBufferID, info.mData);
    }

    notify->setObject("portDesc", desc);
    notify->post();

    return OK;
}




// static
const char *ACodec::_asString(BufferInfo::Status s) {
    switch (s) {
        case BufferInfo::OWNED_BY_US:            return "OUR";
        case BufferInfo::OWNED_BY_COMPONENT:     return "COMPONENT";
        case BufferInfo::OWNED_BY_UPSTREAM:      return "UPSTREAM";
        case BufferInfo::OWNED_BY_DOWNSTREAM:    return "DOWNSTREAM";
        case BufferInfo::OWNED_BY_NATIVE_WINDOW: return "SURFACE";
        case BufferInfo::UNRECOGNIZED:           return "UNRECOGNIZED";
        default:                                 return "?";
    }
}

void ACodec::dumpBuffers(OMX_U32 portIndex) {
    CHECK(portIndex == kPortIndexInput || portIndex == kPortIndexOutput);
    LLOGI("[%s] %s port has %zu buffers:", mComponentName.c_str(),
            portIndex == kPortIndexInput ? "input" : "output", mBuffers[portIndex].size());
    for (size_t i = 0; i < mBuffers[portIndex].size(); ++i) {
        const BufferInfo &info = mBuffers[portIndex][i];
        LLOGI("  slot %2zu: #%8u %p %s(%d) size:%zu",
                i, info.mBufferID, info.mData.get(),
                _asString(info.mStatus), info.mStatus,info.mData->size());
    }
}

status_t ACodec::freeBuffersOnPort(OMX_U32 portIndex) {
    status_t err = OK;
    for (size_t i = mBuffers[portIndex].size(); i > 0;) {
        i--;
        status_t err2 = freeBuffer(portIndex, i);
        if (err == OK) {
            err = err2;
        }
    }

    return err;
}

status_t ACodec::freeOutputBuffersNotOwnedByComponent() {
    status_t err = OK;
    for (size_t i = mBuffers[kPortIndexOutput].size(); i > 0;) {
        i--;
        BufferInfo *info =
            &mBuffers[kPortIndexOutput].editItemAt(i);

        // At this time some buffers may still be with the component
        // or being drained.
        if (info->mStatus != BufferInfo::OWNED_BY_COMPONENT &&
            info->mStatus != BufferInfo::OWNED_BY_DOWNSTREAM) {
            status_t err2 = freeBuffer(kPortIndexOutput, i);
            if (err == OK) {
                err = err2;
            }
        }
    }

    return err;
}

status_t ACodec::freeBuffer(OMX_U32 portIndex, size_t i) {
    BufferInfo *info = &mBuffers[portIndex].editItemAt(i);
    status_t err = OK;


    switch (info->mStatus) {
        case BufferInfo::OWNED_BY_US:
            // fall through

        case BufferInfo::OWNED_BY_NATIVE_WINDOW:
            err = mOMX->freeBuffer(mNode, portIndex, info->mBufferID);
            break;

        default:
            LLOGE("trying to free buffer not owned by us or ANW (%d)", info->mStatus);
            err = FAILED_TRANSACTION;
            break;
    }

    // remove buffer even if mOMX->freeBuffer fails
    mBuffers[portIndex].removeAt(i);
    return err;
}

ACodec::BufferInfo *ACodec::findBufferByID(
        uint32_t portIndex, IOMX::buffer_id bufferID, ssize_t *index) {
    for (size_t i = 0; i < mBuffers[portIndex].size(); ++i) {
        BufferInfo *info = &mBuffers[portIndex].editItemAt(i);

        if (info->mBufferID == bufferID) {
            if (index != NULL) {
                *index = i;
            }
            return info;
        }
    }

    LLOGE("Could not find buffer with ID %u", bufferID);
    return NULL;
}

status_t ACodec::setComponentRole(
        bool isEncoder, const char *mime) {
    const char *role = getComponentRole(isEncoder, mime);
    if (role == NULL) {
        return BAD_VALUE;
    }
    status_t err = setComponentRole(mOMX, mNode, role);
    if (err != OK) {
        LLOGW("[%s] Failed to set standard component role '%s'.",
             mComponentName.c_str(), role);
    }
    return err;
}

//static
const char *ACodec::getComponentRole(
        bool isEncoder, const char *mime) {
    struct MimeToRole {
        const char *mime;
        const char *decoderRole;
        const char *encoderRole;
    };

    static const MimeToRole kMimeToRole[] = {
        { MEDIA_MIMETYPE_AUDIO_AAC,
            "audio_decoder.aac", "audio_encoder.aac" },
        { MEDIA_MIMETYPE_VIDEO_AVC,
            "video_decoder.avc", "video_encoder.avc" },
        { MEDIA_MIMETYPE_VIDEO_HEVC,
            "video_decoder.hevc", "video_encoder.hevc" },
        { MEDIA_MIMETYPE_AUDIO_RAW,
            "audio_decoder.raw", "audio_encoder.raw" },
        { MEDIA_MIMETYPE_AUDIO_AC3,
            "audio_decoder.ac3", "audio_encoder.ac3" },
        { MEDIA_MIMETYPE_AUDIO_EAC3,
            "audio_decoder.eac3", "audio_encoder.eac3" },
    };

    static const size_t kNumMimeToRole =
        sizeof(kMimeToRole) / sizeof(kMimeToRole[0]);

    size_t i;
    for (i = 0; i < kNumMimeToRole; ++i) {
        if (!strcasecmp(mime, kMimeToRole[i].mime)) {
            break;
        }
    }

    if (i == kNumMimeToRole) {
        return NULL;
    }

    return isEncoder ? kMimeToRole[i].encoderRole
                  : kMimeToRole[i].decoderRole;
}

//static
status_t ACodec::setComponentRole(
        const sp<IOMX> &omx, IOMX::node_id node, const char *role) {
    OMX_PARAM_COMPONENTROLETYPE roleParams;
    InitOMXParams(&roleParams);

    strncpy((char *)roleParams.cRole,
            role, OMX_MAX_STRINGNAME_SIZE - 1);

    roleParams.cRole[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';

    return omx->setParameter(
            node, OMX_IndexParamStandardComponentRole,
            &roleParams, sizeof(roleParams));
}

status_t ACodec::configureCodec(
        const char *mime, const sp<AMessage> &msg) {
    int32_t encoder;
    if (!msg->findInt32("encoder", &encoder)) {
        encoder = false;
    }

    sp<AMessage> inputFormat = new AMessage;
    sp<AMessage> outputFormat = new AMessage;
    mConfigFormat = msg;

    mIsEncoder = encoder;

    status_t err = setComponentRole(encoder /* isEncoder */, mime);

    if (err != OK) {
        return err;
    }



    // Only enable metadata mode on encoder output if encoder can prepend
    // sps/pps to idr frames, since in metadata mode the bitstream is in an
    // opaque handle, to which we don't have access.
    int32_t video = !strncasecmp(mime, "video/", 6);
    mIsVideo = video;

    // NOTE: we only use native window for video decoders
    sp<RefBase> obj;
    bool haveNativeWindow = msg->findObject("native-window", &obj)
            && obj != NULL && video && !encoder;
    mUsingNativeWindow = haveNativeWindow;
    if (video && !encoder) {
        inputFormat->setInt32("adaptive-playback", false);
    }

    AudioEncoding pcmEncoding = kAudioEncodingPcm16bit;
    (void)msg->findInt32("pcm-encoding", (int32_t*)&pcmEncoding);

    if (video) {
        // determine need for software renderer
        bool usingSwRenderer = false;
        if (haveNativeWindow && mComponentName.startsWith("OMX.google.")) {
            usingSwRenderer = true;
            haveNativeWindow = false;
        }

        err = setupVideoDecoder(mime, msg, haveNativeWindow, usingSwRenderer, outputFormat);

        if (err != OK) {
            return err;
        }


        // fallback for devices that do not handle flex-YUV for native buffers

        if (usingSwRenderer) {
            outputFormat->setInt32("using-sw-renderer", 1);
        }

    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC)) {
        int32_t numChannels, sampleRate;
        if (!msg->findInt32("channel-count", &numChannels)
                || !msg->findInt32("sample-rate", &sampleRate)) {
            err = INVALID_OPERATION;
        } else {
            int32_t isADTS, aacProfile;
            int32_t sbrMode;
            int32_t maxOutputChannelCount;
            int32_t pcmLimiterEnable;
            drcParams_t drc;
            if (!msg->findInt32("is-adts", &isADTS)) {
                isADTS = 0;
            }
            if (!msg->findInt32("aac-profile", &aacProfile)) {
                aacProfile = OMX_AUDIO_AACObjectNull;
            }
            if (!msg->findInt32("aac-sbr-mode", &sbrMode)) {
                sbrMode = -1;
            }

            if (!msg->findInt32("aac-max-output-channel_count", &maxOutputChannelCount)) {
                maxOutputChannelCount = -1;
            }
            if (!msg->findInt32("aac-pcm-limiter-enable", &pcmLimiterEnable)) {
                // value is unknown
                pcmLimiterEnable = -1;
            }
            if (!msg->findInt32("aac-encoded-target-level", &drc.encodedTargetLevel)) {
                // value is unknown
                drc.encodedTargetLevel = -1;
            }
            if (!msg->findInt32("aac-drc-cut-level", &drc.drcCut)) {
                // value is unknown
                drc.drcCut = -1;
            }
            if (!msg->findInt32("aac-drc-boost-level", &drc.drcBoost)) {
                // value is unknown
                drc.drcBoost = -1;
            }
            if (!msg->findInt32("aac-drc-heavy-compression", &drc.heavyCompression)) {
                // value is unknown
                drc.heavyCompression = -1;
            }
            if (!msg->findInt32("aac-target-ref-level", &drc.targetRefLevel)) {
                // value is unknown
                drc.targetRefLevel = -1;
            }

            err = setupAACCodec(
                    encoder, numChannels, sampleRate, 0/*bitRate*/, aacProfile,
                    isADTS != 0, sbrMode, maxOutputChannelCount, drc,
                    pcmLimiterEnable);
        }

    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_RAW)) {
        int32_t numChannels, sampleRate;
        if (encoder
                || !msg->findInt32("channel-count", &numChannels)
                || !msg->findInt32("sample-rate", &sampleRate)) {
            err = INVALID_OPERATION;
        } else {
            err = setupRawAudioFormat(kPortIndexInput, sampleRate, numChannels, pcmEncoding);
        }
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AC3)) {
        int32_t numChannels;
        int32_t sampleRate;
        if (!msg->findInt32("channel-count", &numChannels)
                || !msg->findInt32("sample-rate", &sampleRate)) {
            err = INVALID_OPERATION;
        } else {
            err = setupAC3Codec(encoder, numChannels, sampleRate);
        }
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_EAC3)) {
        int32_t numChannels;
        int32_t sampleRate;
        if (!msg->findInt32("channel-count", &numChannels)
                || !msg->findInt32("sample-rate", &sampleRate)) {
            err = INVALID_OPERATION;
        } else {
            err = setupEAC3Codec(encoder, numChannels, sampleRate);
        }
    }

    if (err != OK) {
        return err;
    }



    if (msg->findInt32("channel-mask", &mChannelMask)) {
        mChannelMaskPresent = true;
    } else {
        mChannelMaskPresent = false;
    }

    int32_t maxInputSize;
    if (msg->findInt32("max-input-size", &maxInputSize)) {
        err = setMinBufferSize(kPortIndexInput, (size_t)maxInputSize);
    } else if (!strcmp("OMX.Nvidia.aac.decoder", mComponentName.c_str())) {
        err = setMinBufferSize(kPortIndexInput, 8192);  // XXX
    }

    int32_t priority;
    if (msg->findInt32("priority", &priority)) {
        err = setPriority(priority);
    }

    int32_t rateInt = -1;
    float rateFloat = -1;
    if (!msg->findFloat("operating-rate", &rateFloat)) {
        msg->findInt32("operating-rate", &rateInt);
        rateFloat = (float)rateInt;  // 16MHz (FLINTMAX) is OK for upper bound.
    }
    if (rateFloat > 0) {
        err = setOperatingRate(rateFloat, video);
    }

    // NOTE: both mBaseOutputFormat and mOutputFormat are outputFormat to signal first frame.
    mBaseOutputFormat = outputFormat;
    // trigger a kWhatOutputFormatChanged msg on first buffer
    mLastOutputFormat.clear();

    err = getPortFormat(kPortIndexInput, inputFormat);
    if (err == OK) {
        err = getPortFormat(kPortIndexOutput, outputFormat);
        if (err == OK) {
            mInputFormat = inputFormat;
            mOutputFormat = outputFormat;
        }
    }

    //TODO for audio, create data converters if needed
	LLOGW("TODO create data converters if needed");


    return err;
}

status_t ACodec::setPriority(int32_t priority) {
    if (priority < 0) {
        return BAD_VALUE;
    }
    OMX_PARAM_U32TYPE config;
    InitOMXParams(&config);
    config.nU32 = (OMX_U32)priority;
    status_t temp = mOMX->setConfig(
            mNode, (OMX_INDEXTYPE)OMX_IndexConfigPriority,
            &config, sizeof(config));
    if (temp != OK) {
        LLOGI("codec does not support config priority (err %d)", temp);
    }
    return OK;
}

status_t ACodec::setOperatingRate(float rateFloat, bool isVideo) {
    if (rateFloat < 0) {
        return BAD_VALUE;
    }
    OMX_U32 rate;
    if (isVideo) {
        if (rateFloat > 65535) {
            return BAD_VALUE;
        }
        rate = (OMX_U32)(rateFloat * 65536.0f + 0.5f);
    } else {
        if (rateFloat > UINT_MAX) {
            return BAD_VALUE;
        }
        rate = (OMX_U32)(rateFloat);
    }
    OMX_PARAM_U32TYPE config;
    InitOMXParams(&config);
    config.nU32 = rate;
    status_t err = mOMX->setConfig(
            mNode, (OMX_INDEXTYPE)OMX_IndexConfigOperatingRate,
            &config, sizeof(config));
    if (err != OK) {
        LLOGI("codec does not support config operating rate (err %d)", err);
    }
    return OK;
}




status_t ACodec::setMinBufferSize(OMX_U32 portIndex, size_t size) {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;

    status_t err = mOMX->getParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

    if (err != OK) {
        return err;
    }

    if (def.nBufferSize >= size) {
        return OK;
    }

    def.nBufferSize = size;

    err = mOMX->setParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

    if (err != OK) {
        return err;
    }

    err = mOMX->getParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

    if (err != OK) {
        return err;
    }

    if (def.nBufferSize < size) {
        LLOGE("failed to set min buffer size to %zu (is still %u)", size, def.nBufferSize);
        return FAILED_TRANSACTION;
    }

    return OK;
}

status_t ACodec::selectAudioPortFormat(
        OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE desiredFormat) {
    OMX_AUDIO_PARAM_PORTFORMATTYPE format;
    InitOMXParams(&format);

    format.nPortIndex = portIndex;
    for (OMX_U32 index = 0; index <= kMaxIndicesToCheck; ++index) {
        format.nIndex = index;
        status_t err = mOMX->getParameter(
                mNode, OMX_IndexParamAudioPortFormat,
                &format, sizeof(format));

        if (err != OK) {
            return err;
        }

        if (format.eEncoding == desiredFormat) {
            break;
        }

        if (index == kMaxIndicesToCheck) {
            LLOGW("[%s] stopping checking formats after %u: %s(%x)",
                    mComponentName.c_str(), index,
                    asString(format.eEncoding), format.eEncoding);
            return ERROR_UNSUPPORTED;
        }
    }

    return mOMX->setParameter(
            mNode, OMX_IndexParamAudioPortFormat, &format, sizeof(format));
}

status_t ACodec::setupAACCodec(
        bool encoder, int32_t numChannels, int32_t sampleRate,
        int32_t bitRate, int32_t aacProfile, bool isADTS, int32_t sbrMode,
        int32_t maxOutputChannelCount, const drcParams_t& drc,
        int32_t pcmLimiterEnable) {
    if (encoder && isADTS) {
        return -EINVAL;
    }

    status_t err = setupRawAudioFormat(
            encoder ? kPortIndexInput : kPortIndexOutput,
            sampleRate,
            numChannels);

    if (err != OK) {
        return err;
    }

    if (encoder) {
        err = selectAudioPortFormat(kPortIndexOutput, OMX_AUDIO_CodingAAC);

        if (err != OK) {
            return err;
        }

        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParams(&def);
        def.nPortIndex = kPortIndexOutput;

        err = mOMX->getParameter(
                mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

        if (err != OK) {
            return err;
        }

        def.format.audio.bFlagErrorConcealment = OMX_TRUE;
        def.format.audio.eEncoding = OMX_AUDIO_CodingAAC;

        err = mOMX->setParameter(
                mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

        if (err != OK) {
            return err;
        }

        OMX_AUDIO_PARAM_AACPROFILETYPE profile;
        InitOMXParams(&profile);
        profile.nPortIndex = kPortIndexOutput;

        err = mOMX->getParameter(
                mNode, OMX_IndexParamAudioAac, &profile, sizeof(profile));

        if (err != OK) {
            return err;
        }

        profile.nChannels = numChannels;

        profile.eChannelMode =
            (numChannels == 1)
                ? OMX_AUDIO_ChannelModeMono: OMX_AUDIO_ChannelModeStereo;

        profile.nSampleRate = sampleRate;
        profile.nBitRate = bitRate;
        profile.nAudioBandWidth = 0;
        profile.nFrameLength = 0;
        profile.nAACtools = OMX_AUDIO_AACToolAll;
        profile.nAACERtools = OMX_AUDIO_AACERNone;
        profile.eAACProfile = (OMX_AUDIO_AACPROFILETYPE) aacProfile;
        profile.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4FF;
        switch (sbrMode) {
        case 0:
            // disable sbr
            profile.nAACtools &= ~OMX_AUDIO_AACToolLiylSSBR;
            profile.nAACtools &= ~OMX_AUDIO_AACToolLiylDSBR;
            break;
        case 1:
            // enable single-rate sbr
            profile.nAACtools |= OMX_AUDIO_AACToolLiylSSBR;
            profile.nAACtools &= ~OMX_AUDIO_AACToolLiylDSBR;
            break;
        case 2:
            // enable dual-rate sbr
            profile.nAACtools &= ~OMX_AUDIO_AACToolLiylSSBR;
            profile.nAACtools |= OMX_AUDIO_AACToolLiylDSBR;
            break;
        case -1:
            // enable both modes -> the codec will decide which mode should be used
            profile.nAACtools |= OMX_AUDIO_AACToolLiylSSBR;
            profile.nAACtools |= OMX_AUDIO_AACToolLiylDSBR;
            break;
        default:
            // unsupported sbr mode
            return BAD_VALUE;
        }


        err = mOMX->setParameter(
                mNode, OMX_IndexParamAudioAac, &profile, sizeof(profile));

        if (err != OK) {
            return err;
        }

        return err;
    }

    OMX_AUDIO_PARAM_AACPROFILETYPE profile;
    InitOMXParams(&profile);
    profile.nPortIndex = kPortIndexInput;

    err = mOMX->getParameter(
            mNode, OMX_IndexParamAudioAac, &profile, sizeof(profile));

    if (err != OK) {
        return err;
    }

    profile.nChannels = numChannels;
    profile.nSampleRate = sampleRate;

    profile.eAACStreamFormat =
        isADTS
            ? OMX_AUDIO_AACStreamFormatMP4ADTS
            : OMX_AUDIO_AACStreamFormatMP4FF;

    OMX_AUDIO_PARAM_LIYL_AACPRESENTATIONTYPE presentation;
    InitOMXParams(&presentation);
    presentation.nMaxOutputChannels = maxOutputChannelCount;
    presentation.nDrcCut = drc.drcCut;
    presentation.nDrcBoost = drc.drcBoost;
    presentation.nHeavyCompression = drc.heavyCompression;
    presentation.nTargetReferenceLevel = drc.targetRefLevel;
    presentation.nEncodedTargetLevel = drc.encodedTargetLevel;
    presentation.nPCMLimiterEnable = pcmLimiterEnable;

    status_t res = mOMX->setParameter(mNode, OMX_IndexParamAudioAac, &profile, sizeof(profile));
    if (res == OK) {
        // optional parameters, will not cause configuration failure
        mOMX->setParameter(mNode, (OMX_INDEXTYPE)OMX_IndexParamAudioLiylAacPresentation,
                &presentation, sizeof(presentation));
    } else {
        LLOGW("did not set AudioLiylAacPresentation due to error %d when setting AudioAac", res);
    }
    return res;
}

status_t ACodec::setupAC3Codec(
        bool encoder, int32_t numChannels, int32_t sampleRate) {
    status_t err = setupRawAudioFormat(
            encoder ? kPortIndexInput : kPortIndexOutput, sampleRate, numChannels);

    if (err != OK) {
        return err;
    }

    if (encoder) {
        LLOGW("AC3 encoding is not supported.");
        return INVALID_OPERATION;
    }

    OMX_AUDIO_PARAM_LIYL_AC3TYPE def;
    InitOMXParams(&def);
    def.nPortIndex = kPortIndexInput;

    err = mOMX->getParameter(
            mNode,
            (OMX_INDEXTYPE)OMX_IndexParamAudioLiylAc3,
            &def,
            sizeof(def));

    if (err != OK) {
        return err;
    }

    def.nChannels = numChannels;
    def.nSampleRate = sampleRate;

    return mOMX->setParameter(
            mNode,
            (OMX_INDEXTYPE)OMX_IndexParamAudioLiylAc3,
            &def,
            sizeof(def));
}

status_t ACodec::setupEAC3Codec(
        bool encoder, int32_t numChannels, int32_t sampleRate) {
    status_t err = setupRawAudioFormat(
            encoder ? kPortIndexInput : kPortIndexOutput, sampleRate, numChannels);

    if (err != OK) {
        return err;
    }

    if (encoder) {
        LLOGW("EAC3 encoding is not supported.");
        return INVALID_OPERATION;
    }

    OMX_AUDIO_PARAM_LIYL_EAC3TYPE def;
    InitOMXParams(&def);
    def.nPortIndex = kPortIndexInput;

    err = mOMX->getParameter(
            mNode,
            (OMX_INDEXTYPE)OMX_IndexParamAudioLiylEac3,
            &def,
            sizeof(def));

    if (err != OK) {
        return err;
    }

    def.nChannels = numChannels;
    def.nSampleRate = sampleRate;

    return mOMX->setParameter(
            mNode,
            (OMX_INDEXTYPE)OMX_IndexParamAudioLiylEac3,
            &def,
            sizeof(def));
}

static OMX_AUDIO_AMRBANDMODETYPE pickModeFromBitRate(
        bool isAMRWB, int32_t bps) {
    if (isAMRWB) {
        if (bps <= 6600) {
            return OMX_AUDIO_AMRBandModeWB0;
        } else if (bps <= 8850) {
            return OMX_AUDIO_AMRBandModeWB1;
        } else if (bps <= 12650) {
            return OMX_AUDIO_AMRBandModeWB2;
        } else if (bps <= 14250) {
            return OMX_AUDIO_AMRBandModeWB3;
        } else if (bps <= 15850) {
            return OMX_AUDIO_AMRBandModeWB4;
        } else if (bps <= 18250) {
            return OMX_AUDIO_AMRBandModeWB5;
        } else if (bps <= 19850) {
            return OMX_AUDIO_AMRBandModeWB6;
        } else if (bps <= 23050) {
            return OMX_AUDIO_AMRBandModeWB7;
        }

        // 23850 bps
        return OMX_AUDIO_AMRBandModeWB8;
    } else {  // AMRNB
        if (bps <= 4750) {
            return OMX_AUDIO_AMRBandModeNB0;
        } else if (bps <= 5150) {
            return OMX_AUDIO_AMRBandModeNB1;
        } else if (bps <= 5900) {
            return OMX_AUDIO_AMRBandModeNB2;
        } else if (bps <= 6700) {
            return OMX_AUDIO_AMRBandModeNB3;
        } else if (bps <= 7400) {
            return OMX_AUDIO_AMRBandModeNB4;
        } else if (bps <= 7950) {
            return OMX_AUDIO_AMRBandModeNB5;
        } else if (bps <= 10200) {
            return OMX_AUDIO_AMRBandModeNB6;
        }

        // 12200 bps
        return OMX_AUDIO_AMRBandModeNB7;
    }
}


status_t ACodec::setupRawAudioFormat(
        OMX_U32 portIndex, int32_t sampleRate, int32_t numChannels, AudioEncoding encoding) {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;

    status_t err = mOMX->getParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

    if (err != OK) {
        return err;
    }

    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    err = mOMX->setParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

    if (err != OK) {
        return err;
    }

    OMX_AUDIO_PARAM_PCMMODETYPE pcmParams;
    InitOMXParams(&pcmParams);
    pcmParams.nPortIndex = portIndex;

    err = mOMX->getParameter(
            mNode, OMX_IndexParamAudioPcm, &pcmParams, sizeof(pcmParams));

    if (err != OK) {
        return err;
    }

    pcmParams.nChannels = numChannels;
    switch (encoding) {
        case kAudioEncodingPcm8bit:
            pcmParams.eNumData = OMX_NumericalDataUnsigned;
            pcmParams.nBitPerSample = 8;
            break;
        case kAudioEncodingPcmFloat:
            pcmParams.eNumData = OMX_NumericalDataFloat;
            pcmParams.nBitPerSample = 32;
            break;
        case kAudioEncodingPcm16bit:
            pcmParams.eNumData = OMX_NumericalDataSigned;
            pcmParams.nBitPerSample = 16;
            break;
        default:
            return BAD_VALUE;
    }
    pcmParams.bInterleaved = OMX_TRUE;
    pcmParams.nSamplingRate = sampleRate;
    pcmParams.ePCMMode = OMX_AUDIO_PCMModeLinear;

    if (getOMXChannelMapping(numChannels, pcmParams.eChannelMapping) != OK) {
        return OMX_ErrorNone;
    }

    err = mOMX->setParameter(
            mNode, OMX_IndexParamAudioPcm, &pcmParams, sizeof(pcmParams));
    // if we could not set up raw format to non-16-bit, try with 16-bit
    // NOTE: we will also verify this via readback, in case codec ignores these fields
    if (err != OK && encoding != kAudioEncodingPcm16bit) {
        pcmParams.eNumData = OMX_NumericalDataSigned;
        pcmParams.nBitPerSample = 16;
        err = mOMX->setParameter(
                mNode, OMX_IndexParamAudioPcm, &pcmParams, sizeof(pcmParams));
    }
    return err;
}


status_t ACodec::setVideoPortFormatType(
        OMX_U32 portIndex,
        OMX_VIDEO_CODINGTYPE compressionFormat,
        OMX_COLOR_FORMATTYPE colorFormat,
        bool usingNativeBuffers) {
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    InitOMXParams(&format);
    format.nPortIndex = portIndex;
    format.nIndex = 0;
    bool found = false;

    for (OMX_U32 index = 0; index <= kMaxIndicesToCheck; ++index) {
        format.nIndex = index;
        status_t err = mOMX->getParameter(
                mNode, OMX_IndexParamVideoPortFormat,
                &format, sizeof(format));

        if (err != OK) {
            return err;
        }

        // substitute back flexible color format to codec supported format
        OMX_U32 flexibleEquivalent;
        if (compressionFormat == OMX_VIDEO_CodingUnused
                && isFlexibleColorFormat(
                        mOMX, mNode, format.eColorFormat, usingNativeBuffers, &flexibleEquivalent)
                && colorFormat == flexibleEquivalent) {
            LLOGI("[%s] using color format %#x in place of %#x",
                    mComponentName.c_str(), format.eColorFormat, colorFormat);
            colorFormat = format.eColorFormat;
        }


        if (format.eCompressionFormat == compressionFormat
            && format.eColorFormat == colorFormat) {
            found = true;
            break;
        }

        if (index == kMaxIndicesToCheck) {
            LLOGW("[%s] stopping checking formats after %u: %s(%x)/%s(%x)",
                    mComponentName.c_str(), index,
                    asString(format.eCompressionFormat), format.eCompressionFormat,
                    asString(format.eColorFormat), format.eColorFormat);
        }
    }

    if (!found) {
        return UNKNOWN_ERROR;
    }

    status_t err = mOMX->setParameter(
            mNode, OMX_IndexParamVideoPortFormat,
            &format, sizeof(format));

    return err;
}

// Set optimal output format. OMX component lists output formats in the order
// of preference, but this got more complicated since the introduction of flexible
// YUV formats. We support a legacy behavior for applications that do not use
// surface output, do not specify an output format, but expect a "usable" standard
// OMX format. SW readable and standard formats must be flex-YUV.
//
// Suggested preference order:
// - optimal format for texture rendering (mediaplayer behavior)
// - optimal SW readable & texture renderable format (flex-YUV support)
// - optimal SW readable non-renderable format (flex-YUV bytebuffer support)
// - legacy "usable" standard formats
//
// For legacy support, we prefer a standard format, but will settle for a SW readable
// flex-YUV format.
status_t ACodec::setSupportedOutputFormat(bool getLegacyFlexibleFormat) {
    OMX_VIDEO_PARAM_PORTFORMATTYPE format, legacyFormat;
    InitOMXParams(&format);
    format.nPortIndex = kPortIndexOutput;

    InitOMXParams(&legacyFormat);
    // this field will change when we find a suitable legacy format
    legacyFormat.eColorFormat = OMX_COLOR_FormatUnused;

    for (OMX_U32 index = 0; ; ++index) {
        format.nIndex = index;
        status_t err = mOMX->getParameter(
                mNode, OMX_IndexParamVideoPortFormat,
                &format, sizeof(format));
        if (err != OK) {
            // no more formats, pick legacy format if found
            if (legacyFormat.eColorFormat != OMX_COLOR_FormatUnused) {
                 memcpy(&format, &legacyFormat, sizeof(format));
                 break;
            }
            return err;
        }
        if (format.eCompressionFormat != OMX_VIDEO_CodingUnused) {
            return OMX_ErrorBadParameter;
        }
        if (!getLegacyFlexibleFormat) {
            break;
        }
        // standard formats that were exposed to users before
        if (format.eColorFormat == OMX_COLOR_FormatYUV420Planar
                || format.eColorFormat == OMX_COLOR_FormatYUV420PackedPlanar
                || format.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar
                || format.eColorFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar
                || format.eColorFormat == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar) {
            break;
        }
        // find best legacy non-standard format
        OMX_U32 flexibleEquivalent;
        if (legacyFormat.eColorFormat == OMX_COLOR_FormatUnused
                && isFlexibleColorFormat(
                        mOMX, mNode, format.eColorFormat, false /* usingNativeBuffers */,
                        &flexibleEquivalent)
                && flexibleEquivalent == OMX_COLOR_FormatYUV420Flexible) {
            memcpy(&legacyFormat, &format, sizeof(format));
        }
    }
    return mOMX->setParameter(
            mNode, OMX_IndexParamVideoPortFormat,
            &format, sizeof(format));
}

static const struct VideoCodingMapEntry {
    const char *mMime;
    OMX_VIDEO_CODINGTYPE mVideoCodingType;
} kVideoCodingMapEntry[] = {
    { MEDIA_MIMETYPE_VIDEO_AVC, OMX_VIDEO_CodingAVC },
    { MEDIA_MIMETYPE_VIDEO_HEVC, OMX_VIDEO_CodingHEVC },
    { MEDIA_MIMETYPE_VIDEO_MPEG4, OMX_VIDEO_CodingMPEG4 },
    { MEDIA_MIMETYPE_VIDEO_H263, OMX_VIDEO_CodingH263 },
    { MEDIA_MIMETYPE_VIDEO_MPEG2, OMX_VIDEO_CodingMPEG2 },
    { MEDIA_MIMETYPE_VIDEO_VP8, OMX_VIDEO_CodingVP8 },
    { MEDIA_MIMETYPE_VIDEO_VP9, OMX_VIDEO_CodingVP9 },
    { MEDIA_MIMETYPE_VIDEO_DOLBY_VISION, OMX_VIDEO_CodingDolbyVision },
};

static status_t GetVideoCodingTypeFromMime(
        const char *mime, OMX_VIDEO_CODINGTYPE *codingType) {
    for (size_t i = 0;
         i < sizeof(kVideoCodingMapEntry) / sizeof(kVideoCodingMapEntry[0]);
         ++i) {
        if (!strcasecmp(mime, kVideoCodingMapEntry[i].mMime)) {
            *codingType = kVideoCodingMapEntry[i].mVideoCodingType;
            return OK;
        }
    }

    *codingType = OMX_VIDEO_CodingUnused;

    return ERROR_UNSUPPORTED;
}

static status_t GetMimeTypeForVideoCoding(
        OMX_VIDEO_CODINGTYPE codingType, AString *mime) {
    for (size_t i = 0;
         i < sizeof(kVideoCodingMapEntry) / sizeof(kVideoCodingMapEntry[0]);
         ++i) {
        if (codingType == kVideoCodingMapEntry[i].mVideoCodingType) {
            *mime = kVideoCodingMapEntry[i].mMime;
            return OK;
        }
    }

    mime->clear();

    return ERROR_UNSUPPORTED;
}

status_t ACodec::setPortBufferNum(OMX_U32 portIndex, int bufferNum) {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;
    status_t err;
    LLOGD("Setting [%s] %s port buffer number: %d", mComponentName.c_str(),
            portIndex == kPortIndexInput ? "input" : "output", bufferNum);
    err = mOMX->getParameter(
        mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));
    if (err != OK) {
        return err;
    }
    def.nBufferCountActual = bufferNum;
    err = mOMX->setParameter(
        mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));
    if (err != OK) {
        // Component could reject this request.
        LLOGW("Fail to set [%s] %s port buffer number: %d", mComponentName.c_str(),
            portIndex == kPortIndexInput ? "input" : "output", bufferNum);
    }
    return OK;
}

status_t ACodec::setupVideoDecoder(
        const char *mime, const sp<AMessage> &msg, bool haveNativeWindow,
        bool usingSwRenderer, sp<AMessage> &outputFormat) {
    int32_t width, height;
    if (!msg->findInt32("width", &width)
            || !msg->findInt32("height", &height)) {
        return INVALID_OPERATION;
    }

    OMX_VIDEO_CODINGTYPE compressionFormat;
    status_t err = GetVideoCodingTypeFromMime(mime, &compressionFormat);

    if (err != OK) {
        return err;
    }


    err = setVideoPortFormatType(
            kPortIndexInput, compressionFormat, OMX_COLOR_FormatUnused);

    if (err != OK) {
        return err;
    }

    int32_t tmp;
    if (msg->findInt32("color-format", &tmp)) {
        OMX_COLOR_FORMATTYPE colorFormat =
            static_cast<OMX_COLOR_FORMATTYPE>(tmp);
        err = setVideoPortFormatType(
                kPortIndexOutput, OMX_VIDEO_CodingUnused, colorFormat, haveNativeWindow);
        if (err != OK) {
            LLOGW("[%s] does not support color format %d",
                  mComponentName.c_str(), colorFormat);
            err = setSupportedOutputFormat(!haveNativeWindow /* getLegacyFlexibleFormat */);
        }
    } else {
        err = setSupportedOutputFormat(!haveNativeWindow /* getLegacyFlexibleFormat */);
    }

    if (err != OK) {
        return err;
    }

    // Set the component input buffer number to be |tmp|. If succeed,
    // component will set input port buffer number to be |tmp|. If fail,
    // component will keep the same buffer number as before.
    if (msg->findInt32("android._num-input-buffers", &tmp)) {
        err = setPortBufferNum(kPortIndexInput, tmp);
        if (err != OK)
            return err;
    }

    // Set the component output buffer number to be |tmp|. If succeed,
    // component will set output port buffer number to be |tmp|. If fail,
    // component will keep the same buffer number as before.
    if (msg->findInt32("android._num-output-buffers", &tmp)) {
        err = setPortBufferNum(kPortIndexOutput, tmp);
        if (err != OK)
            return err;
    }

    int32_t frameRateInt;
    float frameRateFloat;
    if (!msg->findFloat("frame-rate", &frameRateFloat)) {
        if (!msg->findInt32("frame-rate", &frameRateInt)) {
            frameRateInt = -1;
        }
        frameRateFloat = (float)frameRateInt;
    }

    err = setVideoFormatOnPort(
            kPortIndexInput, width, height, compressionFormat, frameRateFloat);

    if (err != OK) {
        return err;
    }

    err = setVideoFormatOnPort(
            kPortIndexOutput, width, height, OMX_VIDEO_CodingUnused);

    if (err != OK) {
        return err;
    }

    err = setColorAspectsForVideoDecoder(
            width, height, haveNativeWindow | usingSwRenderer, msg, outputFormat);
    if (err == ERROR_UNSUPPORTED) { // support is optional
        err = OK;
    }

    if (err != OK) {
        return err;
    }

    return err;
}

status_t ACodec::initDescribeColorAspectsIndex() {
    status_t err = mOMX->getExtensionIndex(
            mNode, "OMX.google.android.index.describeColorAspects", &mDescribeColorAspectsIndex);
    if (err != OK) {
        mDescribeColorAspectsIndex = (OMX_INDEXTYPE)0;
    }
    return err;
}

status_t ACodec::setCodecColorAspects(DescribeColorAspectsParams &params, bool verify) {
    status_t err = ERROR_UNSUPPORTED;
    if (mDescribeColorAspectsIndex) {
        err = mOMX->setConfig(mNode, mDescribeColorAspectsIndex, &params, sizeof(params));
    }
    LLOGV("[%s] setting color aspects (R:%d(%s), P:%d(%s), M:%d(%s), T:%d(%s)) err=%d(%s)",
            mComponentName.c_str(),
            params.sAspects.mRange, asString(params.sAspects.mRange),
            params.sAspects.mPrimaries, asString(params.sAspects.mPrimaries),
            params.sAspects.mMatrixCoeffs, asString(params.sAspects.mMatrixCoeffs),
            params.sAspects.mTransfer, asString(params.sAspects.mTransfer),
            err, asString(err));

    if (verify && err == OK) {
        err = getCodecColorAspects(params);
    }

    LLOGW_IF(err == ERROR_UNSUPPORTED && mDescribeColorAspectsIndex,
            "[%s] setting color aspects failed even though codec advertises support",
            mComponentName.c_str());
    return err;
}

status_t ACodec::setColorAspectsForVideoDecoder(
        int32_t width, int32_t height, bool usingNativeWindow,
        const sp<AMessage> &configFormat, sp<AMessage> &outputFormat) {
    DescribeColorAspectsParams params;
    InitOMXParams(&params);
    params.nPortIndex = kPortIndexOutput;

    getColorAspectsFromFormat(configFormat, params.sAspects);
    if (usingNativeWindow) {
        setDefaultCodecColorAspectsIfNeeded(params.sAspects, width, height);
        // The default aspects will be set back to the output format during the
        // getFormat phase of configure(). Set non-Unspecified values back into the
        // format, in case component does not support this enumeration.
        setColorAspectsIntoFormat(params.sAspects, outputFormat);
    }

    (void)initDescribeColorAspectsIndex();

    // communicate color aspects to codec
    return setCodecColorAspects(params);
}

status_t ACodec::getCodecColorAspects(DescribeColorAspectsParams &params) {
    status_t err = ERROR_UNSUPPORTED;
    if (mDescribeColorAspectsIndex) {
        err = mOMX->getConfig(mNode, mDescribeColorAspectsIndex, &params, sizeof(params));
    }
    LLOGV("[%s] got color aspects (R:%d(%s), P:%d(%s), M:%d(%s), T:%d(%s)) err=%d(%s)",
            mComponentName.c_str(),
            params.sAspects.mRange, asString(params.sAspects.mRange),
            params.sAspects.mPrimaries, asString(params.sAspects.mPrimaries),
            params.sAspects.mMatrixCoeffs, asString(params.sAspects.mMatrixCoeffs),
            params.sAspects.mTransfer, asString(params.sAspects.mTransfer),
            err, asString(err));
    if (params.bRequestingDataSpace) {
        LLOGV("for dataspace %#x", params.nDataSpace);
    }
    if (err == ERROR_UNSUPPORTED && mDescribeColorAspectsIndex
            && !params.bRequestingDataSpace && !params.bDataSpaceChanged) {
        LLOGW("[%s] getting color aspects failed even though codec advertises support",
                mComponentName.c_str());
    }
    return err;
}


status_t ACodec::getDataSpace(
        DescribeColorAspectsParams &params, liyl_dataspace *dataSpace /* nonnull */,
        bool tryCodec) {
    status_t err = OK;
    if (tryCodec) {
        // request dataspace guidance from codec.
        params.bRequestingDataSpace = OMX_TRUE;
        err = getCodecColorAspects(params);
        params.bRequestingDataSpace = OMX_FALSE;
        if (err == OK && params.nDataSpace != HAL_DATASPACE_UNKNOWN) {
            *dataSpace = (liyl_dataspace)params.nDataSpace;
            return err;
        } else if (err == ERROR_UNSUPPORTED) {
            // ignore not-implemented error for dataspace requests
            err = OK;
        }
    }

    // this returns legacy versions if available
    *dataSpace = getDataSpaceForColorAspects(params.sAspects, true /* mayexpand */);
    LLOGV("[%s] using color aspects (R:%d(%s), P:%d(%s), M:%d(%s), T:%d(%s)) "
          "and dataspace %#x",
            mComponentName.c_str(),
            params.sAspects.mRange, asString(params.sAspects.mRange),
            params.sAspects.mPrimaries, asString(params.sAspects.mPrimaries),
            params.sAspects.mMatrixCoeffs, asString(params.sAspects.mMatrixCoeffs),
            params.sAspects.mTransfer, asString(params.sAspects.mTransfer),
            *dataSpace);
    return err;
}


status_t ACodec::getColorAspectsAndDataSpaceForVideoDecoder(
        int32_t width, int32_t height, const sp<AMessage> &configFormat, sp<AMessage> &outputFormat,
        liyl_dataspace *dataSpace) {
    DescribeColorAspectsParams params;
    InitOMXParams(&params);
    params.nPortIndex = kPortIndexOutput;

    // reset default format and get resulting format
    getColorAspectsFromFormat(configFormat, params.sAspects);
    if (dataSpace != NULL) {
        setDefaultCodecColorAspectsIfNeeded(params.sAspects, width, height);
    }
    status_t err = setCodecColorAspects(params, true /* readBack */);

    // we always set specified aspects for decoders
    setColorAspectsIntoFormat(params.sAspects, outputFormat);

    if (dataSpace != NULL) {
        status_t res = getDataSpace(params, dataSpace, err == OK /* tryCodec */);
        if (err == OK) {
            err = res;
        }
    }

    return err;
}






static OMX_VIDEO_CONTROLRATETYPE getBitrateMode(const sp<AMessage> &msg) {
    int32_t tmp;
    if (!msg->findInt32("bitrate-mode", &tmp)) {
        return OMX_Video_ControlRateVariable;
    }

    return static_cast<OMX_VIDEO_CONTROLRATETYPE>(tmp);
}



// static
int /* OMX_VIDEO_AVCLEVELTYPE */ ACodec::getAVCLevelFor(
        int width, int height, int rate, int bitrate,
        OMX_VIDEO_AVCPROFILETYPE profile) {
    // convert bitrate to main/baseline profile kbps equivalent
    switch (profile) {
        case OMX_VIDEO_AVCProfileHigh10:
            bitrate = divUp(bitrate, 3000); break;
        case OMX_VIDEO_AVCProfileHigh:
            bitrate = divUp(bitrate, 1250); break;
        default:
            bitrate = divUp(bitrate, 1000); break;
    }

    // convert size and rate to MBs
    width = divUp(width, 16);
    height = divUp(height, 16);
    int mbs = width * height;
    rate *= mbs;
    int maxDimension = max(width, height);

    static const int limits[][5] = {
        /*   MBps     MB   dim  bitrate        level */
        {    1485,    99,  28,     64, OMX_VIDEO_AVCLevel1  },
        {    1485,    99,  28,    128, OMX_VIDEO_AVCLevel1b },
        {    3000,   396,  56,    192, OMX_VIDEO_AVCLevel11 },
        {    6000,   396,  56,    384, OMX_VIDEO_AVCLevel12 },
        {   11880,   396,  56,    768, OMX_VIDEO_AVCLevel13 },
        {   11880,   396,  56,   2000, OMX_VIDEO_AVCLevel2  },
        {   19800,   792,  79,   4000, OMX_VIDEO_AVCLevel21 },
        {   20250,  1620, 113,   4000, OMX_VIDEO_AVCLevel22 },
        {   40500,  1620, 113,  10000, OMX_VIDEO_AVCLevel3  },
        {  108000,  3600, 169,  14000, OMX_VIDEO_AVCLevel31 },
        {  216000,  5120, 202,  20000, OMX_VIDEO_AVCLevel32 },
        {  245760,  8192, 256,  20000, OMX_VIDEO_AVCLevel4  },
        {  245760,  8192, 256,  50000, OMX_VIDEO_AVCLevel41 },
        {  522240,  8704, 263,  50000, OMX_VIDEO_AVCLevel42 },
        {  589824, 22080, 420, 135000, OMX_VIDEO_AVCLevel5  },
        {  983040, 36864, 543, 240000, OMX_VIDEO_AVCLevel51 },
        { 2073600, 36864, 543, 240000, OMX_VIDEO_AVCLevel52 },
    };

    for (size_t i = 0; i < ARRAY_SIZE(limits); i++) {
        const int (&limit)[5] = limits[i];
        if (rate <= limit[0] && mbs <= limit[1] && maxDimension <= limit[2]
                && bitrate <= limit[3]) {
            return limit[4];
        }
    }
    return 0;
}




status_t ACodec::verifySupportForProfileAndLevel(
        int32_t profile, int32_t level) {
    OMX_VIDEO_PARAM_PROFILELEVELTYPE params;
    InitOMXParams(&params);
    params.nPortIndex = kPortIndexOutput;

    for (OMX_U32 index = 0; index <= kMaxIndicesToCheck; ++index) {
        params.nProfileIndex = index;
        status_t err = mOMX->getParameter(
                mNode,
                OMX_IndexParamVideoProfileLevelQuerySupported,
                &params,
                sizeof(params));

        if (err != OK) {
            return err;
        }

        int32_t supportedProfile = static_cast<int32_t>(params.eProfile);
        int32_t supportedLevel = static_cast<int32_t>(params.eLevel);

        if (profile == supportedProfile && level <= supportedLevel) {
            return OK;
        }

        if (index == kMaxIndicesToCheck) {
            LLOGW("[%s] stopping checking profiles after %u: %x/%x",
                    mComponentName.c_str(), index,
                    params.eProfile, params.eLevel);
        }
    }
    return ERROR_UNSUPPORTED;
}

status_t ACodec::configureBitrate(
        int32_t bitrate, OMX_VIDEO_CONTROLRATETYPE bitrateMode) {
    OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
    InitOMXParams(&bitrateType);
    bitrateType.nPortIndex = kPortIndexOutput;

    status_t err = mOMX->getParameter(
            mNode, OMX_IndexParamVideoBitrate,
            &bitrateType, sizeof(bitrateType));

    if (err != OK) {
        return err;
    }

    bitrateType.eControlRate = bitrateMode;
    bitrateType.nTargetBitrate = bitrate;

    return mOMX->setParameter(
            mNode, OMX_IndexParamVideoBitrate,
            &bitrateType, sizeof(bitrateType));
}

status_t ACodec::setupErrorCorrectionParameters() {
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType;
    InitOMXParams(&errorCorrectionType);
    errorCorrectionType.nPortIndex = kPortIndexOutput;

    status_t err = mOMX->getParameter(
            mNode, OMX_IndexParamVideoErrorCorrection,
            &errorCorrectionType, sizeof(errorCorrectionType));

    if (err != OK) {
        return OK;  // Optional feature. Ignore this failure
    }

    errorCorrectionType.bEnableHEC = OMX_FALSE;
    errorCorrectionType.bEnableResync = OMX_TRUE;
    errorCorrectionType.nResynchMarkerSpacing = 256;
    errorCorrectionType.bEnableDataPartitioning = OMX_FALSE;
    errorCorrectionType.bEnableRVLC = OMX_FALSE;

    return mOMX->setParameter(
            mNode, OMX_IndexParamVideoErrorCorrection,
            &errorCorrectionType, sizeof(errorCorrectionType));
}

status_t ACodec::setVideoFormatOnPort(
        OMX_U32 portIndex,
        int32_t width, int32_t height, OMX_VIDEO_CODINGTYPE compressionFormat,
        float frameRate) {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;

    OMX_VIDEO_PORTDEFINITIONTYPE *video_def = &def.format.video;

    status_t err = mOMX->getParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));
    if (err != OK) {
        return err;
    }

    if (portIndex == kPortIndexInput) {
        // XXX Need a (much) better heuristic to compute input buffer sizes.
        const size_t X = 64 * 1024;
        if (def.nBufferSize < X) {
            def.nBufferSize = X;
        }
    }

    if (def.eDomain != OMX_PortDomainVideo) {
        LLOGE("expected video port, got %s(%d)", asString(def.eDomain), def.eDomain);
        return FAILED_TRANSACTION;
    }

    video_def->nFrameWidth = width;
    video_def->nFrameHeight = height;

    if (portIndex == kPortIndexInput) {
        video_def->eCompressionFormat = compressionFormat;
        video_def->eColorFormat = OMX_COLOR_FormatUnused;
        if (frameRate >= 0) {
            video_def->xFramerate = (OMX_U32)(frameRate * 65536.0f);
        }
    }

    err = mOMX->setParameter(
            mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));

    return err;
}


size_t ACodec::countBuffersOwnedByComponent(OMX_U32 portIndex) const {
    size_t n = 0;

    for (size_t i = 0; i < mBuffers[portIndex].size(); ++i) {
        const BufferInfo &info = mBuffers[portIndex].itemAt(i);

        if (info.mStatus == BufferInfo::OWNED_BY_COMPONENT) {
            ++n;
        }
    }

    return n;
}

size_t ACodec::countBuffersOwnedByNativeWindow() const {
    size_t n = 0;

    for (size_t i = 0; i < mBuffers[kPortIndexOutput].size(); ++i) {
        const BufferInfo &info = mBuffers[kPortIndexOutput].itemAt(i);

        if (info.mStatus == BufferInfo::OWNED_BY_NATIVE_WINDOW) {
            ++n;
        }
    }

    return n;
}

bool ACodec::allYourBuffersAreBelongToUs(
        OMX_U32 portIndex) {
    for (size_t i = 0; i < mBuffers[portIndex].size(); ++i) {
        BufferInfo *info = &mBuffers[portIndex].editItemAt(i);

        if (info->mStatus != BufferInfo::OWNED_BY_US
                && info->mStatus != BufferInfo::OWNED_BY_NATIVE_WINDOW) {
            LLOGV("[%s] Buffer %u on port %u still has status %d",
                    mComponentName.c_str(),
                    info->mBufferID, portIndex, info->mStatus);
            return false;
        }
    }

    return true;
}

bool ACodec::allYourBuffersAreBelongToUs() {
    return allYourBuffersAreBelongToUs(kPortIndexInput)
        && allYourBuffersAreBelongToUs(kPortIndexOutput);
}

void ACodec::deferMessage(const sp<AMessage> &msg) {
    mDeferredQueue.push_back(msg);
}

void ACodec::processDeferredMessages() {
    List<sp<AMessage> > queue = mDeferredQueue;
    mDeferredQueue.clear();

    List<sp<AMessage> >::iterator it = queue.begin();
    while (it != queue.end()) {
        onMessageReceived(*it++);
    }
}

// static
bool ACodec::describeDefaultColorFormat(DescribeColorFormat2Params &params) {
    MediaImage2 &image = params.sMediaImage;
    memset(&image, 0, sizeof(image));

    image.mType = MediaImage2::MEDIA_IMAGE_TYPE_UNKNOWN;
    image.mNumPlanes = 0;

    const OMX_COLOR_FORMATTYPE fmt = params.eColorFormat;
    image.mWidth = params.nFrameWidth;
    image.mHeight = params.nFrameHeight;

    // only supporting YUV420
    if (fmt != OMX_COLOR_FormatYUV420Planar &&
        fmt != OMX_COLOR_FormatYUV420PackedPlanar &&
        fmt != OMX_COLOR_FormatYUV420SemiPlanar &&
        fmt != OMX_COLOR_FormatYUV420PackedSemiPlanar &&
        fmt != (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YV12) {
        LLOGW("do not know color format 0x%x = %d", fmt, fmt);
        return false;
    }

    // TEMPORARY FIX for some vendors that advertise sliceHeight as 0
    if (params.nStride != 0 && params.nSliceHeight == 0) {
        LLOGW("using sliceHeight=%u instead of what codec advertised (=0)",
                params.nFrameHeight);
        params.nSliceHeight = params.nFrameHeight;
    }

    // we need stride and slice-height to be non-zero and sensible. These values were chosen to
    // prevent integer overflows further down the line, and do not indicate support for
    // 32kx32k video.
    if (params.nStride == 0 || params.nSliceHeight == 0
            || params.nStride > 32768 || params.nSliceHeight > 32768) {
        LLOGW("cannot describe color format 0x%x = %d with stride=%u and sliceHeight=%u",
                fmt, fmt, params.nStride, params.nSliceHeight);
        return false;
    }

    // set-up YUV format
    image.mType = MediaImage2::MEDIA_IMAGE_TYPE_YUV;
    image.mNumPlanes = 3;
    image.mBitDepth = 8;
    image.mBitDepthAllocated = 8;
    image.mPlane[image.Y].mOffset = 0;
    image.mPlane[image.Y].mColInc = 1;
    image.mPlane[image.Y].mRowInc = params.nStride;
    image.mPlane[image.Y].mHorizSubsampling = 1;
    image.mPlane[image.Y].mVertSubsampling = 1;

    switch ((int)fmt) {
        case HAL_PIXEL_FORMAT_YV12:
            if (params.bUsingNativeBuffers) {
                size_t ystride = align(params.nStride, 16);
                size_t cstride = align(params.nStride / 2, 16);
                image.mPlane[image.Y].mRowInc = ystride;

                image.mPlane[image.V].mOffset = ystride * params.nSliceHeight;
                image.mPlane[image.V].mColInc = 1;
                image.mPlane[image.V].mRowInc = cstride;
                image.mPlane[image.V].mHorizSubsampling = 2;
                image.mPlane[image.V].mVertSubsampling = 2;

                image.mPlane[image.U].mOffset = image.mPlane[image.V].mOffset
                        + (cstride * params.nSliceHeight / 2);
                image.mPlane[image.U].mColInc = 1;
                image.mPlane[image.U].mRowInc = cstride;
                image.mPlane[image.U].mHorizSubsampling = 2;
                image.mPlane[image.U].mVertSubsampling = 2;
                break;
            } else {
                // fall through as YV12 is used for YUV420Planar by some codecs
            }

        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420PackedPlanar:
            image.mPlane[image.U].mOffset = params.nStride * params.nSliceHeight;
            image.mPlane[image.U].mColInc = 1;
            image.mPlane[image.U].mRowInc = params.nStride / 2;
            image.mPlane[image.U].mHorizSubsampling = 2;
            image.mPlane[image.U].mVertSubsampling = 2;

            image.mPlane[image.V].mOffset = image.mPlane[image.U].mOffset
                    + (params.nStride * params.nSliceHeight / 4);
            image.mPlane[image.V].mColInc = 1;
            image.mPlane[image.V].mRowInc = params.nStride / 2;
            image.mPlane[image.V].mHorizSubsampling = 2;
            image.mPlane[image.V].mVertSubsampling = 2;
            break;

        case OMX_COLOR_FormatYUV420SemiPlanar:
            // FIXME: NV21 for sw-encoder, NV12 for decoder and hw-encoder
        case OMX_COLOR_FormatYUV420PackedSemiPlanar:
            // NV12
            image.mPlane[image.U].mOffset = params.nStride * params.nSliceHeight;
            image.mPlane[image.U].mColInc = 2;
            image.mPlane[image.U].mRowInc = params.nStride;
            image.mPlane[image.U].mHorizSubsampling = 2;
            image.mPlane[image.U].mVertSubsampling = 2;

            image.mPlane[image.V].mOffset = image.mPlane[image.U].mOffset + 1;
            image.mPlane[image.V].mColInc = 2;
            image.mPlane[image.V].mRowInc = params.nStride;
            image.mPlane[image.V].mHorizSubsampling = 2;
            image.mPlane[image.V].mVertSubsampling = 2;
            break;

        default:
            TRESPASS();
    }
    return true;
}

// static
bool ACodec::describeColorFormat(
        const sp<IOMX> &omx, IOMX::node_id node,
        DescribeColorFormat2Params &describeParams)
{
    OMX_INDEXTYPE describeColorFormatIndex;
    if (omx->getExtensionIndex(
            node, "OMX.google.android.index.describeColorFormat",
            &describeColorFormatIndex) == OK) {
		LLOGE("don't support OMX.google.android.index.describeColorFormat");
		return false; 

    } else if (omx->getExtensionIndex(
            node, "OMX.google.android.index.describeColorFormat2", &describeColorFormatIndex) == OK
               && omx->getParameter(
            node, describeColorFormatIndex, &describeParams, sizeof(describeParams)) == OK) {
		LLOGE("don't support OMX.google.android.index.describeColorFormat2");
		return false; 
    }

    return describeDefaultColorFormat(describeParams);
}

// static
bool ACodec::isFlexibleColorFormat(
         const sp<IOMX> &omx, IOMX::node_id node,
         uint32_t colorFormat, bool usingNativeBuffers, OMX_U32 *flexibleEquivalent) {
    DescribeColorFormat2Params describeParams;
    InitOMXParams(&describeParams);
    describeParams.eColorFormat = (OMX_COLOR_FORMATTYPE)colorFormat;
    // reasonable dummy values
    describeParams.nFrameWidth = 128;
    describeParams.nFrameHeight = 128;
    describeParams.nStride = 128;
    describeParams.nSliceHeight = 128;
    describeParams.bUsingNativeBuffers = (OMX_BOOL)usingNativeBuffers;

    CHECK(flexibleEquivalent != NULL);

    if (!describeColorFormat(omx, node, describeParams)) {
        return false;
    }

    const MediaImage2 &img = describeParams.sMediaImage;
    if (img.mType == MediaImage2::MEDIA_IMAGE_TYPE_YUV) {
        if (img.mNumPlanes != 3
                || img.mPlane[img.Y].mHorizSubsampling != 1
                || img.mPlane[img.Y].mVertSubsampling != 1) {
            return false;
        }

        // YUV 420
        if (img.mPlane[img.U].mHorizSubsampling == 2
                && img.mPlane[img.U].mVertSubsampling == 2
                && img.mPlane[img.V].mHorizSubsampling == 2
                && img.mPlane[img.V].mVertSubsampling == 2) {
            // possible flexible YUV420 format
            if (img.mBitDepth <= 8) {
               *flexibleEquivalent = OMX_COLOR_FormatYUV420Flexible;
               return true;
            }
        }
    }
    return false;
}

status_t ACodec::getPortFormat(OMX_U32 portIndex, sp<AMessage> &notify) {
    const char *niceIndex = portIndex == kPortIndexInput ? "input" : "output";
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;

    status_t err = mOMX->getParameter(mNode, OMX_IndexParamPortDefinition, &def, sizeof(def));
    if (err != OK) {
        return err;
    }

    if (def.eDir != (portIndex == kPortIndexOutput ? OMX_DirOutput : OMX_DirInput)) {
        LLOGE("unexpected dir: %s(%d) on %s port", asString(def.eDir), def.eDir, niceIndex);
        return BAD_VALUE;
    }

    switch (def.eDomain) {
        case OMX_PortDomainVideo:
        {
            OMX_VIDEO_PORTDEFINITIONTYPE *videoDef = &def.format.video;
            switch ((int)videoDef->eCompressionFormat) {
                case OMX_VIDEO_CodingUnused:
                {
                    CHECK(mIsEncoder ^ (portIndex == kPortIndexOutput));
                    notify->setString("mime", MEDIA_MIMETYPE_VIDEO_RAW);

                    notify->setInt32("stride", videoDef->nStride);
                    notify->setInt32("slice-height", videoDef->nSliceHeight);
                    notify->setInt32("color-format", videoDef->eColorFormat);



                    int32_t width = (int32_t)videoDef->nFrameWidth;
                    int32_t height = (int32_t)videoDef->nFrameHeight;

                    if (portIndex == kPortIndexOutput) {
                        OMX_CONFIG_RECTTYPE rect;
                        InitOMXParams(&rect);
                        rect.nPortIndex = portIndex;

                        if (mOMX->getConfig(
                                    mNode,
                                    (portIndex == kPortIndexOutput ?
                                            OMX_IndexConfigCommonOutputCrop :
                                            OMX_IndexConfigCommonInputCrop),
                                    &rect, sizeof(rect)) != OK) {
                            rect.nLeft = 0;
                            rect.nTop = 0;
                            rect.nWidth = videoDef->nFrameWidth;
                            rect.nHeight = videoDef->nFrameHeight;
                        }

                        if (rect.nLeft < 0 ||
                            rect.nTop < 0 ||
                            rect.nLeft + rect.nWidth > videoDef->nFrameWidth ||
                            rect.nTop + rect.nHeight > videoDef->nFrameHeight) {
                            LLOGE("Wrong cropped rect (%d, %d) - (%u, %u) vs. frame (%u, %u)",
                                    rect.nLeft, rect.nTop,
                                    rect.nLeft + rect.nWidth, rect.nTop + rect.nHeight,
                                    videoDef->nFrameWidth, videoDef->nFrameHeight);
                            return BAD_VALUE;
                        }

                        notify->setRect(
                                "crop",
                                rect.nLeft,
                                rect.nTop,
                                rect.nLeft + rect.nWidth - 1,
                                rect.nTop + rect.nHeight - 1);

                        width = rect.nWidth;
                        height = rect.nHeight;

                        liyl_dataspace dataSpace = HAL_DATASPACE_UNKNOWN;
                        (void)getColorAspectsAndDataSpaceForVideoDecoder(
                                width, height, mConfigFormat, notify,
                                mUsingNativeWindow ? &dataSpace : NULL);
                    }

                    break;
                }

                
                default:
                {
                    if (mIsEncoder ^ (portIndex == kPortIndexOutput)) {
                        // should be CodingUnused
                        LLOGE("Raw port video compression format is %s(%d)",
                                asString(videoDef->eCompressionFormat),
                                videoDef->eCompressionFormat);
                        return BAD_VALUE;
                    }
                    AString mime;
                    if (GetMimeTypeForVideoCoding(
                        videoDef->eCompressionFormat, &mime) != OK) {
                        notify->setString("mime", "application/octet-stream");
                    } else {
                        notify->setString("mime", mime.c_str());
                    }

                    break;
                }
            }
            notify->setInt32("width", videoDef->nFrameWidth);
            notify->setInt32("height", videoDef->nFrameHeight);
            LLOGV("[%s] %s format is %s", mComponentName.c_str(),
                    portIndex == kPortIndexInput ? "input" : "output",
                    notify->debugString().c_str());

            break;
        }

        case OMX_PortDomainAudio:
        {
            OMX_AUDIO_PORTDEFINITIONTYPE *audioDef = &def.format.audio;

            switch ((int)audioDef->eEncoding) {
                case OMX_AUDIO_CodingPCM:
                {
                    OMX_AUDIO_PARAM_PCMMODETYPE params;
                    InitOMXParams(&params);
                    params.nPortIndex = portIndex;

                    err = mOMX->getParameter(
                            mNode, OMX_IndexParamAudioPcm, &params, sizeof(params));
                    if (err != OK) {
                        return err;
                    }

                    if (params.nChannels <= 0
                            || (params.nChannels != 1 && !params.bInterleaved)
                            || params.ePCMMode != OMX_AUDIO_PCMModeLinear) {
                        LLOGE("unsupported PCM port: %u channels%s, %u-bit",
                                params.nChannels,
                                params.bInterleaved ? " interleaved" : "",
                                params.nBitPerSample);
                        return FAILED_TRANSACTION;
                    }

                    notify->setString("mime", MEDIA_MIMETYPE_AUDIO_RAW);
                    notify->setInt32("channel-count", params.nChannels);
                    notify->setInt32("sample-rate", params.nSamplingRate);

                    AudioEncoding encoding = kAudioEncodingPcm16bit;
                    if (params.eNumData == OMX_NumericalDataUnsigned
                            && params.nBitPerSample == 8u) {
                        encoding = kAudioEncodingPcm8bit;
                    } else if (params.eNumData == OMX_NumericalDataFloat
                            && params.nBitPerSample == 32u) {
                        encoding = kAudioEncodingPcmFloat;
                    } else if (params.nBitPerSample != 16u
                            || params.eNumData != OMX_NumericalDataSigned) {
                        LLOGE("unsupported PCM port: %s(%d), %s(%d) mode ",
                                asString(params.eNumData), params.eNumData,
                                asString(params.ePCMMode), params.ePCMMode);
                        return FAILED_TRANSACTION;
                    }
                    notify->setInt32("pcm-encoding", encoding);

                    if (mChannelMaskPresent) {
                        notify->setInt32("channel-mask", mChannelMask);
                    }
                    break;
                }

                case OMX_AUDIO_CodingAAC:
                {
                    OMX_AUDIO_PARAM_AACPROFILETYPE params;
                    InitOMXParams(&params);
                    params.nPortIndex = portIndex;

                    err = mOMX->getParameter(
                            mNode, OMX_IndexParamAudioAac, &params, sizeof(params));
                    if (err != OK) {
                        return err;
                    }

                    notify->setString("mime", MEDIA_MIMETYPE_AUDIO_AAC);
                    notify->setInt32("channel-count", params.nChannels);
                    notify->setInt32("sample-rate", params.nSampleRate);
                    break;
                }

                case OMX_AUDIO_CodingLiylAC3:
                {
                    OMX_AUDIO_PARAM_LIYL_AC3TYPE params;
                    InitOMXParams(&params);
                    params.nPortIndex = portIndex;

                    err = mOMX->getParameter(
                            mNode, (OMX_INDEXTYPE)OMX_IndexParamAudioLiylAc3,
                            &params, sizeof(params));
                    if (err != OK) {
                        return err;
                    }

                    notify->setString("mime", MEDIA_MIMETYPE_AUDIO_AC3);
                    notify->setInt32("channel-count", params.nChannels);
                    notify->setInt32("sample-rate", params.nSampleRate);
                    break;
                }

                case OMX_AUDIO_CodingLiylEAC3:
                {
                    OMX_AUDIO_PARAM_LIYL_EAC3TYPE params;
                    InitOMXParams(&params);
                    params.nPortIndex = portIndex;

                    err = mOMX->getParameter(
                            mNode, (OMX_INDEXTYPE)OMX_IndexParamAudioLiylEac3,
                            &params, sizeof(params));
                    if (err != OK) {
                        return err;
                    }

                    notify->setString("mime", MEDIA_MIMETYPE_AUDIO_EAC3);
                    notify->setInt32("channel-count", params.nChannels);
                    notify->setInt32("sample-rate", params.nSampleRate);
                    break;
                }

                default:
                    LLOGE("Unsupported audio coding: %s(%d)\n",
                            asString(audioDef->eEncoding), audioDef->eEncoding);
                    return BAD_TYPE;
            }
            break;
        }

        default:
            LLOGE("Unsupported domain: %s(%d)", asString(def.eDomain), def.eDomain);
            return BAD_TYPE;
    }

    return OK;
}

void ACodec::onDataSpaceChanged(liyl_dataspace dataSpace, const ColorAspects &aspects) {
    // aspects are normally communicated in ColorAspects
    int32_t range, standard, transfer;
    convertCodecColorAspectsToPlatformAspects(aspects, &range, &standard, &transfer);

    // if some aspects are unspecified, use dataspace fields
    if (range != 0) {
        range = (dataSpace & HAL_DATASPACE_RANGE_MASK) >> HAL_DATASPACE_RANGE_SHIFT;
    }
    if (standard != 0) {
        standard = (dataSpace & HAL_DATASPACE_STANDARD_MASK) >> HAL_DATASPACE_STANDARD_SHIFT;
    }
    if (transfer != 0) {
        transfer = (dataSpace & HAL_DATASPACE_TRANSFER_MASK) >> HAL_DATASPACE_TRANSFER_SHIFT;
    }

    mOutputFormat = mOutputFormat->dup(); // trigger an output format changed event
    if (range != 0) {
        mOutputFormat->setInt32("color-range", range);
    }
    if (standard != 0) {
        mOutputFormat->setInt32("color-standard", standard);
    }
    if (transfer != 0) {
        mOutputFormat->setInt32("color-transfer", transfer);
    }

    LLOGD("dataspace changed to %#x (R:%d(%s), P:%d(%s), M:%d(%s), T:%d(%s)) "
          "(R:%d(%s), S:%d(%s), T:%d(%s))",
            dataSpace,
            aspects.mRange, asString(aspects.mRange),
            aspects.mPrimaries, asString(aspects.mPrimaries),
            aspects.mMatrixCoeffs, asString(aspects.mMatrixCoeffs),
            aspects.mTransfer, asString(aspects.mTransfer),
            range, asString((ColorRange)range),
            standard, asString((ColorStandard)standard),
            transfer, asString((ColorTransfer)transfer));
}

void ACodec::onOutputFormatChanged(sp<const AMessage> expectedFormat) {
    // store new output format, at the same time mark that this is no longer the first frame
    mOutputFormat = mBaseOutputFormat->dup();

    if (getPortFormat(kPortIndexOutput, mOutputFormat) != OK) {
        LLOGE("[%s] Failed to get port format to send format change", mComponentName.c_str());
        return;
    }

    if (expectedFormat != NULL) {
        sp<const AMessage> changes = expectedFormat->changesFrom(mOutputFormat);
        sp<const AMessage> to = mOutputFormat->changesFrom(expectedFormat);
        if (changes->countEntries() != 0 || to->countEntries() != 0) {
            LLOGW("[%s] BAD CODEC: Output format changed unexpectedly from (diff) %s to (diff) %s",
                    mComponentName.c_str(),
                    changes->debugString(4).c_str(), to->debugString(4).c_str());
        }
    }

    if (!mIsVideo && !mIsEncoder) {
		//TODO
        LLOGW("TODO mybe need converter");
    }

}


void ACodec::sendFormatChange() {
    AString mime;
    CHECK(mOutputFormat->findString("mime", &mime));

    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", kWhatOutputFormatChanged);
    notify->setMessage("format", mOutputFormat);
    notify->post();

    // mLastOutputFormat is not used when tunneled; doing this just to stay consistent
    mLastOutputFormat = mOutputFormat;
}

void ACodec::signalError(OMX_ERRORTYPE error, status_t internalError) {
    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", CodecBase::kWhatError);
    LLOGE("signalError(omxError %#x, internalError %d)", error, internalError);

    if (internalError == UNKNOWN_ERROR) { // find better error code
        const status_t omxStatus = statusFromOMXError(error);
        if (omxStatus != 0) {
            internalError = omxStatus;
        } else {
            LLOGW("Invalid OMX error %#x", error);
        }
    }

    mFatalError = true;

    notify->setInt32("err", internalError);
    notify->setInt32("actionCode", ACTION_CODE_FATAL); // could translate from OMX error.
    notify->post();
}

////////////////////////////////////////////////////////////////////////////////

ACodec::PortDescription::PortDescription() {
}


void ACodec::PortDescription::addBuffer(
        IOMX::buffer_id id, const sp<ABuffer> &buffer) {
    mBufferIDs.push_back(id);
    mBuffers.push_back(buffer);
}

size_t ACodec::PortDescription::countBuffers() {
    return mBufferIDs.size();
}

IOMX::buffer_id ACodec::PortDescription::bufferIDAt(size_t index) const {
    return mBufferIDs.itemAt(index);
}

sp<ABuffer> ACodec::PortDescription::bufferAt(size_t index) const {
    return mBuffers.itemAt(index);
}


////////////////////////////////////////////////////////////////////////////////

ACodec::BaseState::BaseState(ACodec *codec, const sp<AState> &parentState)
    : AState(parentState),
      mCodec(codec) {
}

ACodec::BaseState::PortMode ACodec::BaseState::getPortMode(
        OMX_U32 /* portIndex */) {
    return KEEP_BUFFERS;
}

bool ACodec::BaseState::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatInputBufferFilled:
        {
            onInputBufferFilled(msg);
            break;
        }

        case kWhatOutputBufferDrained:
        {
            onOutputBufferDrained(msg);
            break;
        }

        case ACodec::kWhatOMXMessageList:
        {
            return checkOMXMessage(msg) ? onOMXMessageList(msg) : true;
        }

        case ACodec::kWhatOMXMessageItem:
        {
            // no need to check as we already did it for kWhatOMXMessageList
            return onOMXMessage(msg);
        }

        case ACodec::kWhatOMXMessage:
        {
            return checkOMXMessage(msg) ? onOMXMessage(msg) : true;
        }

        case ACodec::kWhatSignalEndOfInputStream:
        {
            // This may result in an app illegal state exception.
            LLOGE("Message 0x%x was not handled", msg->what());
            mCodec->signalError(OMX_ErrorUndefined, INVALID_OPERATION);
            return true;
        }

        case ACodec::kWhatReleaseCodecInstance:
        {
            LLOGI("[%s] forcing the release of codec",
                    mCodec->mComponentName.c_str());
            status_t err = mCodec->mOMX->freeNode(mCodec->mNode);
            LLOGE_IF("[%s] failed to release codec instance: err=%d",
                       mCodec->mComponentName.c_str(), err);
            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatShutdownCompleted);
            notify->post();
            break;
        }

        default:
            return false;
    }

    return true;
}

bool ACodec::BaseState::checkOMXMessage(const sp<AMessage> &msg) {
    // there is a possibility that this is an outstanding message for a
    // codec that we have already destroyed
    if (mCodec->mNode == 0) {
        LLOGI("ignoring message as already freed component: %s",
                msg->debugString().c_str());
        return false;
    }

    IOMX::node_id nodeID;
    CHECK(msg->findInt32("node", (int32_t*)&nodeID));
    if (nodeID != mCodec->mNode) {
        LLOGE("Unexpected message for nodeID: %u, should have been %u", nodeID, mCodec->mNode);
        return false;
    }
    return true;
}

bool ACodec::BaseState::onOMXMessageList(const sp<AMessage> &msg) {
    sp<RefBase> obj;
    CHECK(msg->findObject("messages", &obj));
    sp<MessageList> msgList = static_cast<MessageList *>(obj.get());

    bool receivedRenderedEvents = false;
    for (std::list<sp<AMessage>>::const_iterator it = msgList->getList().cbegin();
          it != msgList->getList().cend(); ++it) {
        (*it)->setWhat(ACodec::kWhatOMXMessageItem);
        mCodec->handleMessage(*it);
        int32_t type;
        CHECK((*it)->findInt32("type", &type));
        if (type == omx_message::FRAME_RENDERED) {
            receivedRenderedEvents = true;
        }
    }

    if (receivedRenderedEvents) {
		//TODO
        // NOTE: all buffers are rendered in this case
        //mCodec->notifyOfRenderedFrames();
    }
    return true;
}

bool ACodec::BaseState::onOMXMessage(const sp<AMessage> &msg) {
    int32_t type;
    CHECK(msg->findInt32("type", &type));

    switch (type) {
        case omx_message::EVENT:
        {
            int32_t event, data1, data2;
            CHECK(msg->findInt32("event", &event));
            CHECK(msg->findInt32("data1", &data1));
            CHECK(msg->findInt32("data2", &data2));

            if (event == OMX_EventCmdComplete
                    && data1 == OMX_CommandFlush
                    && data2 == (int32_t)OMX_ALL) {
                // Use of this notification is not consistent across
                // implementations. We'll drop this notification and rely
                // on flush-complete notifications on the individual port
                // indices instead.

                return true;
            }

            return onOMXEvent(
                    static_cast<OMX_EVENTTYPE>(event),
                    static_cast<OMX_U32>(data1),
                    static_cast<OMX_U32>(data2));
        }

        case omx_message::EMPTY_BUFFER_DONE:
        {
            IOMX::buffer_id bufferID;

            CHECK(msg->findInt32("buffer", (int32_t*)&bufferID));

            return onOMXEmptyBufferDone(bufferID);
        }

        case omx_message::FILL_BUFFER_DONE:
        {
            IOMX::buffer_id bufferID;
            CHECK(msg->findInt32("buffer", (int32_t*)&bufferID));

            int32_t rangeOffset, rangeLength, flags;
            int64_t timeUs;

            CHECK(msg->findInt32("range_offset", &rangeOffset));
            CHECK(msg->findInt32("range_length", &rangeLength));
            CHECK(msg->findInt32("flags", &flags));
            CHECK(msg->findInt64("timestamp", &timeUs));

            return onOMXFillBufferDone(
                    bufferID,
                    (size_t)rangeOffset, (size_t)rangeLength,
                    (OMX_U32)flags,
                    timeUs);
        }

        case omx_message::FRAME_RENDERED:
        {
            int64_t mediaTimeUs, systemNano;

            CHECK(msg->findInt64("media_time_us", &mediaTimeUs));
            CHECK(msg->findInt64("system_nano", &systemNano));

            return onOMXFrameRendered(
                    mediaTimeUs, systemNano);
        }

        default:
            LLOGE("Unexpected message type: %d", type);
            return false;
    }
}

bool ACodec::BaseState::onOMXFrameRendered(
        int64_t mediaTimeUs __unused, nsecs_t systemNano __unused) {
    // ignore outside of Executing and PortSettingsChanged states
    return true;
}

bool ACodec::BaseState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    if (event == OMX_EventDataSpaceChanged) {
        ColorAspects aspects;
        aspects.mRange = (ColorAspects::Range)((data2 >> 24) & 0xFF);
        aspects.mPrimaries = (ColorAspects::Primaries)((data2 >> 16) & 0xFF);
        aspects.mMatrixCoeffs = (ColorAspects::MatrixCoeffs)((data2 >> 8) & 0xFF);
        aspects.mTransfer = (ColorAspects::Transfer)(data2 & 0xFF);

        mCodec->onDataSpaceChanged((liyl_dataspace)data1, aspects);
        return true;
    }

    if (event != OMX_EventError) {
        LLOGV("[%s] EVENT(%d, 0x%08x, 0x%08x)",
             mCodec->mComponentName.c_str(), event, data1, data2);

        return false;
    }

    LLOGE("[%s] ERROR(0x%08x)", mCodec->mComponentName.c_str(), data1);

    // verify OMX component sends back an error we expect.
    OMX_ERRORTYPE omxError = (OMX_ERRORTYPE)data1;
    if (!isOMXError(omxError)) {
        LLOGW("Invalid OMX error %#x", omxError);
        omxError = OMX_ErrorUndefined;
    }
    mCodec->signalError(omxError);

    return true;
}

bool ACodec::BaseState::onOMXEmptyBufferDone(IOMX::buffer_id bufferID) {
    LLOGV("[%s] onOMXEmptyBufferDone %u",
         mCodec->mComponentName.c_str(), bufferID);

    BufferInfo *info = mCodec->findBufferByID(kPortIndexInput, bufferID);
    BufferInfo::Status status = BufferInfo::getSafeStatus(info);
    if (status != BufferInfo::OWNED_BY_COMPONENT) {
        LLOGE("Wrong ownership in EBD: %s(%d) buffer #%u", _asString(status), status, bufferID);
        mCodec->dumpBuffers(kPortIndexInput);
        return false;
    }
    info->mStatus = BufferInfo::OWNED_BY_US;


    PortMode mode = getPortMode(kPortIndexInput);

    switch (mode) {
        case KEEP_BUFFERS:
            break;

        case RESUBMIT_BUFFERS:
            postFillThisBuffer(info);
            break;

        case FREE_BUFFERS:
        default:
            LLOGE("SHOULD NOT REACH HERE: cannot free empty output buffers");
            return false;
    }

    return true;
}

void ACodec::BaseState::postFillThisBuffer(BufferInfo *info) {
    if (mCodec->mPortEOS[kPortIndexInput]) {
        return;
    }

    CHECK_EQ((int)info->mStatus, (int)BufferInfo::OWNED_BY_US);

    sp<AMessage> notify = mCodec->mNotify->dup();
    notify->setInt32("what", CodecBase::kWhatFillThisBuffer);
    notify->setInt32("buffer-id", info->mBufferID);

    info->mData->meta()->clear();
    notify->setBuffer("buffer", info->mData);

    sp<AMessage> reply = new AMessage(kWhatInputBufferFilled, mCodec);
    reply->setInt32("buffer-id", info->mBufferID);

    notify->setMessage("reply", reply);

    notify->post();

    info->mStatus = BufferInfo::OWNED_BY_UPSTREAM;
}

void ACodec::BaseState::onInputBufferFilled(const sp<AMessage> &msg) {
    IOMX::buffer_id bufferID;
    CHECK(msg->findInt32("buffer-id", (int32_t*)&bufferID));
    sp<ABuffer> buffer;
    int32_t err = OK;
    bool eos = false;
    PortMode mode = getPortMode(kPortIndexInput);

    if (!msg->findBuffer("buffer", &buffer)) {
        /* these are unfilled buffers returned by client */
        CHECK(msg->findInt32("err", &err));

        if (err == OK) {
            /* buffers with no errors are returned on MediaCodec.flush */
            mode = KEEP_BUFFERS;
        } else {
            LLOGV("[%s] saw error %d instead of an input buffer",
                 mCodec->mComponentName.c_str(), err);
            eos = true;
        }

        buffer.clear();
    }

    int32_t tmp;
    if (buffer != NULL && buffer->meta()->findInt32("eos", &tmp) && tmp) {
        eos = true;
        err = ERROR_END_OF_STREAM;
    }

    BufferInfo *info = mCodec->findBufferByID(kPortIndexInput, bufferID);
    BufferInfo::Status status = BufferInfo::getSafeStatus(info);
    if (status != BufferInfo::OWNED_BY_UPSTREAM) {
        LLOGE("Wrong ownership in IBF: %s(%d) buffer #%u", _asString(status), status, bufferID);
        mCodec->dumpBuffers(kPortIndexInput);
        mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
        return;
    }

    info->mStatus = BufferInfo::OWNED_BY_US;

    switch (mode) {
        case KEEP_BUFFERS:
        {
            if (eos) {
                if (!mCodec->mPortEOS[kPortIndexInput]) {
                    mCodec->mPortEOS[kPortIndexInput] = true;
                    mCodec->mInputEOSResult = err;
                }
            }
            break;
        }

        case RESUBMIT_BUFFERS:
        {
            if (buffer != NULL && !mCodec->mPortEOS[kPortIndexInput]) {
                // Do not send empty input buffer w/o EOS to the component.
                if (buffer->size() == 0 && !eos) {
                    postFillThisBuffer(info);
                    break;
                }

                int64_t timeUs;
                CHECK(buffer->meta()->findInt64("timeUs", &timeUs));

                OMX_U32 flags = OMX_BUFFERFLAG_ENDOFFRAME;

                int32_t isCSD = 0;
                if (buffer->meta()->findInt32("csd", &isCSD) && isCSD != 0) {
                    flags |= OMX_BUFFERFLAG_CODECCONFIG;
                }

                if (eos) {
                    flags |= OMX_BUFFERFLAG_EOS;
                }


                if (flags & OMX_BUFFERFLAG_CODECCONFIG) {
                    LLOGV("[%s] calling emptyBuffer %u w/ codec specific data",
                         mCodec->mComponentName.c_str(), bufferID);
                    //hexdump(buffer->data(), buffer->size(), 4u,NULL);
                } else if (flags & OMX_BUFFERFLAG_EOS) {
                    LLOGV("[%s] calling emptyBuffer %u w/ EOS",
                         mCodec->mComponentName.c_str(), bufferID);
                } else {
                    LLOGV("[%s] calling emptyBuffer %u w/ time %lld us",
                         mCodec->mComponentName.c_str(), bufferID, (long long)timeUs);
                }

#if TRACK_BUFFER_TIMING
                ACodec::BufferStats stats;
                stats.mEmptyBufferTimeUs = ALooper::GetNowUs();
                stats.mFillBufferDoneTimeUs = -1ll;
                mCodec->mBufferStats.add(timeUs, stats);
#endif
                //TODO  try to submit an output buffer for each input buffer
                LLOGW("TODO try to submit an output buffer for each input buffer");

                status_t err2 = OK;
                err2 = mCodec->mOMX->emptyBuffer(
                        mCodec->mNode,
                        bufferID,
                        0,
                        info->mData->size(),
                        flags,
                        timeUs,
                        -1/*info->fenceFd @unused*/);
                if (err2 != OK) {
                    mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err2));
                    return;
                }
                info->mStatus = BufferInfo::OWNED_BY_COMPONENT;

                if (!eos && err == OK) {
                    getMoreInputDataIfPossible();
                } else {
                    LLOGV("[%s] Signalled EOS (%d) on the input port",
                         mCodec->mComponentName.c_str(), err);

                    mCodec->mPortEOS[kPortIndexInput] = true;
                    mCodec->mInputEOSResult = err;
                }
            } else if (!mCodec->mPortEOS[kPortIndexInput]) {
                if (err != OK && err != ERROR_END_OF_STREAM) {
                    LLOGV("[%s] Signalling EOS on the input port due to error %d",
                         mCodec->mComponentName.c_str(), err);
                } else {
                    LLOGV("[%s] Signalling EOS on the input port",
                         mCodec->mComponentName.c_str());
                }

                LLOGV("[%s] calling emptyBuffer %u signalling EOS",
                     mCodec->mComponentName.c_str(), bufferID);

                status_t err2 = mCodec->mOMX->emptyBuffer(
                        mCodec->mNode,
                        bufferID,
                        0,
                        0,
                        OMX_BUFFERFLAG_EOS,
                        0,
                        -1/*info->mFenceFd @unused*/);
                if (err2 != OK) {
                    mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err2));
                    return;
                }
                info->mStatus = BufferInfo::OWNED_BY_COMPONENT;

                mCodec->mPortEOS[kPortIndexInput] = true;
                mCodec->mInputEOSResult = err;
            }
            break;
        }

        case FREE_BUFFERS:
            break;

        default:
            LLOGE("invalid port mode: %d", mode);
            break;
    }
}

void ACodec::BaseState::getMoreInputDataIfPossible() {
    if (mCodec->mPortEOS[kPortIndexInput]) {
        return;
    }

    BufferInfo *eligible = NULL;

    for (size_t i = 0; i < mCodec->mBuffers[kPortIndexInput].size(); ++i) {
        BufferInfo *info = &mCodec->mBuffers[kPortIndexInput].editItemAt(i);

#if 0
        if (info->mStatus == BufferInfo::OWNED_BY_UPSTREAM) {
            // There's already a "read" pending.
            return;
        }
#endif

        if (info->mStatus == BufferInfo::OWNED_BY_US) {
            eligible = info;
        }
    }

    if (eligible == NULL) {
        return;
    }

    postFillThisBuffer(eligible);
}

bool ACodec::BaseState::onOMXFillBufferDone(
        IOMX::buffer_id bufferID,
        size_t rangeOffset, size_t rangeLength,
        OMX_U32 flags,
        int64_t timeUs) {
    LLOGV("[%s] onOMXFillBufferDone %u time %" PRId64 " us, flags = 0x%08x",
         mCodec->mComponentName.c_str(), bufferID, timeUs, flags);

    ssize_t index;
    status_t err= OK;

#if TRACK_BUFFER_TIMING
    index = mCodec->mBufferStats.indexOfKey(timeUs);
    if (index >= 0) {
        ACodec::BufferStats *stats = &mCodec->mBufferStats.editValueAt(index);
        stats->mFillBufferDoneTimeUs = ALooper::GetNowUs();

        LLOGI("frame PTS %lld: %lld",
                timeUs,
                stats->mFillBufferDoneTimeUs - stats->mEmptyBufferTimeUs);

        mCodec->mBufferStats.removeItemsAt(index);
        stats = NULL;
    }
#endif

    BufferInfo *info =
        mCodec->findBufferByID(kPortIndexOutput, bufferID, &index);
    BufferInfo::Status status = BufferInfo::getSafeStatus(info);
    if (status != BufferInfo::OWNED_BY_COMPONENT) {
        LLOGE("Wrong ownership in FBD: %s(%d) buffer #%u", _asString(status), status, bufferID);
        mCodec->dumpBuffers(kPortIndexOutput);
        mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
        return true;
    }

    info->mDequeuedAt = ++mCodec->mDequeueCounter;
    info->mStatus = BufferInfo::OWNED_BY_US;



    PortMode mode = getPortMode(kPortIndexOutput);

    switch (mode) {
        case KEEP_BUFFERS:
            break;

        case RESUBMIT_BUFFERS:
        {
            if (rangeLength == 0 && (!(flags & OMX_BUFFERFLAG_EOS)
                    || mCodec->mPortEOS[kPortIndexOutput])) {
                LLOGV("[%s] calling fillBuffer %u",
                     mCodec->mComponentName.c_str(), info->mBufferID);

                err = mCodec->mOMX->fillBuffer(mCodec->mNode, info->mBufferID 
						,-1/*info->mFenceFd @unused*/);
                if (err != OK) {
                    mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
                    return true;
                }

                info->mStatus = BufferInfo::OWNED_BY_COMPONENT;
                break;
            }

            sp<AMessage> reply =
                new AMessage(kWhatOutputBufferDrained, mCodec);

            if (mCodec->mOutputFormat != mCodec->mLastOutputFormat && rangeLength > 0) {
                // pretend that output format has changed on the first frame (we used to do this)
                if (mCodec->mBaseOutputFormat == mCodec->mOutputFormat) {
                    mCodec->onOutputFormatChanged(mCodec->mOutputFormat);
                }
                mCodec->sendFormatChange();
            } 


            info->mData->setRange(rangeOffset, rangeLength);
            info->mData->meta()->setInt64("timeUs", timeUs);

            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatDrainThisBuffer);
            notify->setInt32("buffer-id", info->mBufferID);
            notify->setBuffer("buffer", info->mData);
            notify->setInt32("flags", flags);

            reply->setInt32("buffer-id", info->mBufferID);

            notify->setMessage("reply", reply);

            notify->post();

            info->mStatus = BufferInfo::OWNED_BY_DOWNSTREAM;

            if (flags & OMX_BUFFERFLAG_EOS) {
                LLOGV("[%s] saw output EOS", mCodec->mComponentName.c_str());

                sp<AMessage> notify = mCodec->mNotify->dup();
                notify->setInt32("what", CodecBase::kWhatEOS);
                notify->setInt32("err", mCodec->mInputEOSResult);
                notify->post();

                mCodec->mPortEOS[kPortIndexOutput] = true;
            }
            break;
        }

        case FREE_BUFFERS:
            err = mCodec->freeBuffer(kPortIndexOutput, index);
            if (err != OK) {
                mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
                return true;
            }
            break;

        default:
            LLOGE("Invalid port mode: %d", mode);
            return false;
    }

    return true;
}

void ACodec::BaseState::onOutputBufferDrained(const sp<AMessage> &msg) {
    IOMX::buffer_id bufferID;
    CHECK(msg->findInt32("buffer-id", (int32_t*)&bufferID));
    ssize_t index;
    BufferInfo *info = mCodec->findBufferByID(kPortIndexOutput, bufferID, &index);
    BufferInfo::Status status = BufferInfo::getSafeStatus(info);
    if (status != BufferInfo::OWNED_BY_DOWNSTREAM) {
        LLOGE("Wrong ownership in OBD: %s(%d) buffer #%u", _asString(status), status, bufferID);
        mCodec->dumpBuffers(kPortIndexOutput);
        mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
        return;
    }


    info->mStatus = BufferInfo::OWNED_BY_US;

    PortMode mode = getPortMode(kPortIndexOutput);

    switch (mode) {
        case KEEP_BUFFERS:
        {
            // XXX fishy, revisit!!! What about the FREE_BUFFERS case below?

            break;
        }

        case RESUBMIT_BUFFERS:
        {
            if (!mCodec->mPortEOS[kPortIndexOutput]) {


                if (info != NULL) {
                    LLOGV("[%s] calling fillBuffer %u",
                         mCodec->mComponentName.c_str(), info->mBufferID);
                    status_t err = mCodec->mOMX->fillBuffer(
                            mCodec->mNode, info->mBufferID,-1/*info->fenceFd @unused*/);
                    if (err == OK) {
                        info->mStatus = BufferInfo::OWNED_BY_COMPONENT;
                    } else {
                        mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
                    }
                }
            }
            break;
        }

        case FREE_BUFFERS:
        {
            status_t err = mCodec->freeBuffer(kPortIndexOutput, index);
            if (err != OK) {
                mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
            }
            break;
        }

        default:
            LLOGE("Invalid port mode: %d", mode);
            return;
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::UninitializedState::UninitializedState(ACodec *codec)
    : BaseState(codec) {
}

void ACodec::UninitializedState::stateEntered() {
    LLOGV("Now uninitialized");

    mCodec->mUsingNativeWindow = false;
    mCodec->mNativeWindow = NULL;
    mCodec->mNode = 0;
    mCodec->mOMX.clear();
    mCodec->mQuirks = 0;
    mCodec->mFlags = 0;
    mCodec->mComponentName.clear();
}

bool ACodec::UninitializedState::onMessageReceived(const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case ACodec::kWhatSetup:
        {
            onSetup(msg);

            handled = true;
            break;
        }

        case ACodec::kWhatAllocateComponent:
        {
            onAllocateComponent(msg);
            handled = true;
            break;
        }

        case ACodec::kWhatShutdown:
        {
            int32_t keepComponentAllocated;
            CHECK(msg->findInt32(
                        "keepComponentAllocated", &keepComponentAllocated));
            LLOGW_IF(keepComponentAllocated,
                     "cannot keep component allocated on shutdown in Uninitialized state");

            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatShutdownCompleted);
            notify->post();

            handled = true;
            break;
        }

        case ACodec::kWhatFlush:
        {
            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatFlushCompleted);
            notify->post();

            handled = true;
            break;
        }

        case ACodec::kWhatReleaseCodecInstance:
        {
            // nothing to do, as we have already signaled shutdown
            handled = true;
            break;
        }

        default:
            return BaseState::onMessageReceived(msg);
    }

    return handled;
}

void ACodec::UninitializedState::onSetup(
        const sp<AMessage> &msg) {
    if (onAllocateComponent(msg)
            && mCodec->mLoadedState->onConfigureComponent(msg)) {
        mCodec->mLoadedState->onStart();
    }
}

bool ACodec::UninitializedState::onAllocateComponent(const sp<AMessage> &msg) {
    LLOGV("onAllocateComponent");

    CHECK(mCodec->mNode == 0);


    sp<IOMX> omx = new OMX(); 


    Vector<AString> matchingCodecs;

    AString mime;

    AString componentName;
    uint32_t quirks = 0;
    int32_t encoder = false;
    if (msg->findString("componentName", &componentName)) {
        sp<IMediaCodecList> list = MediaCodecList::getLocalInstance();
        if (list != NULL && list->findCodecByName(componentName.c_str()) >= 0) {
            matchingCodecs.add(componentName);
        }
    } else {
        CHECK(msg->findString("mime", &mime));

        if (!msg->findInt32("encoder", &encoder)) {
            encoder = false;
        }

        MediaCodecList::findMatchingCodecs(
                mime.c_str(),
                encoder, // createEncoder
                0,       // flags
                &matchingCodecs);
    }

    sp<CodecObserver> observer = new CodecObserver;
    IOMX::node_id node = 0;

    status_t err = NAME_NOT_FOUND;
    for (size_t matchIndex = 0; matchIndex < matchingCodecs.size();
            ++matchIndex) {
        componentName = matchingCodecs[matchIndex];
        quirks = MediaCodecList::getQuirksFor(componentName.c_str());

        pid_t tid = gettid();
        int prevPriority = liylGetThreadPriority(tid);
        liylSetThreadPriority(tid, LIYL_PRIORITY_FOREGROUND);
        err = omx->allocateNode(componentName.c_str(), observer, &node);
        liylSetThreadPriority(tid, prevPriority);

        if (err == OK) {
            break;
        } else {
            LLOGW("Allocating component '%s' failed, try next one.", componentName.c_str());
        }

        node = 0;
    }

    if (node == 0) {
        if (!mime.empty()) {
            LLOGE("Unable to instantiate a %scoder for type '%s' with err %#x.",
                    encoder ? "en" : "de", mime.c_str(), err);
        } else {
            LLOGE("Unable to instantiate codec '%s' with err %#x.", componentName.c_str(), err);
        }

        mCodec->signalError((OMX_ERRORTYPE)err, makeNoSideEffectStatus(err));
        return false;
    }


    sp<AMessage> notify = new AMessage(kWhatOMXMessageList, mCodec);
    observer->setNotificationMessage(notify);

    mCodec->mComponentName = componentName;
    mCodec->mFlags = 0;

    

    mCodec->mQuirks = quirks;
    mCodec->mOMX = omx;
    mCodec->mNode = node;

    {
        sp<AMessage> notify = mCodec->mNotify->dup();
        notify->setInt32("what", CodecBase::kWhatComponentAllocated);
        notify->setString("componentName", mCodec->mComponentName.c_str());
        notify->post();
    }

    mCodec->changeState(mCodec->mLoadedState);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

ACodec::LoadedState::LoadedState(ACodec *codec)
    : BaseState(codec) {
}

void ACodec::LoadedState::stateEntered() {
    LLOGV("[%s] Now Loaded", mCodec->mComponentName.c_str());

    mCodec->mPortEOS[kPortIndexInput] =
        mCodec->mPortEOS[kPortIndexOutput] = false;

    mCodec->mInputEOSResult = OK;

    mCodec->mDequeueCounter = 0;
    mCodec->mInputFormat.clear();
    mCodec->mOutputFormat.clear();
    mCodec->mBaseOutputFormat.clear();

    if (mCodec->mShutdownInProgress) {
        bool keepComponentAllocated = mCodec->mKeepComponentAllocated;

        mCodec->mShutdownInProgress = false;
        mCodec->mKeepComponentAllocated = false;

        onShutdown(keepComponentAllocated);
    }
    mCodec->mExplicitShutdown = false;

    mCodec->processDeferredMessages();
}

void ACodec::LoadedState::onShutdown(bool keepComponentAllocated) {
    if (!keepComponentAllocated) {
        (void)mCodec->mOMX->freeNode(mCodec->mNode);

        mCodec->changeState(mCodec->mUninitializedState);
    }

    if (mCodec->mExplicitShutdown) {
        sp<AMessage> notify = mCodec->mNotify->dup();
        notify->setInt32("what", CodecBase::kWhatShutdownCompleted);
        notify->post();
        mCodec->mExplicitShutdown = false;
    }
}

bool ACodec::LoadedState::onMessageReceived(const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case ACodec::kWhatConfigureComponent:
        {
            onConfigureComponent(msg);
            handled = true;
            break;
        }

        case ACodec::kWhatStart:
        {
            onStart();
            handled = true;
            break;
        }

        case ACodec::kWhatShutdown:
        {
            int32_t keepComponentAllocated;
            CHECK(msg->findInt32(
                        "keepComponentAllocated", &keepComponentAllocated));

            mCodec->mExplicitShutdown = true;
            onShutdown(keepComponentAllocated);

            handled = true;
            break;
        }

        case ACodec::kWhatFlush:
        {
            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatFlushCompleted);
            notify->post();

            handled = true;
            break;
        }

        default:
            return BaseState::onMessageReceived(msg);
    }

    return handled;
}

bool ACodec::LoadedState::onConfigureComponent(
        const sp<AMessage> &msg) {
    LLOGV("onConfigureComponent");

    CHECK(mCodec->mNode != 0);

    status_t err = OK;
    AString mime;
    if (!msg->findString("mime", &mime)) {
        err = BAD_VALUE;
    } else {
        err = mCodec->configureCodec(mime.c_str(), msg);
    }
    if (err != OK) {
        LLOGE("[%s] configureCodec returning error %d",
              mCodec->mComponentName.c_str(), err);

        mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
        return false;
    }

    {
        sp<AMessage> notify = mCodec->mNotify->dup();
        notify->setInt32("what", CodecBase::kWhatComponentConfigured);
        notify->setMessage("input-format", mCodec->mInputFormat);
        notify->setMessage("output-format", mCodec->mOutputFormat);
        notify->post();
    }

    return true;
}


void ACodec::LoadedState::onStart() {
    LLOGV("onStart");

    status_t err = mCodec->mOMX->sendCommand(mCodec->mNode, OMX_CommandStateSet, OMX_StateIdle);
    if (err != OK) {
        mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
    } else {
        mCodec->changeState(mCodec->mLoadedToIdleState);
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::LoadedToIdleState::LoadedToIdleState(ACodec *codec)
    : BaseState(codec) {
}

void ACodec::LoadedToIdleState::stateEntered() {
    LLOGV("[%s] Now Loaded->Idle", mCodec->mComponentName.c_str());

    status_t err;
    if ((err = allocateBuffers()) != OK) {
        LLOGE("Failed to allocate buffers after transitioning to IDLE state "
             "(error 0x%08x)",
             err);

        mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));

        mCodec->mOMX->sendCommand(
                mCodec->mNode, OMX_CommandStateSet, OMX_StateLoaded);
        if (mCodec->allYourBuffersAreBelongToUs(kPortIndexInput)) {
            mCodec->freeBuffersOnPort(kPortIndexInput);
        }
        if (mCodec->allYourBuffersAreBelongToUs(kPortIndexOutput)) {
            mCodec->freeBuffersOnPort(kPortIndexOutput);
        }

        mCodec->changeState(mCodec->mLoadedState);
    }
}

status_t ACodec::LoadedToIdleState::allocateBuffers() {
    status_t err = mCodec->allocateBuffersOnPort(kPortIndexInput);

    if (err != OK) {
        return err;
    }

    return mCodec->allocateBuffersOnPort(kPortIndexOutput);
}

bool ACodec::LoadedToIdleState::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatSetParameters:
        case kWhatShutdown:
        {
            mCodec->deferMessage(msg);
            return true;
        }

        case kWhatSignalEndOfInputStream:
        {
            mCodec->onSignalEndOfInputStream();
            return true;
        }

        case kWhatResume:
        {
            // We'll be active soon enough.
            return true;
        }

        case kWhatFlush:
        {
            // We haven't even started yet, so we're flushed alright...
            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatFlushCompleted);
            notify->post();
            return true;
        }

        default:
            return BaseState::onMessageReceived(msg);
    }
}

bool ACodec::LoadedToIdleState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    switch (event) {
        case OMX_EventCmdComplete:
        {
            status_t err = OK;
            if (data1 != (OMX_U32)OMX_CommandStateSet
                    || data2 != (OMX_U32)OMX_StateIdle) {
                LLOGE("Unexpected command completion in LoadedToIdleState: %s(%u) %s(%u)",
                        asString((OMX_COMMANDTYPE)data1), data1,
                        asString((OMX_STATETYPE)data2), data2);
                err = FAILED_TRANSACTION;
            }

            if (err == OK) {
                err = mCodec->mOMX->sendCommand(
                    mCodec->mNode, OMX_CommandStateSet, OMX_StateExecuting);
            }

            if (err != OK) {
                mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
            } else {
                mCodec->changeState(mCodec->mIdleToExecutingState);
            }

            return true;
        }

        default:
            return BaseState::onOMXEvent(event, data1, data2);
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::IdleToExecutingState::IdleToExecutingState(ACodec *codec)
    : BaseState(codec) {
}

void ACodec::IdleToExecutingState::stateEntered() {
    LLOGV("[%s] Now Idle->Executing", mCodec->mComponentName.c_str());
}

bool ACodec::IdleToExecutingState::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatSetParameters:
        case kWhatShutdown:
        {
            mCodec->deferMessage(msg);
            return true;
        }

        case kWhatResume:
        {
            // We'll be active soon enough.
            return true;
        }

        case kWhatFlush:
        {
            // We haven't even started yet, so we're flushed alright...
            sp<AMessage> notify = mCodec->mNotify->dup();
            notify->setInt32("what", CodecBase::kWhatFlushCompleted);
            notify->post();

            return true;
        }

        case kWhatSignalEndOfInputStream:
        {
            mCodec->onSignalEndOfInputStream();
            return true;
        }

        default:
            return BaseState::onMessageReceived(msg);
    }
}

bool ACodec::IdleToExecutingState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    switch (event) {
        case OMX_EventCmdComplete:
        {
            if (data1 != (OMX_U32)OMX_CommandStateSet
                    || data2 != (OMX_U32)OMX_StateExecuting) {
                LLOGE("Unexpected command completion in IdleToExecutingState: %s(%u) %s(%u)",
                        asString((OMX_COMMANDTYPE)data1), data1,
                        asString((OMX_STATETYPE)data2), data2);
                mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
                return true;
            }

            mCodec->mExecutingState->resume();
            mCodec->changeState(mCodec->mExecutingState);

            return true;
        }

        default:
            return BaseState::onOMXEvent(event, data1, data2);
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::ExecutingState::ExecutingState(ACodec *codec)
    : BaseState(codec),
      mActive(false) {
}

ACodec::BaseState::PortMode ACodec::ExecutingState::getPortMode(
        OMX_U32 /* portIndex */) {
    return RESUBMIT_BUFFERS;
}


void ACodec::ExecutingState::submitRegularOutputBuffers() {
    bool failed = false;
    for (size_t i = 0; i < mCodec->mBuffers[kPortIndexOutput].size(); ++i) {
        BufferInfo *info = &mCodec->mBuffers[kPortIndexOutput].editItemAt(i);

        if (mCodec->mNativeWindow != NULL) {
            if (info->mStatus != BufferInfo::OWNED_BY_US
                    && info->mStatus != BufferInfo::OWNED_BY_NATIVE_WINDOW) {
                LLOGE("buffers should be owned by us or the surface");
                failed = true;
                break;
            }

            if (info->mStatus == BufferInfo::OWNED_BY_NATIVE_WINDOW) {
                continue;
            }
        } else {
            if (info->mStatus != BufferInfo::OWNED_BY_US) {
                LLOGE("buffers should be owned by us");
                failed = true;
                break;
            }
        }

        LLOGV("[%s] calling fillBuffer %u", mCodec->mComponentName.c_str(), info->mBufferID);

        status_t err = mCodec->mOMX->fillBuffer(mCodec->mNode, info->mBufferID, -1/*info->fenceFd unused*/);
        if (err != OK) {
            failed = true;
            break;
        }

        info->mStatus = BufferInfo::OWNED_BY_COMPONENT;
    }

    if (failed) {
        mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
    }
}

void ACodec::ExecutingState::submitOutputBuffers() {
    submitRegularOutputBuffers();
}

void ACodec::ExecutingState::resume() {
    if (mActive) {
        LLOGV("[%s] We're already active, no need to resume.", mCodec->mComponentName.c_str());
        return;
    }

    submitOutputBuffers();

    // Post all available input buffers
    if (mCodec->mBuffers[kPortIndexInput].size() == 0u) {
        LLOGW("[%s] we don't have any input buffers to resume", mCodec->mComponentName.c_str());
    }

    for (size_t i = 0; i < mCodec->mBuffers[kPortIndexInput].size(); i++) {
        BufferInfo *info = &mCodec->mBuffers[kPortIndexInput].editItemAt(i);
        if (info->mStatus == BufferInfo::OWNED_BY_US) {
            postFillThisBuffer(info);
        }
    }

    mActive = true;
}

void ACodec::ExecutingState::stateEntered() {
    LLOGV("[%s] Now Executing", mCodec->mComponentName.c_str());

    mCodec->processDeferredMessages();
}

bool ACodec::ExecutingState::onMessageReceived(const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case kWhatShutdown:
        {
            int32_t keepComponentAllocated;
            CHECK(msg->findInt32(
                        "keepComponentAllocated", &keepComponentAllocated));

            mCodec->mShutdownInProgress = true;
            mCodec->mExplicitShutdown = true;
            mCodec->mKeepComponentAllocated = keepComponentAllocated;

            mActive = false;

            status_t err = mCodec->mOMX->sendCommand(
                    mCodec->mNode, OMX_CommandStateSet, OMX_StateIdle);
            if (err != OK) {
                if (keepComponentAllocated) {
                    mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
                }
                // TODO: do some recovery here.
				LLOGW("TODO do some recovery here");
            } else {
                mCodec->changeState(mCodec->mExecutingToIdleState);
            }

            handled = true;
            break;
        }

        case kWhatFlush:
        {
            LLOGV("[%s] ExecutingState flushing now "
                 "(codec owns %zu/%zu input, %zu/%zu output).",
                    mCodec->mComponentName.c_str(),
                    mCodec->countBuffersOwnedByComponent(kPortIndexInput),
                    mCodec->mBuffers[kPortIndexInput].size(),
                    mCodec->countBuffersOwnedByComponent(kPortIndexOutput),
                    mCodec->mBuffers[kPortIndexOutput].size());

            mActive = false;

            status_t err = mCodec->mOMX->sendCommand(mCodec->mNode, OMX_CommandFlush, OMX_ALL);
            if (err != OK) {
                mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
            } else {
                mCodec->changeState(mCodec->mFlushingState);
            }

            handled = true;
            break;
        }

        case kWhatResume:
        {
            resume();

            handled = true;
            break;
        }

        case kWhatSetParameters:
        {
            sp<AMessage> params;
            CHECK(msg->findMessage("params", &params));

            status_t err = mCodec->setParameters(params);

            sp<AMessage> reply;
            if (msg->findMessage("reply", &reply)) {
                reply->setInt32("err", err);
                reply->post();
            }

            handled = true;
            break;
        }

        case ACodec::kWhatSignalEndOfInputStream:
        {
            mCodec->onSignalEndOfInputStream();
            handled = true;
            break;
        }

        default:
            handled = BaseState::onMessageReceived(msg);
            break;
    }

    return handled;
}

status_t ACodec::setParameters(const sp<AMessage> &params) {
    int32_t videoBitrate;
	status_t err = OK;
    if (params->findInt32("video-bitrate", &videoBitrate)) {
        OMX_VIDEO_CONFIG_BITRATETYPE configParams;
        InitOMXParams(&configParams);
        configParams.nPortIndex = kPortIndexOutput;
        configParams.nEncodeBitrate = videoBitrate;

        err = mOMX->setConfig(
                mNode,
                OMX_IndexConfigVideoBitrate,
                &configParams,
                sizeof(configParams));

        if (err != OK) {
            LLOGE("setConfig(OMX_IndexConfigVideoBitrate, %d) failed w/ err %d",
                   videoBitrate, err);

            return err;
        }
    }

    float rate;
    if (params->findFloat("operating-rate", &rate) && rate > 0) {
        err = setOperatingRate(rate, mIsVideo);
        if (err != OK) {
            LLOGE("Failed to set parameter 'operating-rate' (err %d)", err);
            return err;
        }
    }

    return err;
}

void ACodec::onSignalEndOfInputStream() {
    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", CodecBase::kWhatSignaledInputEOS);

    status_t err = mOMX->signalEndOfInputStream(mNode);
    if (err != OK) {
        notify->setInt32("err", err);
    }
    notify->post();
}

bool ACodec::ExecutingState::onOMXFrameRendered(int64_t mediaTimeUs, nsecs_t systemNano) {
    LLOGV("mCodec->onFrameRendered(mediaTimeUs, systemNano)");
    return true;
}

bool ACodec::ExecutingState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    switch (event) {
        case OMX_EventPortSettingsChanged:
        {
            CHECK_EQ(data1, (OMX_U32)kPortIndexOutput);

            mCodec->onOutputFormatChanged();

            if (data2 == 0 || data2 == OMX_IndexParamPortDefinition) {
                CHECK_EQ(mCodec->mOMX->sendCommand(
                            mCodec->mNode,
                            OMX_CommandPortDisable, kPortIndexOutput),
                         (status_t)OK);

                mCodec->freeOutputBuffersNotOwnedByComponent();

                mCodec->changeState(mCodec->mOutputPortSettingsChangedState);
            } else if (data2 != OMX_IndexConfigCommonOutputCrop
                    && data2 != OMX_IndexConfigLiylIntraRefresh) {
                LLOGV("[%s] OMX_EventPortSettingsChanged 0x%08x",
                     mCodec->mComponentName.c_str(), data2);
            }

            return true;
        }

        case OMX_EventBufferFlag:
        {
            return true;
        }

        default:
            return BaseState::onOMXEvent(event, data1, data2);
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::OutputPortSettingsChangedState::OutputPortSettingsChangedState(
        ACodec *codec)
    : BaseState(codec) {
}

ACodec::BaseState::PortMode ACodec::OutputPortSettingsChangedState::getPortMode(
        OMX_U32 portIndex) {
    if (portIndex == kPortIndexOutput) {
        return FREE_BUFFERS;
    }

    CHECK_EQ(portIndex, (OMX_U32)kPortIndexInput);

    return RESUBMIT_BUFFERS;
}

bool ACodec::OutputPortSettingsChangedState::onMessageReceived(
        const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case kWhatFlush:
        case kWhatShutdown:
        case kWhatResume:
        case kWhatSetParameters:
        {
            if (msg->what() == kWhatResume) {
                LLOGV("[%s] Deferring resume", mCodec->mComponentName.c_str());
            }

            mCodec->deferMessage(msg);
            handled = true;
            break;
        }

        default:
            handled = BaseState::onMessageReceived(msg);
            break;
    }

    return handled;
}

void ACodec::OutputPortSettingsChangedState::stateEntered() {
    LLOGV("[%s] Now handling output port settings change",
         mCodec->mComponentName.c_str());
}

bool ACodec::OutputPortSettingsChangedState::onOMXFrameRendered(
        int64_t mediaTimeUs, nsecs_t systemNano) {
    LLOGV("mCodec->onFrameRendered(mediaTimeUs, systemNano)");
    return true;
}

bool ACodec::OutputPortSettingsChangedState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    switch (event) {
        case OMX_EventCmdComplete:
        {
            if (data1 == (OMX_U32)OMX_CommandPortDisable) {
                if (data2 != (OMX_U32)kPortIndexOutput) {
                    LLOGW("ignoring EventCmdComplete CommandPortDisable for port %u", data2);
                    return false;
                }

                LLOGV("[%s] Output port now disabled.", mCodec->mComponentName.c_str());

                status_t err = OK;
                if (!mCodec->mBuffers[kPortIndexOutput].isEmpty()) {
                    LLOGE("disabled port should be empty, but has %zu buffers",
                            mCodec->mBuffers[kPortIndexOutput].size());
                    err = FAILED_TRANSACTION;
                }

                if (err == OK) {
                    err = mCodec->mOMX->sendCommand(
                            mCodec->mNode, OMX_CommandPortEnable, kPortIndexOutput);
                }

                if (err == OK) {
                    err = mCodec->allocateBuffersOnPort(kPortIndexOutput);
                    LLOGE_IF(err != OK, "Failed to allocate output port buffers after port "
                            "reconfiguration: (%d)", err);
                }

                if (err != OK) {
                    mCodec->signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));

                    // This is technically not correct, but appears to be
                    // the only way to free the component instance.
                    // Controlled transitioning from excecuting->idle
                    // and idle->loaded seem impossible probably because
                    // the output port never finishes re-enabling.
                    mCodec->mShutdownInProgress = true;
                    mCodec->mKeepComponentAllocated = false;
                    mCodec->changeState(mCodec->mLoadedState);
                }

                return true;
            } else if (data1 == (OMX_U32)OMX_CommandPortEnable) {
                if (data2 != (OMX_U32)kPortIndexOutput) {
                    LLOGW("ignoring EventCmdComplete OMX_CommandPortEnable for port %u", data2);
                    return false;
                }

                LLOGV("[%s] Output port now reenabled.", mCodec->mComponentName.c_str());

                if (mCodec->mExecutingState->active()) {
                    mCodec->mExecutingState->submitOutputBuffers();
                }

                mCodec->changeState(mCodec->mExecutingState);

                return true;
            }

            return false;
        }

        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::ExecutingToIdleState::ExecutingToIdleState(ACodec *codec)
    : BaseState(codec),
      mComponentNowIdle(false) {
}

bool ACodec::ExecutingToIdleState::onMessageReceived(const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case kWhatFlush:
        {
            // Don't send me a flush request if you previously wanted me
            // to shutdown.
            LLOGW("Ignoring flush request in ExecutingToIdleState");
            break;
        }

        case kWhatShutdown:
        {
            // We're already doing that...

            handled = true;
            break;
        }

        default:
            handled = BaseState::onMessageReceived(msg);
            break;
    }

    return handled;
}

void ACodec::ExecutingToIdleState::stateEntered() {
    LLOGV("[%s] Now Executing->Idle", mCodec->mComponentName.c_str());

    mComponentNowIdle = false;
    mCodec->mLastOutputFormat.clear();
}

bool ACodec::ExecutingToIdleState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    switch (event) {
        case OMX_EventCmdComplete:
        {
            if (data1 != (OMX_U32)OMX_CommandStateSet
                    || data2 != (OMX_U32)OMX_StateIdle) {
                LLOGE("Unexpected command completion in ExecutingToIdleState: %s(%u) %s(%u)",
                        asString((OMX_COMMANDTYPE)data1), data1,
                        asString((OMX_STATETYPE)data2), data2);
                mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
                return true;
            }

            mComponentNowIdle = true;

            changeStateIfWeOwnAllBuffers();

            return true;
        }

        case OMX_EventPortSettingsChanged:
        case OMX_EventBufferFlag:
        {
            // We're shutting down and don't care about this anymore.
            return true;
        }

        default:
            return BaseState::onOMXEvent(event, data1, data2);
    }
}

void ACodec::ExecutingToIdleState::changeStateIfWeOwnAllBuffers() {
    if (mComponentNowIdle && mCodec->allYourBuffersAreBelongToUs()) {
        status_t err = mCodec->mOMX->sendCommand(
                mCodec->mNode, OMX_CommandStateSet, OMX_StateLoaded);
        if (err == OK) {
            err = mCodec->freeBuffersOnPort(kPortIndexInput);
            status_t err2 = mCodec->freeBuffersOnPort(kPortIndexOutput);
            if (err == OK) {
                err = err2;
            }
        }


        if (err != OK) {
            mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
            return;
        }

        mCodec->changeState(mCodec->mIdleToLoadedState);
    }
}

void ACodec::ExecutingToIdleState::onInputBufferFilled(
        const sp<AMessage> &msg) {
    BaseState::onInputBufferFilled(msg);

    changeStateIfWeOwnAllBuffers();
}

void ACodec::ExecutingToIdleState::onOutputBufferDrained(
        const sp<AMessage> &msg) {
    BaseState::onOutputBufferDrained(msg);

    changeStateIfWeOwnAllBuffers();
}

////////////////////////////////////////////////////////////////////////////////

ACodec::IdleToLoadedState::IdleToLoadedState(ACodec *codec)
    : BaseState(codec) {
}

bool ACodec::IdleToLoadedState::onMessageReceived(const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case kWhatShutdown:
        {
            // We're already doing that...

            handled = true;
            break;
        }

        case kWhatFlush:
        {
            // Don't send me a flush request if you previously wanted me
            // to shutdown.
            LLOGE("Got flush request in IdleToLoadedState");
            break;
        }

        default:
            handled = BaseState::onMessageReceived(msg);
            break;
    }

    return handled;
}

void ACodec::IdleToLoadedState::stateEntered() {
    LLOGV("[%s] Now Idle->Loaded", mCodec->mComponentName.c_str());
}

bool ACodec::IdleToLoadedState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    switch (event) {
        case OMX_EventCmdComplete:
        {
            if (data1 != (OMX_U32)OMX_CommandStateSet
                    || data2 != (OMX_U32)OMX_StateLoaded) {
                LLOGE("Unexpected command completion in IdleToLoadedState: %s(%u) %s(%u)",
                        asString((OMX_COMMANDTYPE)data1), data1,
                        asString((OMX_STATETYPE)data2), data2);
                mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
                return true;
            }

            mCodec->changeState(mCodec->mLoadedState);

            return true;
        }

        default:
            return BaseState::onOMXEvent(event, data1, data2);
    }
}

////////////////////////////////////////////////////////////////////////////////

ACodec::FlushingState::FlushingState(ACodec *codec)
    : BaseState(codec) {
}

void ACodec::FlushingState::stateEntered() {
    LLOGV("[%s] Now Flushing", mCodec->mComponentName.c_str());

    mFlushComplete[kPortIndexInput] = mFlushComplete[kPortIndexOutput] = false;
}

bool ACodec::FlushingState::onMessageReceived(const sp<AMessage> &msg) {
    bool handled = false;

    switch (msg->what()) {
        case kWhatShutdown:
        {
            mCodec->deferMessage(msg);
            break;
        }

        case kWhatFlush:
        {
            // We're already doing this right now.
            handled = true;
            break;
        }

        default:
            handled = BaseState::onMessageReceived(msg);
            break;
    }

    return handled;
}

bool ACodec::FlushingState::onOMXEvent(
        OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2) {
    LLOGV("[%s] FlushingState onOMXEvent(%u,%d)",
            mCodec->mComponentName.c_str(), event, (OMX_S32)data1);

    switch (event) {
        case OMX_EventCmdComplete:
        {
            if (data1 != (OMX_U32)OMX_CommandFlush) {
                LLOGE("unexpected EventCmdComplete %s(%d) data2:%d in FlushingState",
                        asString((OMX_COMMANDTYPE)data1), data1, data2);
                mCodec->signalError(OMX_ErrorUndefined, FAILED_TRANSACTION);
                return true;
            }

            if (data2 == kPortIndexInput || data2 == kPortIndexOutput) {
                if (mFlushComplete[data2]) {
                    LLOGW("Flush already completed for %s port",
                            data2 == kPortIndexInput ? "input" : "output");
                    return true;
                }
                mFlushComplete[data2] = true;

                if (mFlushComplete[kPortIndexInput] && mFlushComplete[kPortIndexOutput]) {
                    changeStateIfWeOwnAllBuffers();
                }
            } else if (data2 == OMX_ALL) {
                if (!mFlushComplete[kPortIndexInput] || !mFlushComplete[kPortIndexOutput]) {
                    LLOGW("received flush complete event for OMX_ALL before ports have been"
                            "flushed (%d/%d)",
                            mFlushComplete[kPortIndexInput], mFlushComplete[kPortIndexOutput]);
                    return false;
                }

                changeStateIfWeOwnAllBuffers();
            } else {
                LLOGW("data2 not OMX_ALL but %u in EventCmdComplete CommandFlush", data2);
            }

            return true;
        }

        case OMX_EventPortSettingsChanged:
        {
            sp<AMessage> msg = new AMessage(kWhatOMXMessage, mCodec);
            msg->setInt32("type", omx_message::EVENT);
            msg->setInt32("node", mCodec->mNode);
            msg->setInt32("event", event);
            msg->setInt32("data1", data1);
            msg->setInt32("data2", data2);

            LLOGV("[%s] Deferring OMX_EventPortSettingsChanged",
                 mCodec->mComponentName.c_str());

            mCodec->deferMessage(msg);

            return true;
        }

        default:
            return BaseState::onOMXEvent(event, data1, data2);
    }

    return true;
}

void ACodec::FlushingState::onOutputBufferDrained(const sp<AMessage> &msg) {
    BaseState::onOutputBufferDrained(msg);

    changeStateIfWeOwnAllBuffers();
}

void ACodec::FlushingState::onInputBufferFilled(const sp<AMessage> &msg) {
    BaseState::onInputBufferFilled(msg);

    changeStateIfWeOwnAllBuffers();
}

void ACodec::FlushingState::changeStateIfWeOwnAllBuffers() {
    if (mFlushComplete[kPortIndexInput]
            && mFlushComplete[kPortIndexOutput]
            && mCodec->allYourBuffersAreBelongToUs()) {


        sp<AMessage> notify = mCodec->mNotify->dup();
        notify->setInt32("what", CodecBase::kWhatFlushCompleted);
        notify->post();

        mCodec->mPortEOS[kPortIndexInput] =
            mCodec->mPortEOS[kPortIndexOutput] = false;

        mCodec->mInputEOSResult = OK;



        mCodec->changeState(mCodec->mExecutingState);
    }
}


// These are supposed be equivalent to the logic in
// "audio_channel_out_mask_from_count".
//static
status_t ACodec::getOMXChannelMapping(size_t numChannels, OMX_AUDIO_CHANNELTYPE map[]) {
    switch (numChannels) {
        case 1:
            map[0] = OMX_AUDIO_ChannelCF;
            break;
        case 2:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            break;
        case 3:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            map[2] = OMX_AUDIO_ChannelCF;
            break;
        case 4:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            map[2] = OMX_AUDIO_ChannelLR;
            map[3] = OMX_AUDIO_ChannelRR;
            break;
        case 5:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            map[2] = OMX_AUDIO_ChannelCF;
            map[3] = OMX_AUDIO_ChannelLR;
            map[4] = OMX_AUDIO_ChannelRR;
            break;
        case 6:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            map[2] = OMX_AUDIO_ChannelCF;
            map[3] = OMX_AUDIO_ChannelLFE;
            map[4] = OMX_AUDIO_ChannelLR;
            map[5] = OMX_AUDIO_ChannelRR;
            break;
        case 7:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            map[2] = OMX_AUDIO_ChannelCF;
            map[3] = OMX_AUDIO_ChannelLFE;
            map[4] = OMX_AUDIO_ChannelLR;
            map[5] = OMX_AUDIO_ChannelRR;
            map[6] = OMX_AUDIO_ChannelCS;
            break;
        case 8:
            map[0] = OMX_AUDIO_ChannelLF;
            map[1] = OMX_AUDIO_ChannelRF;
            map[2] = OMX_AUDIO_ChannelCF;
            map[3] = OMX_AUDIO_ChannelLFE;
            map[4] = OMX_AUDIO_ChannelLR;
            map[5] = OMX_AUDIO_ChannelRR;
            map[6] = OMX_AUDIO_ChannelLS;
            map[7] = OMX_AUDIO_ChannelRS;
            break;
        default:
            return -EINVAL;
    }

    return OK;
}

LIYL_NAMESPACE_END

