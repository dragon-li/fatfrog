/****************************************************
*
*  @Author: liyl- liyunlong880325@gmail.com
*  Last modified: 2018-07-22 15:24
*  @Filename: ndk_mediacodec.c
*****************************************************/
#define LOG_TAG "NdkMediaCodec"
#include "../include/ndk_mediacodec.h"
#include "../../../../jni/foundation/media/include/ADebug.h"
#include "../../../../jni/foundation/media/include/ABuffer.h"
#include "../../../../jni/common/include/MediaErrors.h"
#include "../../../../jni/engine/common/MediaCodecList.h"

LIYL_NAMESPACE_START

const char* LMEDIAFORMAT_KEY_AAC_PROFILE = "aac-profile";
const char* LMEDIAFORMAT_KEY_BIT_RATE = "bitrate";
const char* LMEDIAFORMAT_KEY_CHANNEL_COUNT = "channel-count";
const char* LMEDIAFORMAT_KEY_CHANNEL_MASK = "channel-mask";
const char* LMEDIAFORMAT_KEY_COLOR_FORMAT = "color-format";
const char* LMEDIAFORMAT_KEY_DURATION = "durationUs";
const char* LMEDIAFORMAT_KEY_FLAC_COMPRESSION_LEVEL = "flac-compression-level";
const char* LMEDIAFORMAT_KEY_FRAME_RATE = "frame-rate";
const char* LMEDIAFORMAT_KEY_HEIGHT = "height";
const char* LMEDIAFORMAT_KEY_IS_ADTS = "is-adts";
const char* LMEDIAFORMAT_KEY_IS_AUTOSELECT = "is-autoselect";
const char* LMEDIAFORMAT_KEY_IS_DEFAULT = "is-default";
const char* LMEDIAFORMAT_KEY_IS_FORCED_SUBTITLE = "is-forced-subtitle";
const char* LMEDIAFORMAT_KEY_I_FRAME_INTERVAL = "i-frame-interval";
const char* LMEDIAFORMAT_KEY_LANGUAGE = "language";
const char* LMEDIAFORMAT_KEY_MAX_HEIGHT = "max-height";
const char* LMEDIAFORMAT_KEY_MAX_INPUT_SIZE = "max-input-size";
const char* LMEDIAFORMAT_KEY_MAX_WIDTH = "max-width";
const char* LMEDIAFORMAT_KEY_MIME = "mime";
const char* LMEDIAFORMAT_KEY_PUSH_BLANK_BUFFERS_ON_STOP = "push-blank-buffers-on-shutdown";
const char* LMEDIAFORMAT_KEY_REPEAT_PREVIOUS_FRAME_AFTER = "repeat-previous-frame-after";
const char* LMEDIAFORMAT_KEY_SAMPLE_RATE = "sample-rate";
const char* LMEDIAFORMAT_KEY_WIDTH = "width";
const char* LMEDIAFORMAT_KEY_STRIDE = "stride";
const char* LMEDIAFORMAT_KEY_CSD    = "csd-0";

static status_t doSetup(void* obj) {
    NdkMediaCodec* me = (NdkMediaCodec*)obj;
	MediaCodecList::getLocalInstance();
    status_t ret = OK;
	me->mLooper  = new ALooper;
    me->mLooper->setName("NDK MediaCodec_looper");
	size_t res = me->mLooper->start(
			            false,      // runOnCallingThread
			            true,       // canCallJava XXX
			            PRIORITY_FOREGROUND);
	if(res != OK) {
		LLOGE("looper start failed!!!");
		return UNKNOWN_ERROR;
	}
	AString mime = "video/avc";
    sp<MediaCodec> coder = MediaCodec::CreateByType(me->mLooper,mime,0/*decoder*/,&ret,0/*unused*/);
    if(coder == NULL) {
		LLOGE("coder create failed!!!");
        return UNKNOWN_ERROR;
    }

    me->mCodec = coder;
    me->mMediaFormat = new AMessage;
	sp<ABuffer> csd  = ABuffer::CreateAsCopy(me->mCsd,me->mCsdSize);
    me->mMediaFormat->setString(LMEDIAFORMAT_KEY_MIME,"video/avc");
    me->mMediaFormat->setInt32 (LMEDIAFORMAT_KEY_WIDTH,me->mWidth);
    me->mMediaFormat->setInt32 (LMEDIAFORMAT_KEY_HEIGHT,me->mHeight);
    me->mMediaFormat->setInt32 (LMEDIAFORMAT_KEY_FRAME_RATE,me->mFps);
    me->mMediaFormat->setInt32 (LMEDIAFORMAT_KEY_BIT_RATE,me->mBitrate);
    me->mMediaFormat->setInt32 (LMEDIAFORMAT_KEY_COLOR_FORMAT,19);
	me->mMediaFormat->setBuffer(LMEDIAFORMAT_KEY_CSD,csd);
    ret = me->mCodec->configure(me->mMediaFormat,NULL/*me->mWindow*/,NULL,0u/*decoder*/);
    if(ret != OK){
        me->mCodec = NULL;
        me->mMediaFormat = NULL;
        return ret;
    }

    ret = me->mCodec->start();
    if(ret != OK){
        me->mCodec = NULL;
        me->mMediaFormat = NULL;
        return ret;
    }
    return ret;
}

#define KTIMEOUT_CODEC_PROCESS 250000
static status_t doProcessBuffer(void* obj) {
    NdkMediaCodec* me = (NdkMediaCodec*)obj;
    status_t ret = OK;
    size_t bufidx = 0;
    if (me->mSawInputEOS != true) {
        ret = me->mCodec->dequeueInputBuffer(&bufidx ,KTIMEOUT_CODEC_PROCESS);
        LLOGD("input buffer %zd", bufidx);
        if (ret == OK) {
			sp<ABuffer> buffer;
			ret = me->mCodec->getInputBuffer( bufidx, &buffer);
			CHECK(ret == OK);
            size_t bufsize = buffer->size();;
            uint8_t *buf = buffer->data(); 
            int32_t offset = 0;
            ssize_t sampleSize = -1;
            int64_t presentationTimeUs = -1;
            me->mFTIB(buf,offset,bufsize,&sampleSize,&presentationTimeUs,me->mListener);
            if (sampleSize < 0) {
                sampleSize = 0;
                me->mSawInputEOS = true;
                LLOGI("Input EOS");
            }
			AString errMsg;
            ret = me->mCodec->queueInputBuffer(bufidx, 0, sampleSize, presentationTimeUs,
                    me->mSawInputEOS ? MediaCodec::BUFFER_FLAG_EOS : 0,&errMsg);
			CHECK(ret == OK);
        }
    }

    if (me->mSawOutputEOS != true) {
		size_t index               = 0;
		size_t offset              = 0;
		size_t size                = 0;
		int64_t presentationTimeUs = -1ll;
		uint32_t  flags            = 0;
		status_t status = me->mCodec->dequeueOutputBuffer(&index, &offset,&size ,&presentationTimeUs, &flags, KTIMEOUT_CODEC_PROCESS);
        if (status == OK) {
            if (flags & MediaCodec::BUFFER_FLAG_EOS) {
                LLOGI("output EOS");
                me->mSawOutputEOS = true;
            }
			sp<ABuffer> buffer;
            size_t outSize = -1;
			uint8_t* buf = NULL;
            ret = me->mCodec->getOutputBuffer(index,&buffer);
            if(ret != OK) {
                return ret;
            }
			outSize = buffer->size();
			buf     = buffer->data();
            CHECK(size <= outSize);
            me->mDTOB(buf,offset,size,presentationTimeUs,flags,me->mListener);
            ret = me->mCodec->releaseOutputBuffer(index);
            CHECK(ret == OK);

        } else if (status == INFO_OUTPUT_BUFFERS_CHANGED) {
            LLOGI("output buffers changed");
        } else if (status == liyl::INFO_FORMAT_CHANGED) {
			sp<AMessage> format;
            ret = me->mCodec->getOutputFormat(&format);
            LLOGI("format changed to: %s", format->debugString().c_str());
            CHECK(ret == OK);
        } else if (status == -EAGAIN) {
            LLOGI("no output buffer right now");
        } else {
            LLOGE("unexpected info code: %zd", status);
        }
    }

    if ((me->mSawInputEOS != true) || (me->mSawOutputEOS != true)) {
        //me->mLooper->post(kMsgProcessBuffer, me);
    }

    return ret;
}
static status_t doFlush(void* obj) {
    NdkMediaCodec* me = (NdkMediaCodec*)obj;
    status_t ret = OK;
    if(me->mCodec != NULL) {
        ret = me->mCodec->flush();
        CHECK(ret == OK);
    }
    return ret;
}

static status_t doShutDown(void* obj) {
    NdkMediaCodec* me = (NdkMediaCodec*)obj;
    status_t ret = OK;
    if(me->mCodec != NULL) {
        ret = me->mCodec->stop();
        CHECK(ret == OK);
		ret = me->mCodec->release();
        CHECK(ret == OK);
        me->mCodec = NULL;
    }
    if(me->mMediaFormat != NULL) {
        me->mMediaFormat = NULL;
    }
    me->mSawInputEOS = true;
    me->mSawOutputEOS= true;
	MediaCodecList::destoryLocalInstance();
    return ret;
}

////////////////////////////////////////////NdkMediaCodec//////////////////////
status_t NdkMediaCodec_init(NdkMediaCodec* me) {
    pthread_mutex_init(&(me->mLock), NULL);    
    me->mWindow       = NULL;    
    //me->mCodec        = NULL;
    //me->mMediaFormat  = NULL;
    //me->mLooper       = NULL;
    me->mWidth        = -1;
    me->mHeight       = -1;
    me->mFps          = -1;
    me->mBitrate      = -1;
    me->mCsd          = NULL;
    me->mCsdSize      = -1;
    me->mSawInputEOS  = false;
    me->mSawOutputEOS = false;
    me->mResult       = OK;
    me->mListener     = NULL;
    me->mFTIB         = NULL;
    me->mDTOB         = NULL;
    return OK;
}

status_t NdkMediaCodec_destroy(NdkMediaCodec* me) {
    pthread_mutex_destroy(&(me->mLock));
    me->mWindow       = NULL;    
    //me->mCodec        = NULL;
    //me->mMediaFormat  = NULL;
    //me->mLooper       = NULL;
    me->mWidth        = -1;
    me->mHeight       = -1;
    me->mFps          = -1;
    me->mBitrate      = -1;
    me->mCsd          = NULL;
    me->mCsdSize      = -1;
    me->mSawInputEOS  = false;
    me->mSawOutputEOS = false;
    me->mResult       = OK;
    me->mListener     = NULL;
    me->mFTIB         = NULL;
    me->mDTOB         = NULL;
    return OK;
}

status_t NdkMediaCodec_setup(NdkMediaCodec* me,LNativeWindow* window,int32_t width,int32_t height,int32_t fps ,int32_t bitrate,uint8_t* csd,int32_t csdSize) {
    pthread_mutex_lock(&(me->mLock));
    me->mWindow  = window;
    me->mWidth   = width;
    me->mHeight  = height;
    me->mFps     = fps;
    me->mBitrate = bitrate;
    me->mCsd     = csd;
    me->mCsdSize = csdSize;
    me->mResult  = doSetup(me);
    pthread_mutex_unlock(&(me->mLock));
    return me->mResult;
}

status_t NdkMediaCodec_execute(NdkMediaCodec* me) {
    pthread_mutex_lock(&(me->mLock));
    me->mResult = doProcessBuffer(me);
    pthread_mutex_unlock(&(me->mLock));
    return me->mResult;
}

status_t NdkMediaCodec_flush(NdkMediaCodec* me) {
    pthread_mutex_lock(&(me->mLock));
    me->mResult = doFlush(me);
    pthread_mutex_unlock(&(me->mLock));
    return me->mResult;
}


status_t NdkMediaCodec_shutdown(NdkMediaCodec* me) {
    pthread_mutex_lock(&(me->mLock));
    me->mResult = doShutDown(me);
    if (me->mWindow != NULL) {
		//TODO
        //p_nw_release(me->mWindow);
        //me->mWindow = NULL;
    }
    pthread_mutex_unlock(&(me->mLock));
    return me->mResult;
}

void NdkMediaCodec_setCallBackFunctions(NdkMediaCodec* me,fillThisInputBuffer ftib,drainThisOutputBuffer dtob,void* context) {
    pthread_mutex_lock(&(me->mLock));
    me->mListener = context;
    me->mFTIB     = ftib;
    me->mDTOB     = dtob;
    pthread_mutex_unlock(&(me->mLock));
}

LIYL_NAMESPACE_END
