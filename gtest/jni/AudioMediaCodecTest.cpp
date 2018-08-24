/****************************************************
 *
 *  @Author: liyl- liyunlong880325@gmail.com
 *  Last modified: 2018-08-18 18:10
 *  @Filename: AudioMediaCodecTest.cpp
 *****************************************************/

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

class AudioMediaCodecTest : public ::testing::Test {
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
    int32_t        sampleRate;
    int32_t        channelCount;
    int32_t        frameRate;
    int32_t        bitRate;
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
    const char*  input_file_name = "/sdcard/input.aac";
    const char*  output_file_name = "/sdcard/output.pcm";
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
static int ndk_aac_decode_extradata(NdkDecoderContext* me,uint8_t* extradata,int32_t extradata_size) {
    extradata_size  =  2;
    // AudioSpecificInfo follows
    // oooo offf fccc c000
    // o - audioObjectType
    // f - samplingFreqIndex
    // c - channelConfig

    //4c80
    unsigned profile =  1;
    unsigned sampling_freq_index = 3;
    unsigned channel_configuration = 2;

    extradata[0] = ((profile + 1) << 3) | (sampling_freq_index >> 1);
    extradata[1] = ((sampling_freq_index << 7) & 0x80) | (channel_configuration << 3);

    return extradata_size;

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
    uint8_t* pcmData = (uint8_t*)data+offset;
    if (size > 0 ) {
#ifdef LIYL_DEBUG
        fwrite(pcmData, size, 1, me->fpOutput); //PCM
        fflush( me->fpOutput); //PCM
#endif
        LLOGD("success data size(%lu) outputFrameNum(%d)\n",size,me->outputFrameNum);
    }else {
        LLOGE("failed  data size(%lu) outputFrameNum(%d)\n",size,me->outputFrameNum);
        ret = UNKNOWN_ERROR;
    }
    return ret;
}


static int ndkDecoder_init(void *ctx)
{
    NdkDecoderContext * ndkDecoderContext = (NdkDecoderContext*)(ctx);
    setUp(ndkDecoderContext);
    NdkMediaCodec* decoder = new NdkMediaCodec;
    ndkDecoderContext->sampleRate    = 48000;
    ndkDecoderContext->channelCount  = 2;
    ndkDecoderContext->frameRate     = 50;
    ndkDecoderContext->bitRate       = 200*1000;

    int sampleRate     = ndkDecoderContext->sampleRate;
    int channelCount   = ndkDecoderContext->channelCount;
    int frameRate      = ndkDecoderContext->frameRate;
    int bitRate        = ndkDecoderContext->bitRate;
    int ret = 0;
    if(decoder != NULL) {
        NdkMediaCodec_init(decoder);
        NdkMediaCodec_setCallBackFunctions(decoder,onFillThisInputBuffer,onDrainThisOutputBuffer,ndkDecoderContext);
        decoder->isVideo = false;
        int extradata_size = 2*1024;
        uint8_t* outdata = (uint8_t*)malloc(extradata_size);
        memset(outdata,0,extradata_size);

        int nal_length_size = ndk_aac_decode_extradata(ndkDecoderContext,outdata,0x0/*unused*/);
        uint8_t* aaa = (uint8_t*)(outdata);
        for( int i = 0 ;i < nal_length_size-4;i+=4) {
            LLOGD("nal_length_size %d : %x %x %x %x",nal_length_size,aaa[i+0],aaa[i+1],aaa[i+2],aaa[i+3]);
        }
        ret = NdkMediaCodec_setup(decoder,NULL,sampleRate,channelCount,frameRate,bitRate,outdata,nal_length_size);
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


static size_t getAdtsFrameLength(uint8_t* data, int32_t offset, size_t* headerSize) {

    const size_t kAdtsHeaderLengthNoCrc = 7;
    const size_t kAdtsHeaderLengthWithCrc = 9;

    size_t frameSize = 0;

    uint8_t* startData = data+offset;
    uint8_t syncword[2] = {startData[0],startData[1]};
    if ((syncword[0] != 0xff) || ((syncword[1] & 0xf6) != 0xf0)) {
        return 0;
    }

    uint8_t protectionAbsent = startData[1];
    protectionAbsent &= 0x1;

    uint8_t header[3] = {startData[3],startData[4],startData[5]};

    frameSize = (header[0] & 0x3);
    frameSize = frameSize << 11 | header[1] << 3 | header[2] >> 5;

    // protectionAbsent is 0 if there is CRC
    size_t headSize = protectionAbsent ? kAdtsHeaderLengthNoCrc : kAdtsHeaderLengthWithCrc;
    if (headSize > frameSize) {
        return 0;
    }
    if (headerSize != NULL) {
        *headerSize = headSize;
    }

    return frameSize;
}
#define PKTMAXSIZE 8000*1024
static int getNextNal(NdkDecoderContext *me,uint8_t* dataBase) {
    me->readPos += me->nalSize;
    me->pkt      = dataBase + me->pktOffset;
    me->nalSize  = 0;
    if(me->pktOffset == 0) {
        fseek(me->fpInput, me->readPos, SEEK_SET);
        int ret = fread(me->pkt, 1, PKTMAXSIZE, me->fpInput);
        LLOGD("fread ret = %d",ret);
        if (ret <= 0) {
            me->nalSize = 0;
            return me->nalSize;
        }
    }
    uint8_t* pkt = me->pkt;
    size_t frameLength = getAdtsFrameLength(pkt,0,NULL);
    me->nalSize = frameLength;
    me->pktOffset += frameLength;

    return me->nalSize;

}


#endif

//NdkDecoder test 
void runAudioDecoder() {
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
TEST_F(AudioMediaCodecTest, test0) {
    runAudioDecoder();

}
