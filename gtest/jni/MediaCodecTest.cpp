/*************************************************************************
  > File Name: MediaCodecTest.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2018年03月02日 星期五 11时08分56秒
 ************************************************************************/

#include "../../jni/common/include/MediaDefs.h"
#include "ndkdecoder/include/ndk_mediacodec.h"
#include "../../jni/foundation/media/include/ADebug.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <errno.h>
#include <stdlib.h>
using namespace std;
USING_NAMESPACE_LIYL;

class MediaCodecTest : public ::testing::Test {
    public:
        virtual void SetUp(){
            //ENTER_FUNC;
        }
        virtual void TearDown(){
            //ESC_FUNC;
        }

    public:
};

//////////////////////////////////////////////////////////////////////////////

//#undef LIYL_DEBUG
#define LIYL_DEBUG


typedef struct NdkDecoderContext {
    NdkMediaCodec  *dec;
    int64_t        curDts;
    int64_t        curPts;
    int32_t        inputFrameNum;
    int32_t        outputFrameNum;
    int32_t        frameWidth;
    int32_t        frameHeight;
    int32_t        frameRate;
    int32_t        bitRate;
    int32_t        iframeInterval;
    int32_t        colorFormat;
#ifdef LIYL_DEBUG
    int32_t        readPos;
    int32_t        nalSize;

    uint8_t*       pkt;
    int32_t        pktOffset;
    uint8_t*       frame;
    int32_t        frameSize;

    FILE*          fpInput;
    FILE*          fpOutput;
#endif
} NdkDecoderContext;

static void setUp(void* context){
#ifdef LIYL_DEBUG
    const char*  input_file_name = "/sdcard/input.h264";
    const char*  output_file_name = "/sdcard/output.yuv";
#endif
    NdkDecoderContext* me = (NdkDecoderContext*)context;
    CHECK(me != NULL);
#ifdef LIYL_DEBUG
    me->readPos    = 0;
    me->nalSize    = 0;
    me->pkt        = NULL;
    me->pktOffset  = 0;
    me->frame      = NULL;
    me->frameSize  = 0;
    me->fpInput    = fopen(input_file_name, "rb");
    if(me->fpInput == NULL) {
        LLOGE("file open failed %s",input_file_name);
    }
    me->fpOutput   = fopen(output_file_name, "wb");
    if(me->fpOutput == NULL) {
        LLOGE("file open failed %s",output_file_name);
    }
#endif
    me->curDts = 0ll;
    me->curPts = 0ll;
    me->inputFrameNum = 0;
    me->outputFrameNum= 0;
}
static void tearDown(void* context){
    NdkDecoderContext* me = (NdkDecoderContext*)context;
    CHECK(me != NULL);
#ifdef LIYL_DEBUG
    me->readPos    = 0;
    me->nalSize    = 0;
    me->pkt        = NULL;
    me->pktOffset  = 0;
    me->frame      = NULL;
    me->frameSize  = 0;
    if(me->fpInput != NULL) {
        fflush(me->fpInput);
        fclose(me->fpInput);
    }
    me->fpInput = NULL;

    if(me->fpOutput != NULL) {
        fflush(me->fpOutput);
        fclose(me->fpOutput);
    }
    me->fpOutput= NULL;
#endif
    me->curDts = 0ll;
    me->curPts = 0ll;
    me->inputFrameNum = 0;
    me->outputFrameNum= 0;

}

#ifdef LIYL_DEBUG
static int ndk_h264_decode_extradata(NdkDecoderContext* me,uint8_t* extradata,int32_t extradata_size) {
    uint8_t* data = (uint8_t*)extradata;
    fseek(me->fpInput, 0, SEEK_SET);
    int size = fread(data,extradata_size ,1, me->fpInput);
    me->readPos += size*extradata_size;
    return size*extradata_size;

}
#endif

static int onFillThisInputBuffer(void* data,int32_t offset,size_t size,int32_t* realSize,int64_t* presentationTimeUs,void* context) {
    NdkDecoderContext* me = (NdkDecoderContext*)context;
    // Read the input frame.
    int32_t bytesRead = -1; 
#ifdef LIYL_DEBUG
    //memcpy
    bytesRead = me->nalSize >= size? size : me->nalSize;
    memcpy((uint8_t*)data+offset,me->pkt, bytesRead);
#endif
    *realSize = bytesRead;
    me->curDts += 1000*1000/me->frameRate;
    *presentationTimeUs = me->curDts;//TODO 
    me->inputFrameNum++;
    uint8_t* aaa = (uint8_t*)data;
    LLOGD("nal: %x %x %x %x %x %x %x %x %x %x",aaa[0],aaa[1],aaa[2],aaa[3],aaa[4],aaa[5],aaa[6],aaa[7],aaa[8],aaa[9]);
    LLOGD("inputFramNum %d bytesRead %d frameSize %zu pts=%ld current render pts = %ld",me->inputFrameNum,bytesRead,size,*presentationTimeUs,me->curPts);
    if (bytesRead < 0) {
        LLOGD("inputFramNum %d bytesRead %d frameSize %zu pts=%ld",me->inputFrameNum,bytesRead,size,*presentationTimeUs);
        return UNKNOWN_ERROR;
    }   
    return OK;
}

static int onDrainThisOutputBuffer(void* data,int32_t offset,size_t size,int64_t presentationTimeUs,uint32_t flags,void* context) {
    if(flags == MediaCodec::BUFFER_FLAG_EOS) {
        LLOGD("EOS REACH\n");
        return UNKNOWN_ERROR;
    }
    NdkDecoderContext* me = (NdkDecoderContext*)context;

    me->outputFrameNum++;
    me->curPts = presentationTimeUs;
    int ret = OK;    
    CHECK(data != NULL);
    uint8_t* yuvData = (uint8_t*)data+offset;
    int32_t yuvSize = (me->frameWidth * me->frameHeight)*3/2;
    if (size > 0 && size == yuvSize ) {
#ifdef LIYL_DEBUG
        fwrite(yuvData, yuvSize, 1, me->fpOutput); //YUV
        fflush( me->fpOutput); //YUV
#endif
        LLOGD("success data size(%lu) VS YUVSize(%d) outputFrameNum(%d)\n",size,yuvSize,me->outputFrameNum);
    }else {
        LLOGE("failed  data size(%lu) VS YUVSize(%d) outputFrameNum(%d)\n",size,yuvSize,me->outputFrameNum);
        ret = UNKNOWN_ERROR;
    }
    return ret;
}


static int ndkDecoder_init(void *ctx)
{
    NdkDecoderContext * ndkDecoderContext = (NdkDecoderContext*)(ctx);
    setUp(ndkDecoderContext);
    NdkMediaCodec* decoder = new NdkMediaCodec;
    ndkDecoderContext->frameWidth    = 1920;
    ndkDecoderContext->frameHeight   = 1080;
    ndkDecoderContext->frameRate     = 24;
    ndkDecoderContext->bitRate       = 4000*1000;

    int frameWidth     = ndkDecoderContext->frameWidth;
    int frameHeight    = ndkDecoderContext->frameHeight;
    int frameRate      = ndkDecoderContext->frameRate;
    int bitRate        = ndkDecoderContext->bitRate;
    int ret = 0;
    if(decoder != NULL) {
        NdkMediaCodec_init(decoder);
        NdkMediaCodec_setCallBackFunctions(decoder,onFillThisInputBuffer,onDrainThisOutputBuffer,ndkDecoderContext);
        int extradata_size = 16*1024;
        uint8_t* outdata = (uint8_t*)malloc(extradata_size);
        memset(outdata,0,extradata_size);

        int nal_length_size = ndk_h264_decode_extradata(ndkDecoderContext,outdata,0x294);
        uint8_t* aaa = (uint8_t*)(outdata);
        for( int i = 0 ;i < nal_length_size-4;i+=4) {
            LLOGD("nal_length_size %d : %x %x %x %x",nal_length_size,aaa[i+0],aaa[i+1],aaa[i+2],aaa[i+3]);
        }
        ret = NdkMediaCodec_setup(decoder,NULL,frameWidth,frameHeight,frameRate,bitRate,outdata,nal_length_size);
        if(ret != OK) {
            delete decoder;
        }
        free(outdata);
        outdata = NULL;
    }else {
        LLOGD("decoder malloc failed,out of memory!!!");
        ret = UNKNOWN_ERROR;
    }
    ndkDecoderContext->dec = decoder;
    return ret;
}


static int ndkDecoder_frame(void *ctx,void *frame, int *outdata_size, void *pkt)
{
    NdkDecoderContext * ndkDecoderContext = (NdkDecoderContext*)(ctx);
    CHECK(ndkDecoderContext != NULL);
    NdkMediaCodec* decoder = ndkDecoderContext->dec;
    int ret = 0;
    *outdata_size = 0;
    if(decoder != NULL) {
        ret = NdkMediaCodec_execute(decoder);    
        if(ret != OK) {
            free(decoder);
            decoder = NULL;
            ndkDecoderContext->dec = NULL;
        }else {
            *outdata_size = 1;
            LLOGD("outdata_size = %d",*outdata_size); 

        }
    }else {
        LLOGE("decoder is NULL !!!"); 
        ret = UNKNOWN_ERROR;
    }
    return *outdata_size;
}

static int ndkDecoder_flush(void *ctx)
{
    NdkDecoderContext * ndkDecoderContext = (NdkDecoderContext*)(ctx);
    CHECK(ndkDecoderContext != NULL);
    int ret = 0;
    NdkMediaCodec* decoder = ndkDecoderContext->dec;
    if(decoder != NULL) {
        ret = NdkMediaCodec_flush(decoder);
    }else {
        LLOGE("decoder was gone"); 
    }
    return 0;
}

static int ndkDecoder_close(void *ctx)
{
    NdkDecoderContext * ndkDecoderContext = (NdkDecoderContext*)(ctx);
    CHECK(ndkDecoderContext != NULL);
    int ret = 0;
    NdkMediaCodec* decoder = ndkDecoderContext->dec;
    if(decoder != NULL) {
        ret = NdkMediaCodec_shutdown(decoder);
        if( ret == OK) {
            NdkMediaCodec_destroy(decoder);
        }
        delete decoder;
        ndkDecoderContext->dec = NULL;
    }else {
        LLOGE("decoder was destroyed"); 
    }
    tearDown(ndkDecoderContext);
    return 0;
}


#ifdef LIYL_DEBUG
#define PKTMAXSIZE 8000*1024
static int getNextNal(NdkDecoderContext *me,uint8_t* dataBase) {
    me->readPos += me->nalSize;
    me->pkt      = dataBase + me->pktOffset;
    me->nalSize  = 0;
    if(me->readPos == 0) {
        fseek(me->fpInput, me->readPos, SEEK_SET);
        int ret = fread(me->pkt, 1, PKTMAXSIZE, me->fpInput);
        LLOGD("fread ret = %d",ret);
        if (ret <= 0) {
            me->nalSize = 0;
            return me->nalSize;
        }
    }
    uint8_t* pkt = me->pkt;
    int remainder = PKTMAXSIZE-me->pktOffset-4;
    for(int i = 0;i < remainder; i++) {
        if(pkt[i] == 0 && pkt[i+1] == 0 && pkt[i+2] == 1 ) {
            me->readPos += i;
            me->nalSize = 3;
            me->pkt = pkt+i;
            break;
        }
        if(pkt[i] == 0 && pkt[i+1] == 0 && pkt[i+2] == 0 && pkt[i+3] == 1) {
            me->readPos += i;
            me->nalSize = 4;
            me->pkt = pkt+i;
            break;
        }
    }
    int startCode = me->nalSize;
    me->nalSize   = 0;
    pkt = me->pkt;
    remainder = PKTMAXSIZE-me->pktOffset-4;
    //LLOGD(" me->pktOffset = %d  remainder = %d ",me->pktOffset,remainder);
    for(int i = startCode; i < remainder; i++) {
        if(pkt[i] == 0 && pkt[i+1] == 0 && pkt[i+2] == 1 ) {
            me->nalSize = i;
            me->pktOffset += i;
            break;
        }
        if(pkt[i] == 0 && pkt[i+1] == 0 && pkt[i+2] == 0 && pkt[i+3] == 1) {
            me->nalSize = i;
            me->pktOffset += i;
            break;
        }

    }
    return me->nalSize;

}


#endif

//NdkDecoder test 
void runDecoder() {
#ifdef LIYL_DEBUG
    NdkDecoderContext * me = (NdkDecoderContext*)malloc(sizeof(NdkDecoderContext));
    if(me != NULL) {
        ndkDecoder_init(me);
        me->pktOffset    = 0;
        me->pkt          = (uint8_t*)malloc(PKTMAXSIZE);
        uint8_t* tmp     = me->pkt;
        memset(tmp,0,PKTMAXSIZE);
        int outdata_size = -1;
        int err          = 1;
        int nalSize      = 0;
        while(err == 1) {
            nalSize      = getNextNal(me,tmp);
            if(nalSize <= 0){
                err = 0;
                break;
            }
            err          = ndkDecoder_frame(me,/*frame*/NULL, &outdata_size, me->pkt);
        }
        ndkDecoder_flush(me);
        ndkDecoder_close(me);
        free(tmp);
    } else {
        LLOGE("out of memory !!!");
    }
    free(me);
    me = NULL;
#endif

}


//////////////////////////////////////////////////////////////////////////////
TEST_F(MediaCodecTest, test0) {
    runDecoder();

}


