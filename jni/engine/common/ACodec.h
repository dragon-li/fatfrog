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

#ifndef LIYL_A_CODEC_H_

#define LIYL_A_CODEC_H_

#include <stdint.h>
#include "MediaWindow.h"
#include "../../omx/api/IOMX.h"
#include "../../foundation/media/include/AHierarchicalStateMachine.h"
#include "CodecBase.h"
#include "../../common/include/MediaDefs.h"
//#include "../../SkipCutBuffer.h"
#include "../../prebuild/openmax/OMX_Audio.h"

#define TRACK_BUFFER_TIMING     0

LIYL_NAMESPACE_START

struct ABuffer;
struct DescribeColorFormat2Params;

struct ACodec : public AHierarchicalStateMachine, public CodecBase {
    ACodec();

    virtual void setNotificationMessage(const sp<AMessage> &msg);

    void initiateSetup(const sp<AMessage> &msg);

    virtual void initiateAllocateComponent(const sp<AMessage> &msg);
    virtual void initiateConfigureComponent(const sp<AMessage> &msg);
    virtual void initiateStart();
    virtual void initiateShutdown(bool keepComponentAllocated = false);

    virtual status_t setSurface(const MediaWindow *surface);

    virtual void signalFlush();
    virtual void signalResume();

    virtual void signalSetParameters(const sp<AMessage> &msg);
    virtual void signalEndOfInputStream();

    // AHierarchicalStateMachine implements the message handling
    virtual void onMessageReceived(const sp<AMessage> &msg) {
        handleMessage(msg);
    }

    struct PortDescription : public CodecBase::PortDescription {
        size_t countBuffers();
        IOMX::buffer_id bufferIDAt(size_t index) const;
        sp<ABuffer> bufferAt(size_t index) const;

    private:
        friend struct ACodec;

        Vector<IOMX::buffer_id> mBufferIDs;
        Vector<sp<ABuffer> > mBuffers;

        PortDescription();
        void addBuffer(
                IOMX::buffer_id id, const sp<ABuffer> &buffer);

        DISALLOW_EVIL_CONSTRUCTORS(PortDescription);
    };

    static bool isFlexibleColorFormat(
            const sp<IOMX> &omx, IOMX::node_id node,
            uint32_t colorFormat, bool usingNativeBuffers, OMX_U32 *flexibleEquivalent);

    // Returns 0 if configuration is not supported.  NOTE: this is treated by
    // some OMX components as auto level, and by others as invalid level.
    static int /* OMX_VIDEO_AVCLEVELTYPE */ getAVCLevelFor(
            int width, int height, int rate, int bitrate,
            OMX_VIDEO_AVCPROFILETYPE profile = OMX_VIDEO_AVCProfileBaseline);


    static status_t getOMXChannelMapping(size_t numChannels, OMX_AUDIO_CHANNELTYPE map[]);

protected:
    virtual ~ACodec();

private:
    struct BaseState;
    struct UninitializedState;
    struct LoadedState;
    struct LoadedToIdleState;
    struct IdleToExecutingState;
    struct ExecutingState;
    struct OutputPortSettingsChangedState;
    struct ExecutingToIdleState;
    struct IdleToLoadedState;
    struct FlushingState;

    enum {
        kWhatSetup                   = 'setu',
        kWhatOMXMessage              = 'omx ',
        // same as kWhatOMXMessage - but only used with
        // handleMessage during OMX message-list handling
        kWhatOMXMessageItem          = 'omxI',
        kWhatOMXMessageList          = 'omxL',
        kWhatInputBufferFilled       = 'inpF',
        kWhatOutputBufferDrained     = 'outD',
        kWhatShutdown                = 'shut',
        kWhatFlush                   = 'flus',
        kWhatResume                  = 'resm',
        kWhatDrainDeferredMessages   = 'drai',
        kWhatAllocateComponent       = 'allo',
        kWhatConfigureComponent      = 'conf',
        kWhatSetSurface              = 'setS',
        kWhatSignalEndOfInputStream  = 'eois',
        kWhatStart                   = 'star',
        kWhatRequestIDRFrame         = 'ridr',
        kWhatSetParameters           = 'setP',
        kWhatReleaseCodecInstance    = 'relC',
    };

    enum {
        kPortIndexInput  = 0,
        kPortIndexOutput = 1
    };



    struct BufferInfo {
        enum Status {
            OWNED_BY_US,
            OWNED_BY_COMPONENT,
            OWNED_BY_UPSTREAM,
            OWNED_BY_DOWNSTREAM,
            OWNED_BY_NATIVE_WINDOW,
            UNRECOGNIZED,            // not a tracked buffer
        };

        static inline Status getSafeStatus(BufferInfo *info) {
            return info == NULL ? UNRECOGNIZED : info->mStatus;
        }

        IOMX::buffer_id mBufferID;
        Status mStatus;
        unsigned mDequeuedAt;

        sp<ABuffer> mData;      // the client's buffer;
    };

    static const char *_asString(BufferInfo::Status s);
    void dumpBuffers(OMX_U32 portIndex);


#if TRACK_BUFFER_TIMING
    struct BufferStats {
        int64_t mEmptyBufferTimeUs;
        int64_t mFillBufferDoneTimeUs;
    };

    KeyedVector<int64_t, BufferStats> mBufferStats;
#endif

    sp<AMessage> mNotify;

    sp<UninitializedState> mUninitializedState;
    sp<LoadedState> mLoadedState;
    sp<LoadedToIdleState> mLoadedToIdleState;
    sp<IdleToExecutingState> mIdleToExecutingState;
    sp<ExecutingState> mExecutingState;
    sp<OutputPortSettingsChangedState> mOutputPortSettingsChangedState;
    sp<ExecutingToIdleState> mExecutingToIdleState;
    sp<IdleToLoadedState> mIdleToLoadedState;
    sp<FlushingState> mFlushingState;
    //sp<SkipCutBuffer> mSkipCutBuffer; //for audio encoder, like ringbuffer

    AString mComponentName;
    uint32_t mFlags;
    uint32_t mQuirks;
    sp<IOMX> mOMX;
    IOMX::node_id mNode;

    bool mUsingNativeWindow;
    MediaWindow* mNativeWindow;
    sp<AMessage> mConfigFormat;
    sp<AMessage> mInputFormat;
    sp<AMessage> mOutputFormat;

    // Initial output format + configuration params that is reused as the base for all subsequent
    // format updates. This will equal to mOutputFormat until the first actual frame is received.
    sp<AMessage> mBaseOutputFormat;

    Vector<BufferInfo> mBuffers[2];
    bool mPortEOS[2];
    status_t mInputEOSResult;

    List<sp<AMessage> > mDeferredQueue;

    sp<AMessage> mLastOutputFormat;
    bool mIsVideo;
    bool mIsEncoder;
    bool mFatalError;
    bool mShutdownInProgress;
    bool mExplicitShutdown;

    // If "mKeepComponentAllocated" we only transition back to Loaded state
    // and do not release the component instance.
    bool mKeepComponentAllocated;

    int32_t mRotationDegrees;

    bool mChannelMaskPresent;
    int32_t mChannelMask;
    unsigned mDequeueCounter;
    bool mLegacyAdaptiveExperiment;
    size_t mNumUndequeuedBuffers;

    int64_t mRepeatFrameDelayUs;
    int64_t mMaxPtsGapUs;
    float mMaxFps;

    int64_t mTimePerFrameUs;
    int64_t mTimePerCaptureUs;

    bool mCreateInputBuffersSuspended;


    OMX_INDEXTYPE mDescribeColorAspectsIndex;
    OMX_INDEXTYPE mDescribeHDRStaticInfoIndex;

    status_t allocateBuffersOnPort(OMX_U32 portIndex);
    status_t freeBuffersOnPort(OMX_U32 portIndex);
    status_t freeBuffer(OMX_U32 portIndex, size_t i);


    status_t freeOutputBuffersNotOwnedByComponent();

    

    BufferInfo *findBufferByID(
            uint32_t portIndex, IOMX::buffer_id bufferID,
            ssize_t *index = NULL);

    status_t setComponentRole(bool isEncoder, const char *mime);
    static const char *getComponentRole(bool isEncoder, const char *mime);
    static status_t setComponentRole(
            const sp<IOMX> &omx, IOMX::node_id node, const char *role);

    status_t configureCodec(const char *mime, const sp<AMessage> &msg);


    status_t setVideoPortFormatType(
            OMX_U32 portIndex,
            OMX_VIDEO_CODINGTYPE compressionFormat,
            OMX_COLOR_FORMATTYPE colorFormat,
            bool usingNativeBuffers = false);

    status_t setSupportedOutputFormat(bool getLegacyFlexibleFormat);

    status_t setupVideoDecoder(
            const char *mime, const sp<AMessage> &msg, bool usingNativeBuffers, bool haveSwRenderer,
            sp<AMessage> &outputformat);


    status_t setVideoFormatOnPort(
            OMX_U32 portIndex,
            int32_t width, int32_t height,
            OMX_VIDEO_CODINGTYPE compressionFormat, float frameRate = -1.0);

    // sets |portIndex| port buffer numbers to be |bufferNum|. NOTE: Component could reject
    // this setting if the |bufferNum| is less than the minimum buffer num of the port.
    status_t setPortBufferNum(OMX_U32 portIndex, int bufferNum);

    // gets index or sets it to 0 on error. Returns error from codec.
    status_t initDescribeColorAspectsIndex();

    // sets |params|. If |readBack| is true, it re-gets them afterwards if set succeeded.
    // returns the codec error.
    status_t setCodecColorAspects(DescribeColorAspectsParams &params, bool readBack = false);

    // gets |params|; returns the codec error. |param| should not change on error.
    status_t getCodecColorAspects(DescribeColorAspectsParams &params);

    // gets dataspace guidance from codec and platform. |params| should be set up with the color
    // aspects to use. If |tryCodec| is true, the codec is queried first. If it succeeds, we
    // return OK. Otherwise, we fall back to the platform guidance and return the codec error;
    // though, we return OK if the codec failed with UNSUPPORTED, as codec guidance is optional.
    status_t getDataSpace(
            DescribeColorAspectsParams &params, liyl_dataspace *dataSpace /* nonnull */,
            bool tryCodec);

    // sets color aspects for the encoder for certain |width/height| based on |configFormat|, and
    // set resulting color config into |outputFormat|. If |usingNativeWindow| is true, we use
    // video defaults if config is unspecified. Returns error from the codec.
    status_t setColorAspectsForVideoDecoder(
            int32_t width, int32_t height, bool usingNativeWindow,
            const sp<AMessage> &configFormat, sp<AMessage> &outputFormat);

    // gets color aspects for the encoder for certain |width/height| based on |configFormat|, and
    // set resulting color config into |outputFormat|. If |dataSpace| is non-null, it requests
    // dataspace guidance from the codec and platform and sets it into |dataSpace|. Returns the
    // error from the codec.
    status_t getColorAspectsAndDataSpaceForVideoDecoder(
            int32_t width, int32_t height, const sp<AMessage> &configFormat,
            sp<AMessage> &outputFormat, liyl_dataspace *dataSpace);




    // updates the encoder output format with |aspects| defaulting to |dataSpace| for
    // unspecified values.
    void onDataSpaceChanged(liyl_dataspace dataSpace, const ColorAspects &aspects);

   //dynmic range control
   typedef struct drcParams {
        int32_t drcCut;
        int32_t drcBoost;
        int32_t heavyCompression;
        int32_t targetRefLevel;
        int32_t encodedTargetLevel;
    } drcParams_t;
    status_t setupAACCodec(
            bool encoder,
            int32_t numChannels, int32_t sampleRate, int32_t bitRate,
            int32_t aacProfile, bool isADTS, int32_t sbrMode,
            int32_t maxOutputChannelCount, const drcParams_t& drc,
            int32_t pcmLimiterEnable);

    status_t setupAC3Codec(bool encoder, int32_t numChannels, int32_t sampleRate);

    status_t setupEAC3Codec(bool encoder, int32_t numChannels, int32_t sampleRate);

    status_t selectAudioPortFormat(
            OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE desiredFormat);



    status_t setupRawAudioFormat(
            OMX_U32 portIndex, int32_t sampleRate, int32_t numChannels,
            AudioEncoding encoding = kAudioEncodingPcm16bit);

    status_t setPriority(int32_t priority);
    status_t setOperatingRate(float rateFloat, bool isVideo);
    status_t getIntraRefreshPeriod(uint32_t *intraRefreshPeriod);
    status_t setIntraRefreshPeriod(uint32_t intraRefreshPeriod, bool inConfigure);

    // Configures temporal layering based on |msg|. |inConfigure| shall be true iff this is called
    // during configure() call. on success the configured layering is set in |outputFormat|. If
    // |outputFormat| is mOutputFormat, it is copied to trigger an output format changed event.
    status_t configureTemporalLayers(
            const sp<AMessage> &msg, bool inConfigure, sp<AMessage> &outputFormat);

    status_t setMinBufferSize(OMX_U32 portIndex, size_t size);


    status_t verifySupportForProfileAndLevel(int32_t profile, int32_t level);

    status_t configureBitrate(
            int32_t bitrate, OMX_VIDEO_CONTROLRATETYPE bitrateMode);

    status_t setupErrorCorrectionParameters();


    // Returns true iff all buffers on the given port have status
    // OWNED_BY_US or OWNED_BY_NATIVE_WINDOW.
    bool allYourBuffersAreBelongToUs(OMX_U32 portIndex);

    bool allYourBuffersAreBelongToUs();

    void waitUntilAllPossibleNativeWindowBuffersAreReturnedToUs();

    size_t countBuffersOwnedByComponent(OMX_U32 portIndex) const;
    size_t countBuffersOwnedByNativeWindow() const;

    void deferMessage(const sp<AMessage> &msg);
    void processDeferredMessages();


    // Pass |expectedFormat| to print a warning if the format differs from it.
    // Using sp<> instead of const sp<>& because expectedFormat is likely the current mOutputFormat
    // which will get updated inside.
    void onOutputFormatChanged(sp<const AMessage> expectedFormat = NULL);
    void sendFormatChange();

    status_t getPortFormat(OMX_U32 portIndex, sp<AMessage> &notify);

    void signalError(
            OMX_ERRORTYPE error = OMX_ErrorUndefined,
            status_t internalError = UNKNOWN_ERROR);

    static bool describeDefaultColorFormat(DescribeColorFormat2Params &describeParams);
    static bool describeColorFormat(
        const sp<IOMX> &omx, IOMX::node_id node,
        DescribeColorFormat2Params &describeParams);

    status_t setParameters(const sp<AMessage> &params);

    // Send EOS on input stream.
    void onSignalEndOfInputStream();

    DISALLOW_EVIL_CONSTRUCTORS(ACodec);
};

LIYL_NAMESPACE_END

#endif  // A_CODEC_H_
