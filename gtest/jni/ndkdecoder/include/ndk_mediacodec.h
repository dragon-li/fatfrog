/****************************************************
 *
 *  @Author: liyl- liyunlong880325@gmail.com
 *  Last modified: 2018-07-21 23:35
 *  @Filename: ndk_mediacodec.h
 *****************************************************/
#ifndef NDK_MEDIACODEC_H
#define NDK_MEDIACODEC_H
#include <pthread.h>
#include "../../../../jni/engine/common/MediaCodec.h"
#include "../../../../jni/foundation/media/include/AMessage.h"


LIYL_NAMESPACE_START

enum {
    kMsgCodecSetUp,
    kMsgProcessBuffer,
    kMsgCodeShutDown,
};
typedef int (*fillThisInputBuffer)(void* data,int32_t offset,size_t size,int32_t* realSize,int64_t* presentationTimeUs,void* context);
typedef int (*drainThisOutputBuffer)(void* data,int32_t offset,size_t size,int64_t presentationTimeUs,uint32_t flags,void* context);

typedef struct NdkMediaCodec {
    pthread_mutex_t       mLock;
    bool                  isVideo;
    LNativeWindow*        mWindow;
    sp<MediaCodec>        mCodec;
    sp<AMessage>          mMediaFormat;
    sp<ALooper>           mLooper;
    int32_t               mWidth;
    int32_t               mHeight;
    int32_t               mFps;
    int32_t               mBitrate;
    uint8_t*              mCsd;
    int32_t               mCsdSize;
    bool                  mSawInputEOS;
    bool                  mSawOutputEOS;
    status_t              mResult;
    void*                 mListener;
    fillThisInputBuffer   mFTIB;
    drainThisOutputBuffer mDTOB;
} NdkMediaCodec;

status_t NdkMediaCodec_init(NdkMediaCodec* me);
status_t NdkMediaCodec_destroy(NdkMediaCodec* me);
status_t NdkMediaCodec_setup(NdkMediaCodec* me,LNativeWindow* window,int32_t width,int32_t height,int32_t fps ,int32_t bitrate,uint8_t* csd,int32_t csdSize);
status_t NdkMediaCodec_execute(NdkMediaCodec* me);
status_t NdkMediaCodec_flush(NdkMediaCodec* me);
status_t NdkMediaCodec_shutdown(NdkMediaCodec* me);
void NdkMediaCodec_setCallBackFunctions(NdkMediaCodec* me,fillThisInputBuffer ftib,drainThisOutputBuffer dtob,void* context);

LIYL_NAMESPACE_END

#endif //NDK_MEDIACODEC_H
