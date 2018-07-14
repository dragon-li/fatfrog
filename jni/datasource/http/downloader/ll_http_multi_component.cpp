/*************************************************************************
  > File Name: ll_http_component.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com
  > Created Time: 2017年06月02日 星期五 18时04分31秒
 ************************************************************************/

#include "ll_http_multi_component.h"
#include "ll_curlmulti.h"
#include "dl_utils.h"
#include <vector>

#undef LOG_TAG
#define LOG_TAG "LLMultiHTTPComponent"

using std::vector;

LIYL_NAMESPACE_START
/*static*/

MultiHTTPComponent::MultiHTTPComponent(
        const char *name,
        const DL_CALLBACKTYPE *callbacks,
        DL_PTR appData,
        DL_COMPONENTTYPE **component):DefaultComponent(name,callbacks,appData,component){
    ENTER_FUNC;
    LLOGV("MultiHTTPComponent constructor !");
    mCurlHandle = NULL;
    mCurlHandle = DL_CreateMultiHandle();

    mLastestHeadBuffer = new DL_BUFFERHEADERTYPE;
    mLastestHeadBuffer->pPreNode = NULL;
    mLastestHeadBuffer->pNextNode= NULL;
    mLastestDataBuffer = new DL_BUFFERHEADERTYPE;
    mLastestDataBuffer->pPreNode = NULL;
    mLastestDataBuffer->pNextNode= NULL;

    mFilledDataBytes = 0;
    mPostDataBytes   = 0;

    mTaskNumMax = 0;


    mFreeFlag = DL_FALSE;
    mCurrentTaskIndex = -1;
    mState = DL_StateInit;
    mTargetState = DL_StateInit;
    ESC_FUNC;
}

MultiHTTPComponent::~MultiHTTPComponent() {
    ENTER_FUNC;
    LLOGV("MultiHTTPComponent deconstructor !");

    if(mCurlHandle != NULL) {
        DL_CloseMultiHandle(mCurlHandle);
        mCurlHandle = NULL;
    }
    if(mLastestHeadBuffer != NULL) {
        mLastestHeadBuffer->pPreNode = NULL;
        mLastestHeadBuffer->pNextNode= NULL;
        delete mLastestHeadBuffer;
    }

    if(mLastestDataBuffer != NULL) {
        mLastestDataBuffer->pPreNode = NULL;
        mLastestDataBuffer->pNextNode= NULL;
        delete mLastestDataBuffer;
    }

    mTaskNumMax = 0;
    ESC_FUNC;
}

void MultiHTTPComponent::prepareForDestruction() {
    Mutex::Autolock autoLock(mLock);
    if(mCurlHandle != NULL) {
        DL_CloseMultiHandle(mCurlHandle);
        mCurlHandle = NULL;
    }

    //memory free

    {
        Mutex::Autolock autolock(mFreeDataLock);
        mFreeFlag = DL_TRUE;

        DL_BUFFERHEADERTYPE* headNode = NULL;
        for(headNode = (DL_BUFFERHEADERTYPE*)(mLastestHeadBuffer->pPreNode);headNode != NULL;) {
            DL_BUFFERHEADERTYPE* preHeadNode =(DL_BUFFERHEADERTYPE*)(headNode->pPreNode);
            LLOGV("free headNode->pBuffer = %p",headNode->pBuffer);
            LLOGV("free headNode = %p nFlags = %lu ",headNode,headNode->nFlags);
            LLOGV("free headNode->pPreNode= %p",preHeadNode);
            mLastestHeadBuffer->pPreNode = NULL;
            mLastestHeadBuffer->pNextNode= NULL;
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
        mLastestHeadBuffer->pPreNode = NULL;
        delete (mLastestHeadBuffer);
        mLastestHeadBuffer = NULL;

        DL_BUFFERHEADERTYPE* bufNode = NULL;

        for(bufNode = (DL_BUFFERHEADERTYPE*)(mLastestDataBuffer->pPreNode);bufNode != NULL;) {
            DL_BUFFERHEADERTYPE* preBufNode =(DL_BUFFERHEADERTYPE*)(bufNode->pPreNode);
            LLOGV("free bufNode->pBuffer = %p",bufNode->pBuffer);
            LLOGV("free bufNode = %p nFlags = %lu ",bufNode,bufNode->nFlags);
            LLOGV("free bufNode->pPreNode= %p",preBufNode);
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
        mLastestDataBuffer->pPreNode = NULL;
        mLastestDataBuffer->pNextNode= NULL;
        delete (mLastestDataBuffer);
        mLastestDataBuffer = NULL;

        releaseTaskBuffers();
        mCurrentTaskIndex = -1;


        mState = DL_StateDeinit;
        mTargetState = DL_StateDeinit;
    }

}

DL_ERRORTYPE MultiHTTPComponent::initCheck() const {
    if(mState == DL_StateInit && mCurlHandle == NULL) {
        return DL_ErrorInvalidState;
    }
    return DL_ErrorNone;
}
DL_ERRORTYPE MultiHTTPComponent::sendCommand(
        DL_COMMANDTYPE  cmd , DL_U32  param , DL_PTR  data ) {
    //changeState
    (void)data;
    Mutex::Autolock autoLock(mLock);
    onSendCommand(cmd,(DL_STATETYPE)param);
    return DL_ErrorNone;
}
DL_ERRORTYPE MultiHTTPComponent::getConfig( DL_INDEXTYPE  index , DL_PTR  params ) {
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

DL_ERRORTYPE MultiHTTPComponent::setConfig(
        DL_INDEXTYPE /* index */, const DL_PTR /* params */) {
    //set mCurlHandle config by index;
    Mutex::Autolock autoLock(mLock);
    return DL_ErrorNone;
}

DL_ERRORTYPE MultiHTTPComponent::getDownloadInfo_l(DL_PTR params) {
    ENTER_FUNC;
    HTTPDownloadStatusInfo* info = (HTTPDownloadStatusInfo*)params;

    int ret  = DL_GetMultiDownloadInfo(mCurlHandle,info);
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

DL_ERRORTYPE MultiHTTPComponent::getDownloadSpeed_l(DL_PTR params) {
    ENTER_FUNC;
    DL_S64* speed = (DL_S64*)params;

    HTTPDownloadStatusInfo downloaderInfo;
    int ret  = DL_GetMultiDownloadInfo(mCurlHandle,&downloaderInfo);

    if(ret == DL_INVAL_HANDLE) {
        ESC_FUNC;
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        ESC_FUNC;
        return DL_ErrorBadParameter;
    }else if(ret == DL_OK){
        *speed = static_cast<DL_S64>(downloaderInfo.download_speed);
        LLOGV("download speed = %lld",*speed);
        ESC_FUNC;
        return DL_ErrorNone;
    }else {
        ESC_FUNC;
        return DL_ErrorUnKnown;
    }

}

DL_ERRORTYPE MultiHTTPComponent::getState(DL_STATETYPE * state ) {
    //get MultiHTTPComponent current state
    Mutex::Autolock autolock(mLock);
    *state = mState;
    return DL_ErrorNone;
}


DL_ERRORTYPE MultiHTTPComponent::prepareLoading( DataSpec*  dataSpec) {
    ENTER_FUNC;

    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateInit && mTargetState == mState);
    CHECK(dataSpec != NULL);
    std::vector<std::string> headerProps;
    std::string userAgent;
    if(dataSpec->headUserAgent != NULL) {
        userAgent = dataSpec->headUserAgent;
    }else {
        userAgent = "User-Agent: Android6.0";
    }
    headerProps.push_back(userAgent);

    http_thread *multiCurlWrapper = (http_thread*)mCurlHandle;
    multiCurlWrapper->dataSpec = dataSpec;//used by addTaskBufferInfo()
    mTaskNumMax = dataSpec->multiHttpTastNum;
    DL_S32 sessionTimout = dataSpec->connectTimeoutMs;
    int ret = DL_PrepareMultiDownload(mCurlHandle,  dataSpec->url, dataSpec->position, dataSpec->length,false ,onEventCB , this, headerProps,sessionTimout);

    CHECK(mCurrentTaskIndex == -1);
    mCurrentTaskIndex = 0 ;
    onChangeState(DL_StateIdle);
    ESC_FUNC;
    if(ret == DL_INVAL_HANDLE) {
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        return DL_ErrorBadParameter;
    }else if (ret == DL_OUT_OF_MEMORY) {
        return DL_ErrorInsufficientResources;
    }else if(ret == DL_ALREADY_PREPARED) {
        LLOGW("my curlDownloader running already");
        return DL_ErrorNone;
    }else if(ret == DL_OK){
        return DL_ErrorNone;
    }else {
        return DL_ErrorUnKnown;
    }
}

DL_ERRORTYPE MultiHTTPComponent::startLoading(
        DataSpec* /* dataSpec*/) {
    ENTER_FUNC;
    //check state
    //call DLMETHOD
    //int DL_StartDownload(DLHandle handle,bool isAsync);
    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateIdle&& mTargetState == mState);
    int ret =  DL_StartMultiDownload(mCurlHandle,true);


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

DL_ERRORTYPE MultiHTTPComponent::stopLoading() {
    ENTER_FUNC;
    //check state
    //call DLMETHOD
    //int DL_StopDownload(DLHandle handle, int64_t nanoseconds=0);
    int ret = DL_OK;
    {
        Mutex::Autolock autolock(mLock);

        CHECK(mState == DL_StateExecuting && mTargetState == mState);
        if(mState != DL_StateExecuting ) {

            LLOGD("mState = %d  curlDownloader stoping already",mState);
            return DL_ErrorNone;
        }
        ret =  DL_StopMutiDownload(mCurlHandle,0ll);

        onChangeState(DL_StateIdle);

    }
    ESC_FUNC;
    if(ret == DL_OK) {
        //WARNING ME  must not be locked, mutex with ll_load_manager !!!
        notify(DL_EventCmdComplete, DL_ET_STOP_CMPL, 0/*unused*/, NULL);
        return DL_ErrorNone;
    }else if(ret == DL_INVAL_HANDLE) {
        return DL_ErrorInsufficientResources;
    }else if (ret == DL_INVAL_PARAM) {
        return DL_ErrorBadParameter;
    }else if(ret >= DL_EHTTP_OVER_REDIRECT && ret < DL_EHTTP_MAX){
        return DL_ErrorProtocol;
    }else if(ret == DL_TIMEOUT) {
        LLOGE("stop_loading DL_TIMEOUT");
        if(mState == DL_StateIdle) {
            notify(DL_EventCmdComplete, DL_ET_STOP_CMPL, 0/*unused*/, NULL);
        }
        return DL_ErrorTimeout;
    }else {
        return DL_ErrorUnKnown;
    }


}

DL_ERRORTYPE MultiHTTPComponent::pauseLoading() {
    ENTER_FUNC;
    //check state
    //call DLMETHOD
    //int DL_PauseDownload(DLHandle handle);
    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateExecuting && mTargetState == mState);
    int ret =  DL_PauseMultiDownload(mCurlHandle);

    ESC_FUNC;

    if(ret == DL_OK) {
        return DL_ErrorNone;
    }else {
        return DL_ErrorUnKnown;
    }


}


DL_ERRORTYPE MultiHTTPComponent::resumeLoading() {
    ENTER_FUNC;
    //check state
    //call DLMETHOD
    //int DL_ResumeDownload(DLHandle handle);
    Mutex::Autolock autolock(mLock);
    CHECK(mState == DL_StateExecuting && mTargetState == mState);
    int ret =  DL_ResumeMultiDownload(mCurlHandle);

    ESC_FUNC;

    if(ret == DL_OK) {
        return DL_ErrorNone;
    }else {
        return DL_ErrorUnKnown;
    }
}

///////////////////////////////////////////////
DL_ERRORTYPE MultiHTTPComponent::addTaskBufferInfo(DL_S64 position,DL_S32 length, DL_S32 index){
    //used by add a new httpTask
    BufferInfo* pTaskBuffer = (BufferInfo*)malloc(sizeof(BufferInfo));
    if(pTaskBuffer == NULL) {
        LLOGE("not enough memory,myebe out of memory!!");
        return DL_ErrorInsufficientResources;
    }
    pTaskBuffer->absOffset = position;
    pTaskBuffer->size = length;
    pTaskBuffer->readOffset = 0;
    pTaskBuffer->writeOffset = 0;
    pTaskBuffer->data = NULL;

    pTaskBuffer->data = (DL_BYTE)malloc(pTaskBuffer->size);
    if(pTaskBuffer->data == NULL) {
        LLOGE("not enough memory,myebe out of memory!!");
        return DL_ErrorInsufficientResources;
    }

    memset(pTaskBuffer->data,0,pTaskBuffer->size);
    Mutex::Autolock autolock(mTaskBufferLock);
    if( mTaskBuffers.insert(std::pair<DL_S32,BufferInfo*>(index,pTaskBuffer)).second == false ) {
        LLOGE("Falied to insert taskBuffer to the map!!");
        return DL_ErrorUnKnown;
    }

    LLOGV("addTaskBufferInfo Asbposition %lld length %ld",position,length);
    return DL_ErrorNone;
}

DL_ERRORTYPE MultiHTTPComponent::delTaskBufferInfo(DL_S32 index){
    Mutex::Autolock autolock(mTaskBufferLock);
    BufferInfo* taskBuffer = mTaskBuffers[index];
    mTaskBuffers.erase(index);
    free(taskBuffer->data);
    free(taskBuffer);
    return DL_ErrorNone;
}
//get the filled buffer
DL_BUFFERHEADERTYPE* MultiHTTPComponent::getAlreadyBuffer(DL_S32 index) {
    Mutex::Autolock autolock(mTaskBufferLock);
    BufferInfo* taskBuffer = mTaskBuffers[index];
    if(taskBuffer == NULL) {
        LLOGE("can't found mTaskBuffers[%ld]",index);
        return NULL;
    }
    DL_S32 filledSize = getFilledSize(taskBuffer);
    if(filledSize <= 0 ) {
        LLOGE("not enough data ,filledSize = %ld",filledSize);
        return NULL;
    }
    DL_BUFFERHEADERTYPE *header = NULL;
    if(filledSize == taskBuffer->size) {
        LLOGD("me %p index = %ld the whole buffer filled",this,index);
        header = new DL_BUFFERHEADERTYPE;
        (header)->nSize = sizeof(DL_BUFFERHEADERTYPE);
        (header)->nVersion.s.nVersionMajor = 1;
        (header)->nVersion.s.nVersionMinor = 0;
        (header)->nVersion.s.nRevision = 0;
        (header)->nVersion.s.nStep = 0;
        (header)->pBuffer = (DL_BYTE)(taskBuffer->data);
        (header)->nAllocLen = taskBuffer->size;
        (header)->nFilledLen = filledSize;
        (header)->nOffset = taskBuffer->readOffset;
        (header)->pAppPrivate = NULL;
        (header)->pDownloaderPrivate= this;
        (header)->nFlags = (DL_U32)1u;
        (header)->pPreNode = (DL_PTR)NULL;
        (header)->pNextNode= (DL_PTR)NULL;

        taskBuffer->readOffset = taskBuffer->writeOffset;
        if(isReadCompleted(taskBuffer)) {
            mTaskBuffers.erase(index);
            free(taskBuffer);
            LLOGD("me %p mCurrentTaskIndex = %ld the whole buffer filled,need free",this,index);
            mCurrentTaskIndex++;
        }




    }else {
        LLOGD("me %p index = %ld filled %ld  data readOffset = %ld writeOffset = %ld"
                , this,index,filledSize,taskBuffer->readOffset,taskBuffer->writeOffset);
        DL_BYTE mem = (DL_BYTE)malloc(filledSize);
        if(mem == NULL) {
            LLOGE("not enough memory (realloc returned NULL)\n");
            return NULL;
        }
        memset(mem,0,filledSize);

        memcpy(mem,taskBuffer->data+taskBuffer->readOffset,filledSize);
        header = new DL_BUFFERHEADERTYPE;
        (header)->nSize = sizeof(DL_BUFFERHEADERTYPE);
        (header)->nVersion.s.nVersionMajor = 1;
        (header)->nVersion.s.nVersionMinor = 0;
        (header)->nVersion.s.nRevision = 0;
        (header)->nVersion.s.nStep = 0;
        (header)->pBuffer = (DL_BYTE)(mem);
        (header)->nAllocLen = filledSize;
        (header)->nFilledLen = filledSize;
        (header)->nOffset = 0;
        (header)->pAppPrivate = NULL;
        (header)->pDownloaderPrivate= this;
        (header)->nFlags = (DL_U32)1u;
        (header)->pPreNode = (DL_PTR)NULL;
        (header)->pNextNode= (DL_PTR)NULL;

        taskBuffer->readOffset = taskBuffer->writeOffset;
        if(isReadCompleted(taskBuffer)) {
            mTaskBuffers.erase(index);
            free(taskBuffer->data);
            free(taskBuffer);
            LLOGD("me %p mCurrentTaskIndex = %ld the whole buffer filled,need free",this,index);
            mCurrentTaskIndex++;
        }


    }


    return header;
}

DL_ERRORTYPE MultiHTTPComponent::fillThisTaskBuffer(DL_S32 taskIndex,DL_BYTE pData,DL_S32 size){


    Mutex::Autolock autoLock(mTaskBufferLock);
    BufferInfo* taskBuffer = mTaskBuffers[taskIndex];
    CHECK(taskBuffer != NULL);
    memcpy((taskBuffer->data+taskBuffer->writeOffset),pData, size);
    taskBuffer->writeOffset = taskBuffer->writeOffset+size;
    return DL_ErrorNone;
}

void MultiHTTPComponent::releaseTaskBuffers(){

    Mutex::Autolock autoLock(mTaskBufferLock);
    tTaskBuffer::iterator it;
    if(mTaskBuffers.empty()) {
        LLOGW("mTaskBuffers is empty");
        return;
    }
    /*it  = mTaskBuffers.begin();
      BufferInfo* taskBuffer = it->second;
      if(taskBuffer->readOffset == 0) {
//DL_BUFFERHEADERTYPE* unused this Buffer
free(taskBuffer->data);

}
mTaskBuffers.erase(it);
free(taskBuffer);*/
for(it  = mTaskBuffers.begin();it != mTaskBuffers.end();) {
    BufferInfo* taskBuffer = it->second;
    mTaskBuffers.erase(it++);
    free(taskBuffer->data);
    free(taskBuffer);


}

return ;
}

/////////////////////////////////////

#define DLMultiCurl http_thread
#define DLTask http_task
/*static*/
void MultiHTTPComponent::onEventCB(void* handle/*DLMultiCurl*/, void* pContext/*pAPP*/, int type, void* data, int size)
{
    //TODO
    //DL_EVENTTYPE httpEvent;
    DL_EventType httpEvent;
    MultiHTTPComponent* me = NULL;
    DLTask* dlTask= NULL;
    DLMultiCurl* dlMulticurl = NULL;
    DL_PTR pData = NULL;
    DL_S32 taskIndex = -1;


    {
        //Mutex::Autolock autolock(mOnEventLock);
        httpEvent = (DL_EventType)type;
        me = (MultiHTTPComponent*)pContext;
        dlTask = (DLTask*) handle;
        if (dlTask == NULL) {
            LLOGE("dlTask is NULL)\n");
            return ;
        }
        dlMulticurl = (DLMultiCurl*) dlTask->thread;

        if (dlMulticurl == NULL) {
            LLOGE("dlMulticurl is NULL)\n");
            return ;
        }

        taskIndex = dlTask->i_index;
        pData = data;

    }




    switch (httpEvent) {
        case DL_ET_ADDTASK_CMPL:
            {// add http task complete; already obtain lock
                LLOGV("DL_ET_ADDTASK_COMPLETE");
                Mutex::Autolock autolock(me->mFreeDataLock);
                if(me->mFreeFlag == DL_TRUE) {
                    break;
                }

                DL_S64 position = *((DL_S64*)pData);
                me->addTaskBufferInfo(position,size,dlTask->i_index);
                break;

            }

        case DL_ET_DATA:
            {//DL_HTTP_FTBD_CB

                LLOGV("DL_HTTP_FTBD_CB me = %p taskIndex = %ld  me->mCurrentTaskIndex = %ld pData = %p size = %d",me,taskIndex,me->mCurrentTaskIndex,pData,size);
                Mutex::Autolock autolock(me->mFreeDataLock);
                if(me->mFreeFlag == DL_TRUE) {
                    break;
                }
                me->mFilledDataBytes += size;
                me->fillThisTaskBuffer(taskIndex,(DL_BYTE)pData,size);
                DL_BUFFERHEADERTYPE *header = me->getAlreadyBuffer(me->mCurrentTaskIndex);
                if(header != NULL) {
                    DL_BUFFERHEADERTYPE* lastestPreNode = (DL_BUFFERHEADERTYPE*)(me->mLastestDataBuffer->pPreNode);
                    if(lastestPreNode != NULL) {
                        header->pPreNode = lastestPreNode;
                        lastestPreNode->pNextNode = header;
                    }

                    me->mLastestDataBuffer->pPreNode= header;
                    header->pNextNode= me->mLastestDataBuffer;

                    me->mPostDataBytes += header->nFilledLen;
                    me->notifyFillBufferDone(header);
                }


                break;

            }
        case DL_ET_HEADER://DL_HTTP_HEADER_CB
            {
                Mutex::Autolock autolock(me->mFreeDataLock);
                if(me->mFreeFlag == DL_TRUE) {
                    break;
                }

                LLOGV("DL_HTTP_HEADER_CB pData = %s",(DL_STRING)pData);
                if(dlTask->i_index != 0) {
                    return;
                }
                CHECK(strlen((DL_STRING)pData) == size);
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
                (header)->nFlags = (DL_U32)1u;
                (header)->pPreNode = NULL;
                (header)->pNextNode= NULL;
                LLOGV("Header %p mem %p",header,tmpBuffer);
                LLOGV("header pPreNode %p ",me->mLastestHeadBuffer);

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
                //onProcessHTTPPROGRESS unused
                me->notify(DL_EventCmdComplete,DL_ET_PROGRESS,0u,NULL);
                break;
            }
        case DL_ET_CURL_DEBUG://DL_HTTP_DEBUG_CB
            {
                //onProccess: parse error,info
                break;
            }
        case DL_ET_DATA_CMPL://http_thread notify
            {

                LLOGV("DL_ET_DATA_COMPLETE IN me = %p taskIndex = %ld  me->mCurrentTaskIndex = %ld  me->mFilledDataBytes = %lld me->mPostDataBytes = %lld ",me,taskIndex,me->mCurrentTaskIndex,me->mFilledDataBytes,me->mPostDataBytes);
                Mutex::Autolock autolock(me->mFreeDataLock);
                if(me->mFreeFlag == DL_TRUE) {
                    break;
                }
                int taskNum = TASKCOUNTSMAX;
                if(me->mTaskNumMax > 0) {
                    taskNum = me->mTaskNumMax;
                }
                while (me->mCurrentTaskIndex < taskNum) {
                    DL_BUFFERHEADERTYPE *header = me->getAlreadyBuffer(me->mCurrentTaskIndex);
                    if(header != NULL) {
                        DL_BUFFERHEADERTYPE* lastestPreNode = (DL_BUFFERHEADERTYPE*)(me->mLastestDataBuffer->pPreNode);
                        if(lastestPreNode != NULL) {
                            header->pPreNode = lastestPreNode;
                            lastestPreNode->pNextNode = header;
                        }

                        me->mLastestDataBuffer->pPreNode= header;
                        header->pNextNode= me->mLastestDataBuffer;

                        me->mPostDataBytes += header->nFilledLen;
                        me->notifyFillBufferDone(header);
                    }else {
                        LLOGW("maybe http_task[%ld] was timeout",me->mCurrentTaskIndex);
                        break;
                    }
                }
                LLOGV("DL_ET_DATA_COMPLETE OUT me = %p taskIndex = %ld  me->mCurrentTaskIndex = %ld  me->mFilledDataBytes = %lld me->mPostDataBytes = %lld ",me,taskIndex,me->mCurrentTaskIndex,me->mFilledDataBytes,me->mPostDataBytes);
                me->notify(DL_EventCmdComplete,DL_ET_DATA_CMPL,0u,NULL);
                break;
            }
        case DL_ET_SUBTASK_DATA_CMPL://http_task notify data cmpl
            {

                LLOGV("DL_ET_SUBTASK_DATA_CMPL me = %p taskIndex = %ld ",me,taskIndex);
                break;
            }
        case DL_ET_SUBTASK_ERR://http_task notify data ERR
            {

                int responseCode = *((int*)pData);
                LLOGV("DL_ET_SUBTASK_ERR IN me = %p taskIndex = %ld responseCode = %d me->mFilledDataBytes = %lld me->mPostDataBytes = %lld ",me,taskIndex,responseCode,me->mFilledDataBytes,me->mPostDataBytes);
                if(responseCode != DL_OK){
                    Mutex::Autolock autolock(me->mFreeDataLock);
                    if(me->mFreeFlag == DL_TRUE) {
                        break;
                    }
                    int taskNum = TASKCOUNTSMAX;
                    if(me->mTaskNumMax > 0) {
                        taskNum = me->mTaskNumMax;
                    }

                    while (me->mCurrentTaskIndex < taskNum) {
                        DL_BUFFERHEADERTYPE *header = me->getAlreadyBuffer(me->mCurrentTaskIndex);
                        if(header != NULL) {
                            DL_BUFFERHEADERTYPE* lastestPreNode = (DL_BUFFERHEADERTYPE*)(me->mLastestDataBuffer->pPreNode);
                            if(lastestPreNode != NULL) {
                                header->pPreNode = lastestPreNode;
                                lastestPreNode->pNextNode = header;
                            }

                            me->mLastestDataBuffer->pPreNode= header;
                            header->pNextNode= me->mLastestDataBuffer;

                            me->mPostDataBytes += header->nFilledLen;
                            me->notifyFillBufferDone(header);
                        }else {
                            LLOGW("maybe http_task[%ld] was timeout",me->mCurrentTaskIndex);
                            break;
                        }
                    }

                    //TODO warning: notify error per subtask
                    LLOGV("DL_ET_SUBTASK_ERR OUT me = %p taskIndex = %ld responseCode = %d me->mFilledDataBytes = %lld me->mPostDataBytes = %lld ",me,taskIndex,responseCode,me->mFilledDataBytes,me->mPostDataBytes);
                    me->notify(DL_EventError,DL_ET_ERR,responseCode,NULL);
                }else {
                    //UNUSED
                }

                break;
            }

        case DL_ET_ERR://DL_HTTP_RESPONSE_ERR_CB
            {
                //parse reponsecode
                //post msg
                int errCode = *(int*)pData;
                LLOGE("DL_HTTP_RESPONSE_ERR_CB errCode = %d",errCode);

                if(errCode != DL_OK){
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
}

void MultiHTTPComponent::onReset() {
    ENTER_FUNC;
    DLMultiCurl* dlCurl = (DLMultiCurl*)mCurlHandle;
    if(dlCurl != NULL) {
        //TODO
        DL_ResetMultiHandle(dlCurl);
    }

    ESC_FUNC;
    return;
}

void MultiHTTPComponent::onSendCommand( DL_COMMANDTYPE cmd, DL_U32 param) {
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

void MultiHTTPComponent::onChangeState(DL_STATETYPE state) {
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
                LLOGE("State is incorrect  :%d \n",state);
                //notify(DL_EventError, DL_CommandStateSet, state, NULL);
                CHECK_EQ(1,0);//crash

            }
            break;
        case DL_StateExecuting:
            {
                CHECK_EQ((int)state , (int)DL_StateIdle);

                //TODO stop or pause mCurlHandle
                DLMultiCurl* dlCurl = (DLMultiCurl*)mCurlHandle;
                if(dlCurl != NULL && dlCurl->mState == DL_STATE_BUSY) {
                    //TODO
                    DL_StopMutiDownload(mCurlHandle,0ll);

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

void MultiHTTPComponent::checkTransitions() {
    if (mState != mTargetState) {
        bool transitionComplete = true;

        if (mState == DL_StateInit) {
            CHECK_EQ((int)mTargetState, (int)DL_StateIdle);
            //TODO check mCurlHandle prepare
            DLMultiCurl* dlCurl = (DLMultiCurl*)mCurlHandle;
            if(dlCurl != NULL && dlCurl->mState == DL_STATE_BUSY) {
                //TODO
                transitionComplete = false;
            }

        } else if (mTargetState == DL_StateInit) {
            CHECK_EQ((int)mState, (int)DL_StateIdle);
            //TODO check mCurlHandle isClear,clear some memory,reset state
            DLMultiCurl* dlCurl = (DLMultiCurl*)mCurlHandle;
            if(dlCurl != NULL && (dlCurl->mState == DL_STATE_BUSY)) {

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
        }
    }else {
        LLOGV("already mstate == mtargetState = %d !!! line = %d ",mState,__LINE__);
    }

}

LIYL_NAMESPACE_END

liyl::DefaultComponent* createDLMultiHTTPComponent(
        const char *name, const DL_CALLBACKTYPE *callbacks, DL_PTR appData,
        DL_COMPONENTTYPE **component) {
    return new liyl::MultiHTTPComponent(name, callbacks, appData, component);
}


