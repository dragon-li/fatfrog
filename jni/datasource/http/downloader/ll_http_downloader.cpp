/*************************************************************************
  > File Name: ll_http_downloader.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 年月31日 星期三 15时31分28秒
 ************************************************************************/
#define LOG_NDEBUG 0

#include "ll_http_downloader.h"

#include "ll_http_multi_component.h"
#include "ll_http_component.h"

#include "dl_utils.h"
#include <unistd.h>
#include <ctype.h>

#undef LOG_TAG
#define LOG_TAG "HTTPDownloader"

#undef ENTER_FUNC
#undef ESC_FUNC 
#define ENTER_FUNC do{ LLOGV("[%d]liyl add enter %s  %d",mNodeID,__func__,__LINE__);}while(0)
#define ESC_FUNC do{ LLOGV("[%d]liyl add esc %s  %d",mNodeID,__func__,__LINE__);}while(0)




LIYL_NAMESPACE_START


status_t HTTPDownloader::initSetup(void *owner, const sp<DLObserver> &observer, const char *name) {
    LLOGV("liyl add enter %s  %d",__func__,__LINE__);
    Mutex::Autolock autoLock(mLock);
    HTTPDownloader::mOwner=(LL_LoadManager*)owner;
    HTTPDownloader::mNodeID=(int32_t)NULL;
    HTTPDownloader::mObserver=observer;
    HTTPDownloader::mDying=DL_FALSE;
    DL_COMPONENTTYPE* component = NULL; 
    //mHTTPComponent = new MultiHTTPComponent(name,&HTTPDownloader::kCallbacks,this,&component);
    mHTTPComponent = new HTTPComponent(name,&HTTPDownloader::kCallbacks,this,&component);
    CHECK(component != NULL);
    mHandle = component;
    CHECK(mDataSpec != NULL);


    LLOGD("mHandle = %p",mHandle);
    LLOGV("liyl add esp %s  %d",__func__,__LINE__);
    return OK;
}
HTTPDownloader::~HTTPDownloader() {
    ENTER_FUNC;
    ESC_FUNC;
}

// free node must be in corrected state
status_t HTTPDownloader::freeNode(DownLoaderFactory* mFactory) {
    ENTER_FUNC;
    static int32_t kMaxNumIterations = 10 ;
    if(mDying != DL_TRUE) {
        mDying = DL_TRUE;
    }else {
        LLOGD("node[%d] warning:double free!!",mNodeID);
        return UNKNOWN_ERROR;  
    }

    DL_STATETYPE state;
    CHECK(mHandle != NULL);
    DL_ERRORTYPE errType = DL_GetState(mHandle, &state);
    if( errType !=  DL_ErrorNone) {
        LLOGE("node[%d]DL_GetState failed  err=%x !",mNodeID,errType);
        return StatusFromDLError(errType);
    }
    LLOGD("node[%d]DL_GetState state=0x%x !",mNodeID,state);
    switch (state) {
        case DL_StateExecuting: //HTTP:READ
            {
                LLOGD("node[%d]forcing Executing->Idle",mNodeID);
                status_t ret = sendCommand(DL_CommandStateSet, DL_StateIdle);
                if(ret != OK) {
                    return UNKNOWN_ERROR;
                }
                DL_ERRORTYPE err;
                int32_t iteration = 0;
                while ((err = DL_GetState(mHandle, &state)) == DL_ErrorNone
                        && state != DL_StateIdle
                        && state != DL_StateInvalid) {
                    if (++iteration > kMaxNumIterations) {
                        LLOGE("node[%d]component failed to enter Idle state, aborting.",mNodeID);
                        state = DL_StateInvalid;
                        break;
                    }

                    LLOGD("node[%d]curent state = % d waiting for Idle state...",mNodeID,state);
                    usleep(100000);
                }
                if(err != DL_ErrorNone) {
                    return StatusFromDLError(err);
                }

                if (state == DL_StateInvalid) {
                    LLOGW("node[%d] state == DL_StateInvalid ",mNodeID);
                    break;
                }

            }    
        case DL_StateIdle://HTTP:CONNECT
            {
                LLOGV("node[%d]forcing Idle->Init",mNodeID);
                status_t ret = sendCommand(DL_CommandStateSet, DL_StateInit);
                if(ret != OK) {
                    return UNKNOWN_ERROR;
                }

                DL_ERRORTYPE err;
                int32_t iteration = 0;
                while ((err = DL_GetState(mHandle, &state)) == DL_ErrorNone
                        && state != DL_StateInit
                        && state != DL_StateInvalid) {
                    if (++iteration > kMaxNumIterations) {
                        LLOGE("node[%d]component failed to enter Init state, aborting.",mNodeID);
                        state = DL_StateInvalid;
                        break;
                    }

                    LLOGD("node[%d]curent state = % d waiting for Init state...",mNodeID,state);
                    usleep(100000);
                }
                if(err != DL_ErrorNone) {
                    return StatusFromDLError(err);
                }

            }

        case DL_StateInit://HTTP:INIT CURL
            LLOGD("current DL_StateInit");
            break;
        case DL_StateDeinit://HTTP:Deinit CURL
            LLOGD("current DL_StateDeinit");
            break;
        case DL_StateInvalid:
            LLOGD("current DL_StateInvalid");
            break;

        default:
            CHECK(!"should not be here, unknown state. ");
            break;
    }

    LLOGD("node[%d]calling destroyComponentInstance",mNodeID);
    clear();//TODO destroyComponentInstance;     

    LLOGD("node[%d]calling destroyComponentInstance done",mNodeID);
    mHandle = NULL;


    mOwner->invalidateNodeID(mNodeID);

    LLOGD("node[%d]downloader going away.",mNodeID);
    mNodeID = (int32_t)NULL;

    ESC_FUNC;
    return OK;

}


status_t HTTPDownloader::initCheck(){ENTER_FUNC;


    ESC_FUNC;

    return OK;
}                                                                                         
status_t HTTPDownloader::clear(){ENTER_FUNC;

    Mutex::Autolock autoLock(mLock); //locked for getConfig
    if(mHTTPComponent != NULL) { 

        LLOGD("node[%d] destroy mHTTPComponent start",mNodeID);
        {
            Mutex::Autolock  flagLock(mClearComponentLock);
            ((DefaultComponent*)mHTTPComponent)->prepareForDestruction();
        }
        LLOGD("node[%d] destroy mHTTPComponent end",mNodeID);
        delete (DefaultComponent*)mHTTPComponent; 
    }
    mHTTPComponent = NULL;

    ESC_FUNC;
    return OK;
}                                                                                         

status_t HTTPDownloader::getConfig(DL_INDEXTYPE index, void *params, size_t size){
    ENTER_FUNC;
    Mutex::Autolock autoLock(mLock);
    DL_ERRORTYPE err = DL_ErrorNone;
    if(mHandle != NULL && mHTTPComponent != NULL) {
        err = DL_GetConfig(mHandle, index, params);
    }else {
        err = DL_ErrorInvalidComponent;
    }
    ESC_FUNC;
    return StatusFromDLError(err);
}


status_t HTTPDownloader::setNotifyFun(void* cookie,notify_callback_f notifyFun){ENTER_FUNC; 

    ESC_FUNC;
    return INVALID_OPERATION;
}                                                                                         


status_t HTTPDownloader::onLoadStateChange(){ENTER_FUNC;
    //notify the loading env  stateChange
    ESC_FUNC;return INVALID_OPERATION;
}


DL_BOOL HTTPDownloader::handleMessage(/*const*/ downloader_message &msg){
    ENTER_FUNC;
    DL_BOOL isError = DL_FALSE;
    LLOGV("node[%d]HTTPDownloader, onMessage msg.type = %lu",msg.node,msg.type);
    switch (msg.type) {
        case DL_EventCmdComplete: 
            {
                LLOGD("node[%d]DL_EventCmdComplete",msg.node);
                break;
            }    
        case DL_EventError:
            {
                LLOGD("node[%d]DL_EventError",msg.node);
                isError = DL_TRUE;

                break;
            }

        case DL_EventSettingChange:
            LLOGD("node[%d]DL_EventSettingChange",msg.node);
            break;
        case DL_EventSendCmd:
            LLOGD("node[%d]DL_EventSendCmd",msg.node);
            break;
        case DL_EventVendorStartUnused:
            LLOGD("node[%d]DL_EventVendorStartUnused",msg.node);
            break;
        default:
            CHECK(!"should not be here, unknown state. ");
            break;
    }



    DL_U32 msg_type = msg.data1;
    if (msg_type == (DL_U32)DL_ET_DATA) {
        //copy buffer for DL_BUFFERHEADERTYPE*
        LLOGD("node[%d]onMessage msg DL_HTTP_FTBD_CB",msg.node );
        DL_BUFFERHEADERTYPE* tmpBuf = (DL_BUFFERHEADERTYPE*)(msg.pEventData);
        {
            Mutex::Autolock  flagLock(mClearComponentLock);
            if(mDying == DL_TRUE) {
                ESC_FUNC;
                return DL_TRUE;
            }
            if(mHTTPComponent != NULL) {
                ((DefaultComponent*)mHTTPComponent)->setConsumedBytes(tmpBuf->nFilledLen);
            }
            ((DL_BUFFERHEADERTYPE*)(tmpBuf->pNextNode))->pPreNode = NULL; // erase pPreNode
        }
        LLOGV("node[%d]Buffer %p nOffset %lu",msg.node,tmpBuf,tmpBuf->nOffset);
        LLOGV("node[%d]Buffer %p nAllocLen %lu",msg.node,tmpBuf,tmpBuf->nAllocLen);
        LLOGV("node[%d]Buffer->nFilledLen %lu",msg.node,tmpBuf->nFilledLen);
        LLOGV("node[%d]Buffer->pBuffer= %p",msg.node,tmpBuf->pBuffer);
        ESC_FUNC;return DL_FALSE;
    }else if(msg_type == (DL_U32)DL_ET_HEADER){
        //CB process
        LLOGD("node[%d]onMessage msg DL_HTTP_HEADER_CB",msg.node);
        DL_BUFFERHEADERTYPE* tmpBuf = NULL;
        {
            Mutex::Autolock  flagLock(mClearComponentLock);
            if(mDying == DL_TRUE) {
                ESC_FUNC;
                return DL_TRUE;
            }
            tmpBuf = (DL_BUFFERHEADERTYPE*)(msg.pEventData);
            LLOGD("node[%d]Header %p nSize %lu nFilledLen %lu pBuffer +++++++++ %s",msg.node,tmpBuf,tmpBuf->nSize,tmpBuf->nFilledLen,(char*)(tmpBuf->pBuffer));

            ((DL_BUFFERHEADERTYPE*)(tmpBuf->pNextNode))->pPreNode = NULL; // erase pPreNode

        }
        ESC_FUNC;return DL_FALSE;
    }else if(msg_type == (DL_U32)DL_ET_PROGRESS){
        //CB process
        LLOGD("node[%d]onMessage msg DL_HTTP_PROGRESS_CB",msg.node);

        ESC_FUNC;return DL_TRUE;
    }else if(msg_type == (DL_U32)DL_ET_CURL_DEBUG){
        //CB process
        LLOGD("node[%d]onMessage msg DL_HTTP_DEBUG_CB",msg.node);

        ESC_FUNC;return DL_TRUE;
    }else if(msg_type == (DL_U32)DL_ET_DATA_CMPL){
        //CB process
        LLOGD("node[%d]onMessage msg DL_HTTP_RESPONSE_CMPL_CB",msg.node);

        ESC_FUNC;return DL_FALSE;
    }else if(msg_type == (DL_U32)DL_ET_ERR){
        //CB process
        LLOGD("node[%d]onMessage msg DL_HTTP_RESPONSE_ERR_CB",msg.node);

        ESC_FUNC;return DL_FALSE;
    }else if(msg_type == (DL_U32)DL_ET_STOP_CMPL){

        LLOGD("node[%d]onMessage msg DL_HTTP_STOP_CMPL_CB",msg.node);
        ESC_FUNC;return DL_FALSE;
    }else {
    }


    ESC_FUNC;return DL_FALSE;
}


void HTTPDownloader::onProcessSessionStatusCB(DL_U32 status,DataSpec *pDataSpec) {

}

void HTTPDownloader::onProcessHeaderCB(DL_STRING pData,DataSpec *pDataSpec) {
}


LIYL_NAMESPACE_END
