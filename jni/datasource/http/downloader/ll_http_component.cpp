/*************************************************************************
  > File Name: ll_http_component.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com
  > Created Time: 2017年06月02日 星期五 18时04分31秒
 ************************************************************************/

#include "ll_curl_wrapper.h"
#include "ll_http_component.h"
#include "dl_utils.h"
#include <vector>
using std::vector;


#undef LOG_TAG
#define LOG_TAG "HTTPComponent"
#define DATA_BUFFER_MAX_SIZE CURL_MAX_WRITE_SIZE*10


LIYL_NAMESPACE_START
/*static*/
//UNUSED because that  onEvent don't has member variable assignment.
//YKMutex HTTPComponent::mOnEventLock;

HTTPComponent::HTTPComponent(
        const char *name,
        const DL_CALLBACKTYPE *callbacks,
        DL_PTR appData,
        DL_COMPONENTTYPE **component):DefaultComponent(name,callbacks,appData,component){
    ENTER_FUNC;
    LLOGD("HTTPComponent constructor !");
    mCurlHandle = DL_CreateHandle();

    mLastestHeadBuffer = new DL_BUFFERHEADERTYPE;
    mLastestHeadBuffer->pPreNode = NULL;
    mLastestHeadBuffer->pNextNode= NULL;
    mLastestDataBuffer = new DL_BUFFERHEADERTYPE;
    mLastestDataBuffer->pPreNode = NULL;
    mLastestDataBuffer->pNextNode= NULL;

    //init swap data buffer 
    mLastestDataBuffer->pBuffer = NULL;
    mLastestDataBuffer->nAllocLen = 0;
    mLastestDataBuffer->nFilledLen = 0;
    mLastestDataBuffer->nOffset = 0;
    mLastestDataBuffer->nFlags = (DL_U32)0;


    mFilledSum = 0l;
    mConsumedSum = 0l;

    mFreeFlag = DL_FALSE;


    mState = DL_StateInit;
    mTargetState = DL_StateInit;
    ESC_FUNC;
}

HTTPComponent::~HTTPComponent() {
    ENTER_FUNC;
    LLOGD("HTTPComponent deconstructor !");
    if(mLastestHeadBuffer != NULL) {
        mLastestHeadBuffer->pPreNode = NULL;
        mLastestHeadBuffer->pNextNode= NULL;
        delete mLastestHeadBuffer;
    }

    if(mLastestDataBuffer != NULL) {
        //init swap data buffer 
        mLastestDataBuffer->pBuffer = NULL;
        mLastestDataBuffer->nAllocLen = 0;
        mLastestDataBuffer->nFilledLen = 0;
        mLastestDataBuffer->nOffset = 0;
        mLastestDataBuffer->nFlags = (DL_U32)0;


        mLastestDataBuffer->pPreNode = NULL;
        mLastestDataBuffer->pNextNode= NULL;
        delete mLastestDataBuffer;
    }


    ESC_FUNC;
}

void HTTPComponent::prepareForDestruction() {
    Mutex::Autolock autoLock(mLock);
    if(mCurlHandle != NULL)
        DL_CloseHandle(mCurlHandle);
    //memory free

    {
        Mutex::Autolock autolock(mFreeDataLock);
        mFreeFlag = DL_TRUE;
        DL_BUFFERHEADERTYPE* headNode = NULL;
        for(headNode = (DL_BUFFERHEADERTYPE*)(mLastestHeadBuffer->pPreNode);headNode != NULL;) {
            DL_BUFFERHEADERTYPE* preHeadNode =(DL_BUFFERHEADERTYPE*)(headNode->pPreNode);
            LLOGD("free headNode = %p nFlags = %lu pBuffer = %p",headNode,headNode->nFlags,headNode->pBuffer);
            headNode->pPreNode = NULL;
            headNode->pNextNode= NULL;
            free(headNode->pBuffer);
            delete ((DL_BUFFERHEADERTYPE*)(headNode));
            mLastestHeadBuffer->pPreNode = NULL;
            if(preHeadNode != NULL) {
                mLastestHeadBuffer->pPreNode = preHeadNode;
                preHeadNode->pNextNode = mLastestHeadBuffer;
            }
            headNode = preHeadNode;
        }

        mLastestHeadBuffer->pPreNode = NULL;
        mLastestHeadBuffer->pNextNode= NULL;
        delete (mLastestHeadBuffer);
        mLastestHeadBuffer = NULL;

        DL_BUFFERHEADERTYPE* bufNode = NULL;

        for(bufNode = (DL_BUFFERHEADERTYPE*)(mLastestDataBuffer->pPreNode);bufNode != NULL;) {
            DL_BUFFERHEADERTYPE* preBufNode =(DL_BUFFERHEADERTYPE*)(bufNode->pPreNode);
            LLOGD("free bufNode = %p nFlags = %lu bufNode->pBuffer=%p ",bufNode,bufNode->nFlags,bufNode->pBuffer);
            bufNode->pPreNode = NULL;
            bufNode->pNextNode= NULL;
            free(bufNode->pBuffer);
            delete ((DL_BUFFERHEADERTYPE*)(bufNode));
            mLastestDataBuffer->pPreNode = NULL;
            if(preBufNode != NULL) {
                mLastestDataBuffer->pPreNode = preBufNode;
                preBufNode->pNextNode = mLastestDataBuffer;
            }
            bufNode = preBufNode;
        }
        //free swap data buffer
        if(mLastestDataBuffer->nFlags & DL_BUFFERFLAG_FLUSHBUFFER) {
            CHECK(mLastestDataBuffer->pBuffer != NULL );
            free(mLastestDataBuffer->pBuffer);
        }
        mLastestDataBuffer->pBuffer = NULL;
        mLastestDataBuffer->nAllocLen = 0;
        mLastestDataBuffer->nFilledLen = 0;
        mLastestDataBuffer->nOffset = 0;
        mLastestDataBuffer->nFlags = (DL_U32)0;



        mLastestDataBuffer->pPreNode = NULL;
        mLastestDataBuffer->pNextNode= NULL;
        delete (mLastestDataBuffer);
        mLastestDataBuffer = NULL;

        mFilledSum = 0l;
        mConsumedSum = 0l;


        mState = DL_StateDeinit;
        mTargetState = DL_StateDeinit;
        mCurlHandle = NULL;
    }


}

DL_ERRORTYPE HTTPComponent::initCheck() const {
    if(mState == DL_StateInit && mCurlHandle == NULL) {
        return DL_ErrorInvalidState;
    }
    return DL_ErrorNone;
}

DL_ERRORTYPE HTTPComponent::setConsumedBytes(DL_S32 size) {
    if(size < 0 ) {
        return DL_ErrorBadParameter;
    }
    Mutex::Autolock autolock(mFreeDataLock);
    if(mFreeFlag == DL_TRUE) {
        return DL_ErrorInvalidState;
    }
    mConsumedSum += size;
    return DL_ErrorNone;
}

DL_ERRORTYPE HTTPComponent::sendCommand(
        DL_COMMANDTYPE  cmd , DL_U32  param , DL_PTR  data ) {
    //changeState
    ENTER_FUNC;
    (void)data;
    Mutex::Autolock autoLock(mLock);
    onSendCommand(cmd,(DL_STATETYPE)param);
    ESC_FUNC;
    return DL_ErrorNone;
}
DL_ERRORTYPE HTTPComponent::getConfig( DL_INDEXTYPE  index , DL_PTR  params ) {
    //obatin mCurlHandle config by index;
    if(params == NULL) {
        return DL_ErrorBadParameter;
    }
    Mutex::Autolock autoLock(mLock);
    switch (index) {
        case DL_IndexComponentDownloadInfo:
            {
                LLOGD("DL_IndexComponentDownloadInfo");
                return getDownloadInfo_l(params);
            }

        case DL_IndexComponentInitInfo:
            {
                LLOGD("DL_IndexComponentInitInfo");
                return DL_ErrorNone;
            }
        case DL_IndexComponentDownloadSpeedGet:
            {
                LLOGD("DL_IndexComponentDownloadSpeedGet");

                DL_ERRORTYPE ret = getDownloadSpeed_l(params);
                return ret;
            }


        default:
            return DL_ErrorNone;
    }
}

DL_ERRORTYPE HTTPComponent::setConfig(
        DL_INDEXTYPE  index , const DL_PTR  params ) {
    //set mCurlHandle config by index;
    Mutex::Autolock autoLock(mLock);
    switch (index) {
        case DL_IndexComponentDownloadInfo:
            {
                LLOGD("DL_IndexComponentDownloadInfo");
                return DL_ErrorNone;
            }

        case DL_IndexComponentInitInfo:
            {
                LLOGD("DL_IndexComponentInitInfo");
                return DL_ErrorNone;
            }
        case DL_IndexComponentDnsIPlist:
            {
                LLOGD("DL_IndexComponentDnsIPlist");

                return  configHttpDnsIplist_l(params);


            }
        case DL_IndexComponentDownloadSpeedSetting:
            {
                LLOGD("DL_IndexComponentNetSpeedSetting");

                return  configDownloadSpeed_l(params);

            }

        default:
            return DL_ErrorNone;
    }

}


DL_ERRORTYPE HTTPComponent::getDownloadInfo_l(DL_PTR params) {
    ENTER_FUNC;
    HTTPDownloadStatusInfo* info = (HTTPDownloadStatusInfo*)params;

    int ret  = DL_GetDownloadInfo(mCurlHandle,info);
    if(ret == DL_INVAL_HANDLE) {
        ESC_FUNC;
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        ESC_FUNC;
        return DL_ErrorBadParameter;
    }else if(ret == DL_OK){
        ESC_FUNC;
        return DL_ErrorNone;
    }else {
        ESC_FUNC;
        return DL_ErrorUnKnown;
    }

}

DL_ERRORTYPE HTTPComponent::getDownloadSpeed_l(DL_PTR params) {
    ENTER_FUNC;
    DL_S64* speed = (DL_S64*)params;

    double tmp = 0.0;
    int ret  = DL_GetDownloadSpeed(mCurlHandle,&tmp);
    if(ret == DL_INVAL_HANDLE) {
        ESC_FUNC;
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        ESC_FUNC;
        return DL_ErrorBadParameter;
    }else if(ret == DL_OK){
        *speed = static_cast<DL_S64>(tmp);
        LLOGD("download speed = %lld",*speed);
        ESC_FUNC;
        return DL_ErrorNone;
    }else {
        ESC_FUNC;
        return DL_ErrorUnKnown;
    }

}

DL_ERRORTYPE HTTPComponent::configHttpDnsIplist_l(DL_PTR params) {
    ENTER_FUNC;
    int ret = DL_OK;
    //TODO
    /*
       if(dnsiplist == NULL) {
       return DL_ErrorBadParameter;
       }
std:string hostName = dnsiplist->hostName;
std::vector<std::string> ipList;
std::vector<std::string>* tmp = static_cast<std::vector<std::string> *>(dnsiplist->ipList) ;
ipList.swap(*tmp);
ret =  DL_AddResolves(mCurlHandle, hostName,ipList);
*/
    if(ret == DL_INVAL_HANDLE) {
        ESC_FUNC;
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        ESC_FUNC;
        return DL_ErrorBadParameter;
    }else if(ret == DL_OK){
        ESC_FUNC;
        return DL_ErrorNone;
    }else {
        ESC_FUNC;
        return DL_ErrorUnKnown;
    }

}


DL_ERRORTYPE HTTPComponent::configDownloadSpeed_l(DL_PTR params) {
    ENTER_FUNC;

    int ret  = DL_OK;
    if(ret == DL_INVAL_HANDLE) {
        LLOGE("ret Error DL_INVAL_HANDLE");
        return DL_ErrorInsufficientResources;
    }else if(ret == DL_OK){
        ESC_FUNC;
        return DL_ErrorNone;
    }else {
        LLOGE("should not be here!!!");
        return DL_ErrorUnKnown;
    }

}




DL_ERRORTYPE HTTPComponent::getState(DL_STATETYPE * state ) {
    //get HTTPComponent current state
    ENTER_FUNC;
    Mutex::Autolock autolock(mLock);
    *state = mState;
    ESC_FUNC;
    return DL_ErrorNone;
}


DL_ERRORTYPE HTTPComponent::prepareLoading( DataSpec*  dataSpec) {
    ENTER_FUNC;
    //check state
    Mutex::Autolock autolock(mLock);
    // reuse component when mState == DL_StateIdle
    CHECK((mState == DL_StateInit || mState == DL_StateIdle) && mTargetState == mState);
    CHECK(dataSpec != NULL);
    std::vector<std::string> headerProps;
    std::string userAgent;
    if(dataSpec->headUserAgent != NULL) {
        userAgent = dataSpec->headUserAgent;
    }else {
        userAgent = "User-Agent: Android6.0";
    }
    headerProps.push_back(userAgent);
    //dataSpec->connectTimeoutMs
    int ret = DL_PrepareDownload(mCurlHandle,  dataSpec->url, dataSpec->position, dataSpec->length,false ,onEventCB , this, headerProps/*headerProps*/);

    onChangeState(DL_StateIdle);
    if(ret == DL_INVAL_HANDLE) {
        LLOGE("ret DL_INVAL_HANDLE");
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        LLOGE("ret DL_INVAL_PARAM");
        return DL_ErrorBadParameter;
    }else if(ret == DL_ALREADY_PREPARED) {
        LLOGW("my curlDownloader running already");
        return DL_ErrorNone;
    }else if(ret == DL_OK){
        ESC_FUNC;
        return DL_ErrorNone;
    }else {
        LLOGE("my curlDownloader should not be here!!!");
        return DL_ErrorUnKnown;
    }
}

DL_ERRORTYPE HTTPComponent::startLoading(
        DataSpec* /* dataSpec*/) {
    ENTER_FUNC;
    //check state
    //call DLMETHOD
    //int DL_StartDownload(DLHandle handle,bool isAsync);
    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateIdle&& mTargetState == mState);
    int ret =  DL_StartDownload(mCurlHandle,true);


    onChangeState(DL_StateExecuting);

    ESC_FUNC;

    if(ret == DL_OK) {
        return DL_ErrorNone;
    }else if(ret == DL_INVAL_HANDLE) {
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        return DL_ErrorBadParameter;
    }else if(ret == DL_ALREADY_STARTED) {
        LLOGW("my curlDownloader running already");
        return DL_ErrorNone;
    }else if(ret >= DL_EHTTP_OVER_REDIRECT && ret < DL_EHTTP_MAX){
        return DL_ErrorProtocol;
    }else {
        return DL_ErrorUnKnown;
    }
}

DL_ERRORTYPE HTTPComponent::stopLoading() {
    ENTER_FUNC;
    //check state
    int ret = DL_OK;
    {
        Mutex::Autolock autolock(mLock);

        CHECK(mState == DL_StateExecuting && mTargetState == mState);
        if(mState != DL_StateExecuting ) {

            LLOGD("mState = %d  curlDownloader stoping already",mState);
            return DL_ErrorNone;
        }
        ret =  DL_StopDownload(mCurlHandle,3000000000ll);

        onChangeState(DL_StateIdle);

    }
    if(ret == DL_OK) {
        //WARNING ME  must not be locked, mutex with ll_load_manager !!!
        notify(DL_EventCmdComplete, DL_ET_STOP_CMPL, 0/*unused*/, NULL);
        ESC_FUNC;
        return DL_ErrorNone;
    }else if(ret == DL_INVAL_HANDLE) {
        LLOGE("DL_INVAL_HANDLE");
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        LLOGE("DL_INVAL_PARAM");
        return DL_ErrorBadParameter;
    }else if(ret >= DL_EHTTP_OVER_REDIRECT && ret < DL_EHTTP_MAX){
        LLOGE("Protocol ERROR");
        return DL_ErrorProtocol;
    }else if(ret == DL_TIMEOUT) {
        LLOGE("DL_TIMEOUT");
        if(mState == DL_StateIdle) {
            notify(DL_EventCmdComplete, DL_ET_STOP_CMPL, 0/*unused*/, NULL);
        }
        return DL_ErrorTimeout;
    }else {
        LLOGE("should not be here !!!");
        return DL_ErrorUnKnown;
    }


}

DL_ERRORTYPE HTTPComponent::pauseLoading() {
    ENTER_FUNC;
    //check state
    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateExecuting && mTargetState == mState);
    int ret = DL_ErrorUnKnown; 

    ESC_FUNC;

    if(ret == DL_OK) {
        return DL_ErrorNone;
    }else {
        return DL_ErrorUnKnown;
    }


}


DL_ERRORTYPE HTTPComponent::resumeLoading() {
    ENTER_FUNC;
    //check state
    //call DLMETHOD
    //int DL_ResumeDownload(DLHandle handle);
    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateExecuting && mTargetState == mState);
    int ret = DL_ErrorUnKnown; 
    ESC_FUNC;
    if(ret == DL_OK) {
        return DL_ErrorNone;
    }else {
        return DL_ErrorUnKnown;
    }


}


/*static*/
void HTTPComponent::onEventCB(void* handle/*DLCurl*/, void* pContext/*pAPP*/, int type, void* data, int size)
{
    //TODO
    //DL_HTTPWHAT httpEvent;
    DL_EventType httpEvent;
    HTTPComponent* me = NULL;
    DLCurl* dlcurl = NULL;
    void* pData = NULL;


    {
        httpEvent = (DL_EventType)type;
        me = (HTTPComponent*)pContext;
        dlcurl = (DLCurl*) handle;
        pData = data;

    }

    if (dlcurl == NULL) {
        LLOGE("dlcurl is NULL)\n");
        return ;
    }
#define MEM_LL_THRESHOLD (1*1000*1000)
    switch (httpEvent) {
        case DL_ET_DATA:
            {//DL_HTTP_FTBD_CB

                LLOGV("FTBD");
                DL_S32 ll_len = 0;

                Mutex::Autolock autolock(me->mFreeDataLock);
                if(me->mFreeFlag == DL_TRUE) {
                    break;
                }

                ll_len = me->mFilledSum - me->mConsumedSum;
                if(ll_len >= MEM_LL_THRESHOLD) {
                    LLOGD("ll_len >= MEM_LL_THRESHOLD ll_len (%ld) = FilledSum(%ld) - ConsumedSum(%ld)"
                            ,ll_len,me->mFilledSum,me->mConsumedSum);
                    usleep(100000);
                }else {
                    LLOGV("filled ll_len = %ld",ll_len);
                }



                if(me->mLastestDataBuffer->pBuffer == NULL) {
                    DL_BYTE mem = (DL_BYTE)malloc(DATA_BUFFER_MAX_SIZE);
                    if(mem == NULL) {
                        LLOGE("not enough memory (realloc returned NULL)\n");
                        me->notify(DL_EventError,DL_ET_DATA,0u,NULL);
                        return ;
                    }
                    memset(mem,0,DATA_BUFFER_MAX_SIZE);
                    me->mLastestDataBuffer->pBuffer = mem;
                    me->mLastestDataBuffer->nAllocLen = DATA_BUFFER_MAX_SIZE;

                }

                memcpy(me->mLastestDataBuffer->pBuffer + me->mLastestDataBuffer->nOffset,pData, size);
                me->mFilledSum += size;
                me->mLastestDataBuffer->nOffset += size;
                me->mLastestDataBuffer->nFilledLen += size;
                CHECK(me->mLastestDataBuffer->nFilledLen <= DATA_BUFFER_MAX_SIZE);
                //nAllocLen - nOffset < CURL_MAX_WRITE_SIZE
                DL_BOOL isFilled = (me->mLastestDataBuffer->nOffset + CURL_MAX_WRITE_SIZE >= me->mLastestDataBuffer->nAllocLen) ? DL_TRUE : DL_FALSE;

                if(me->mLastestDataBuffer->pBuffer != NULL && (isFilled == DL_TRUE) ) {

                    DL_BUFFERHEADERTYPE *header = new DL_BUFFERHEADERTYPE;
                    (header)->nSize = sizeof(DL_BUFFERHEADERTYPE);
                    (header)->nVersion.s.nVersionMajor = 1;
                    (header)->nVersion.s.nVersionMinor = 0;
                    (header)->nVersion.s.nRevision = 0;
                    (header)->nVersion.s.nStep = 0;
                    (header)->pBuffer = (DL_BYTE)me->mLastestDataBuffer->pBuffer;
                    (header)->nAllocLen = me->mLastestDataBuffer->nAllocLen;
                    (header)->nFilledLen = me->mLastestDataBuffer->nFilledLen;
                    (header)->nOffset = 0;
                    (header)->pAppPrivate = NULL;
                    (header)->pDownloaderPrivate= me;
                    (header)->nFlags = DL_BUFFERFLAG_NEEDFREE;
                    (header)->pPreNode = NULL;
                    (header)->pNextNode= NULL;
                    LLOGV("BUFFER %p mem %p",header,me->mLastestDataBuffer->pBuffer);
                    LLOGV("Data pPreNode %p ",me->mLastestDataBuffer);
                    DL_BUFFERHEADERTYPE* lastestPreNode = (DL_BUFFERHEADERTYPE*)(me->mLastestDataBuffer->pPreNode);
                    if(lastestPreNode != NULL) {
                        header->pPreNode = lastestPreNode;
                        lastestPreNode->pNextNode = header;
                    }

                    me->mLastestDataBuffer->pPreNode = header;
                    header->pNextNode = me->mLastestDataBuffer;

                    //reset reset data buffer 
                    me->mLastestDataBuffer->pBuffer = NULL;
                    me->mLastestDataBuffer->nAllocLen = 0;
                    me->mLastestDataBuffer->nFilledLen = 0;
                    me->mLastestDataBuffer->nOffset = 0;
                    me->mLastestDataBuffer->nFlags = (DL_U32)0;

                    me->notifyFillBufferDone(header);
                }
                break;

            }
        case DL_ET_HEADER://DL_HTTP_HEADER_CB
            {
                //onProcessHTTPHeader ();
                LLOGV("HEADER_CB data++++++++++++++++++++++++: %s ", (DL_STRING)pData);
                Mutex::Autolock autolock(me->mFreeDataLock);
                if(me->mFreeFlag == DL_TRUE) {
                    break;
                }

                CHECK(strlen((DL_STRING)pData) == (unsigned int)size);
                DL_STRING tmpBuffer = (DL_STRING)malloc(size + 1);
                if(tmpBuffer == NULL) {
                    /* out of memory! */
                    LLOGE("not enough memory (malloc returned NULL)");
                    return ;
                }

                memset(tmpBuffer, 0, size+1);

                memcpy(tmpBuffer, (DL_STRING)pData, size);
                tmpBuffer[size] = 0;
                DL_BUFFERHEADERTYPE *header = new DL_BUFFERHEADERTYPE;
                (header)->nSize = sizeof(DL_BUFFERHEADERTYPE);
                (header)->nVersion.s.nVersionMajor = 1;
                (header)->nVersion.s.nVersionMinor = 0;
                (header)->nVersion.s.nRevision = 0;
                (header)->nVersion.s.nStep = 0;
                (header)->pBuffer = (DL_BYTE)tmpBuffer;
                (header)->nAllocLen = size;
                (header)->nFilledLen = size;
                (header)->nOffset = 0;
                (header)->pAppPrivate = NULL;
                (header)->pDownloaderPrivate= me;
                (header)->nFlags = DL_BUFFERFLAG_NEEDFREE;
                (header)->pPreNode = NULL;
                (header)->pNextNode= NULL;
                LLOGV("Header %p mem %p pPreNode",header,tmpBuffer,me->mLastestHeadBuffer);
                LLOGV("pPreNode %p ",me->mLastestHeadBuffer);

                DL_BUFFERHEADERTYPE* lastestPreNode = (DL_BUFFERHEADERTYPE*)(me->mLastestHeadBuffer->pPreNode);
                if(lastestPreNode != NULL) {
                    header->pPreNode = lastestPreNode;
                    lastestPreNode->pNextNode = header;
                }


                header->pNextNode = me->mLastestHeadBuffer;
                me->mLastestHeadBuffer->pPreNode = header;
                me->notify(DL_EventCmdComplete,DL_ET_HEADER,size,header);
                break;
            }
        case DL_ET_PROGRESS://DL_HTTP_PROGRESS_CB
            {
                //onProcessHTTPHeader ();
                //onProcessHTTPPROGRESS unused
                LLOGD("PROGRESS_CB");
                me->notify(DL_EventCmdComplete,DL_ET_PROGRESS,0u,NULL);
                break;
            }
        case DL_ET_CURL_DEBUG://DL_HTTP_DEBUG_CB
            {
                //onProccess: debug info 
                LLOGD("DEBUG_CB");

                break;
            }
        case DL_ET_DATA_CMPL://DL_HTTP_RESPONSE_CMPL_CB
            {
                LLOGD("DL_HTTP_RESPONSE_CMPL_CB");
                Mutex::Autolock autolock(me->mFreeDataLock);
                //free me->mLastestDataBuffer->pBuffer by Observer

                if(me->mLastestDataBuffer->pBuffer != NULL) {
                    //me->mLastestDataBuffer->nFlags |= DL_BUFFERFLAG_FLUSHBUFFER;                
                    DL_BUFFERHEADERTYPE *header = new DL_BUFFERHEADERTYPE;
                    (header)->nSize = sizeof(DL_BUFFERHEADERTYPE);
                    (header)->nVersion.s.nVersionMajor = 1;
                    (header)->nVersion.s.nVersionMinor = 0;
                    (header)->nVersion.s.nRevision = 0;
                    (header)->nVersion.s.nStep = 0;
                    (header)->pBuffer = (DL_BYTE)me->mLastestDataBuffer->pBuffer;
                    (header)->nAllocLen = me->mLastestDataBuffer->nAllocLen;
                    (header)->nFilledLen = me->mLastestDataBuffer->nFilledLen;
                    (header)->nOffset = 0;
                    (header)->pAppPrivate = NULL;
                    (header)->pDownloaderPrivate= me;
                    (header)->nFlags = DL_BUFFERFLAG_NEEDFREE;
                    (header)->pPreNode = NULL;
                    (header)->pNextNode= NULL;
                    LLOGV("BUFFER %p mem %p",header,me->mLastestDataBuffer->pBuffer);
                    LLOGV("Data pPreNode %p ",me->mLastestDataBuffer);
                    DL_BUFFERHEADERTYPE* lastestPreNode = (DL_BUFFERHEADERTYPE*)(me->mLastestDataBuffer->pPreNode);
                    if(lastestPreNode != NULL) {
                        header->pPreNode = lastestPreNode;
                        lastestPreNode->pNextNode = header;
                    }

                    me->mLastestDataBuffer->pPreNode = header;
                    header->pNextNode = me->mLastestDataBuffer;

                    //reset reset data buffer 
                    me->mLastestDataBuffer->pBuffer = NULL;
                    me->mLastestDataBuffer->nAllocLen = 0;
                    me->mLastestDataBuffer->nFilledLen = 0;
                    me->mLastestDataBuffer->nOffset = 0;
                    me->mLastestDataBuffer->nFlags = (DL_U32)0;
                    me->notifyFillBufferDone(header);
                }
                me->notify(DL_EventCmdComplete,DL_ET_DATA_CMPL,0u,NULL);
                break;
            }
        case DL_ET_ERR://DL_HTTP_RESPONSE_ERR_CB
            {
                //parse reponsecode
                //post msg
                int errCode = *(int*)pData;
                LLOGE("DL_HTTP_RESPONSE_ERR_CB errCode = %d",errCode);
                if(errCode != DL_OK){
                    Mutex::Autolock autolock(me->mFreeDataLock);
                    //free me->mLastestDataBuffer->pBuffer by prepareForDestruction
                    if(me->mLastestDataBuffer->pBuffer != NULL)
                        me->mLastestDataBuffer->nFlags |= DL_BUFFERFLAG_FLUSHBUFFER;      
                    me->notify(DL_EventError,DL_ET_ERR,errCode,NULL);
                }else {
                    //UNUSED
                }
                break;
            }

        default:
            {
                LLOGE("should not be here!");
                break;
            }
    }

    return;
}

void HTTPComponent::onReset() {
    ENTER_FUNC;
    DLCurl* dlCurl = (DLCurl*)mCurlHandle;
    if(dlCurl != NULL) {
        //TODO
        DL_ResetHandle(dlCurl);
    }

    ESC_FUNC;
    return;
}

void HTTPComponent::onSendCommand( DL_COMMANDTYPE cmd, DL_U32 param) {
    switch (cmd) {
        case DL_CommandStateSet:
            {
                onChangeState((DL_STATETYPE)param);
                break;
            }

        case DL_CommandMax:
            {
                LLOGE("should not be here!!");
                break;
            }
        default:
            LLOGE("should not be here!!");
            break;
    }
}

void HTTPComponent::onChangeState(DL_STATETYPE state) {
    if (mState == DL_StateInit
            && mTargetState == DL_StateIdle
            && state == DL_StateInit) {
        // specifically allows "canceling" a state transition from init
        // to idle. Pretend we made it to idle, and go back to init
        LLOGV("init->idle canceled");
        mState = mTargetState = DL_StateIdle;
        state = DL_StateInit;
    }
    // We shouldn't be in a state transition already.
    CHECK_EQ((int)mState, (int)mTargetState);

    switch (mState) {
        case DL_StateInit:
            CHECK_EQ((int)state , (int)DL_StateIdle);
            break;
        case DL_StateIdle:
            if(state == DL_StateInit || state == DL_StateExecuting) {
            }else {
                CHECK_EQ((int)state , (int)DL_StateIdle);
            }
            break;
        case DL_StateExecuting:
            {
                CHECK_EQ((int)state , (int)DL_StateIdle);

                //TODO stop or pause mCurlHandle
                DLCurl* dlCurl = (DLCurl*)mCurlHandle;
                if(dlCurl != NULL && dlCurl->state == DL_STATE_BUSY) {
                    //TODO
                    DL_StopDownload(mCurlHandle,0ll);

                }
                //NEEDFIXED
                mState = DL_StateIdle; //stop is take a long time ,so immediately deal with it.
                break;
            }

        default:
            LLOGE("should not be here\n");
    }

    mTargetState = state;

    checkTransitions();
}

void HTTPComponent::checkTransitions() {
    if (mState != mTargetState) {
        bool transitionComplete = true;

        if (mState == DL_StateInit) {
            CHECK_EQ((int)mTargetState, (int)DL_StateIdle);
            //TODO check mCurlHandle prepare
            DLCurl* dlCurl = (DLCurl*)mCurlHandle;
            if(dlCurl != NULL && dlCurl->state == DL_STATE_BUSY) {
                //TODO
                transitionComplete = false;
            }

        } else if (mTargetState == DL_StateInit) {
            CHECK_EQ((int)mState, (int)DL_StateIdle);
            //TODO check mCurlHandle isClear,clear some memory,reset state
            DLCurl* dlCurl = (DLCurl*)mCurlHandle;
            if(dlCurl != NULL && (dlCurl->state == DL_STATE_BUSY)) {

                transitionComplete = false;
            }

        }

        if (transitionComplete) {
            mState = mTargetState;

            if (mState == DL_StateInit) {
                onReset();
            }

            notify(DL_EventCmdComplete, DL_CommandStateSet, mState, NULL);
        }else {

            //TODO
            LLOGE("DL_CommandStateSet error mTargetState is 0x%x",mTargetState);
            //notify(DL_EventError, DL_CommandStateSet, mTargetState, NULL);
        }
    }else {
        LLOGV("already mstate == mtargetState = %d !!! line = %d ",mState,__LINE__);
    }

}
#undef CHECK_EQ

LIYL_NAMESPACE_END

liyl::DefaultComponent* createDLHTTPComponent(
        const char *name, const DL_CALLBACKTYPE *callbacks, DL_PTR appData,
        DL_COMPONENTTYPE **component) {
    return new liyl::HTTPComponent(name, callbacks, appData, component);
}


