/*************************************************************************
  > File Name: ll_downloader.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 7年05月26日 星期五 14时59分54秒
 ************************************************************************/

#include<iostream>

#include "ll_downloader.h"
#include "dl_utils.h"
#undef LOG_TAG
#define LOG_TAG "IDownloader"

#undef ENTER_FUNC
#undef ESC_FUNC 
#define ENTER_FUNC do{ LLOGV("[%d]liyl add enter %s  %d",mNodeID,__func__,__LINE__);}while(0)
#define ESC_FUNC do{ LLOGV("[%d]liyl add esc %s  %d",mNodeID,__func__,__LINE__);}while(0)




LIYL_NAMESPACE_START

/*static*/
DL_CALLBACKTYPE IDownloader::kCallbacks = {
	&OnEvent, &OnFillThisBufferDone
};

IDownloader::IDownloader(){ENTER_FUNC;
    LLOGV("liyl add enter %s  %d",__func__,__LINE__);
    mDataSpec = NULL;
    status_t ret = createDataSpec(&mDataSpec); 
    if(ret != OK) {
       LLOGE("createDataSpec failed");
    }

    LLOGV("liyl add esp %s  %d",__func__,__LINE__);
}

IDownloader::~IDownloader() {
    LLOGV("liyl add enter %s  %d",__func__,__LINE__);
    if(mDataSpec != NULL) {
		
       releaseDataSpec(mDataSpec);
       mDataSpec = NULL;
    }
    LLOGV("liyl add esp %s  %d",__func__,__LINE__);
}

void IDownloader::setHandle(node_id node_id, DL_HANDLETYPE handle) {
	mNodeID = node_id;
	//TODO
	//mHandle = handle;
}

void IDownloader::setDataSpec(DataSpec* dataSpec) {
    if(dataSpec == NULL) {
        LLOGE("dataSpec is NULL!!");
        return;
    }
    
    if(dataSpec->url != NULL) {
        mDataSpecUrl = dataSpec->url;
        mDataSpec->url = mDataSpecUrl.c_str();
    }else {
        LLOGE("url is NULL");
    }
    mDataSpec->absoluteStreamPosition = dataSpec->absoluteStreamPosition;
    mDataSpec->position            = dataSpec->position;
    mDataSpec->length              = dataSpec->length;
    if(dataSpec->key != NULL) {
		mDataSpecKey = dataSpec->key;
        mDataSpec->key             = mDataSpecKey.c_str();
    }else {
        LLOGE("key is NULL");
    }
    mDataSpec->flags               = dataSpec->flags;
    if(dataSpec->headUserAgent != NULL) {
        mDataSpecUserAgent = dataSpec->headUserAgent;
        mDataSpec->headUserAgent = mDataSpecUserAgent.c_str();
    }else {
        LLOGE("userAgent is NULL");
    }
    mDataSpec->downloadStatus = dataSpec->downloadStatus;
    mDataSpec->connectTimeoutMs = dataSpec->connectTimeoutMs;
    mDataSpec->readTimeoutMs = dataSpec->readTimeoutMs;
    mDataSpec->multiHttpTastNum    = dataSpec->multiHttpTastNum; //the task number per Downloader

}

LL_LoadManager *IDownloader::owner() {
	return mOwner;
}

sp<DLObserver> IDownloader::observer() {
	return mObserver;
}

node_id IDownloader::nodeID() {
	return mNodeID;
}


status_t IDownloader::freeNode(DownLoaderFactory * mFactory) {
	return OK; 
}

status_t IDownloader::sendCommand(
		DL_COMMANDTYPE cmd, DL_S32 param) {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
    DL_ERRORTYPE err;
	if(mHandle != NULL) {
	    err = DL_SendCommand(mHandle, cmd, param, NULL);
	}else {
		LLOGW("mHandle is NULL");
		err = DL_ErrorInvalidComponent;
	}
	ESC_FUNC;
	return StatusFromDLError(err);
}

/*static*/
status_t IDownloader::StatusFromDLError(DL_ERRORTYPE err) {
	switch (err) {
		case DL_ErrorNone:
			return OK;
		case DL_ErrorUnsupportedSetting:
			return INVALID_OPERATION;
		default:
			return UNKNOWN_ERROR;
	}
}

status_t IDownloader::prepareLoading(DataSpec* dataSpec) {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	//TODO  
	if(dataSpec == NULL) {
	    LLOGW("dataSpec is NULL");
		dataSpec = mDataSpec;
	}
    DL_ERRORTYPE err;
	if(mHandle != NULL) {
	   err = DL_PrepareLoading(mHandle,dataSpec);
	}else {
	   LLOGW("mHandle is NULL");
	   err = DL_ErrorInvalidComponent;
	}

	ESC_FUNC;
	return StatusFromDLError(err);
}

status_t IDownloader::startLoading(DataSpec* dataSpec) {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	//TODO  
	DL_ERRORTYPE err;
	if(mHandle != NULL) {
	   err = DL_StartLoading(mHandle,dataSpec);
	}else {
       LLOGW("mHandle is NULL");
	   err = DL_ErrorInvalidComponent;
	}
	ESC_FUNC;
	return StatusFromDLError(err);
}

status_t IDownloader::stopLoading() {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	DL_ERRORTYPE  err = DL_ErrorNone;
	if(mHandle != NULL) {
	    err = DL_StopLoading(mHandle);
	}else {
		LLOGW("mHandle is NULL");
		err = DL_ErrorInvalidComponent;
	}

	ESC_FUNC;
	return StatusFromDLError(err);
}

status_t IDownloader::pauseLoading() {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	//TODO  
	DL_ERRORTYPE err = DL_PauseLoading(mHandle);

	ESC_FUNC;
	return StatusFromDLError(err);
}

status_t IDownloader::resumeLoading() {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	//TODO  
	DL_ERRORTYPE err = DL_ResumeLoading(mHandle);

	ESC_FUNC;
	return StatusFromDLError(err);
}



status_t IDownloader::getConfig(DL_INDEXTYPE index, void *params, size_t size)       {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	DL_ERRORTYPE err = DL_ErrorNone;
	if(mHandle != NULL)
	     err = DL_GetConfig(mHandle, index, params);
	ESC_FUNC;
	return StatusFromDLError(err);
}

status_t IDownloader::setConfig( DL_INDEXTYPE index, const void *params, size_t size) {
	ENTER_FUNC;
	(void)size;
	Mutex::Autolock autoLock(mLock);
	DL_ERRORTYPE  err = DL_ErrorNone;
	if(mHandle != NULL) {
	    err =  DL_SetConfig( mHandle, index, const_cast<void *>(params));
	}else {
		LLOGW("mHandle is NULL");
		err = DL_ErrorInvalidComponent;
	}
	ESC_FUNC;
	return StatusFromDLError(err);
}

status_t IDownloader::getState(DL_STATETYPE* state) {
	ENTER_FUNC;
	Mutex::Autolock autoLock(mLock);
	DL_ERRORTYPE  err = DL_ErrorNone;
	if(mHandle != NULL) {
	    err = DL_GetState(mHandle, state);
	}else {
	    LLOGW("mHandle is NULL");
		err = DL_ErrorInvalidComponent;
	}
	ESC_FUNC;
	return StatusFromDLError(err);
}



void IDownloader::onMessages(std::list<downloader_message> &messages) {

	ENTER_FUNC;
	for (std::list<downloader_message>::iterator it = messages.begin(); it != messages.end(); ) {
		if (handleMessage(*it)) {
			it = messages.erase(it);
		} else {
			++it;
		}
	}

	if (!messages.empty()) {
		mObserver->onMessagesRecive(messages);
	}

	ESC_FUNC;
}

/*static*/
DL_ERRORTYPE IDownloader::OnEvent(
		DL_IN DL_HANDLETYPE hComponent,
		DL_IN DL_PTR pAppData,
		DL_IN DL_EVENTTYPE eEvent,
		DL_IN DL_U32 nData1,
		DL_IN DL_U32 nData2,
		DL_IN DL_PTR pEventData) {
	IDownloader *instance = static_cast<IDownloader*>(pAppData);
	if (instance->mDying == DL_TRUE) {
		return DL_ErrorNone;
	}
	return instance->owner()->OnEvent( eEvent, nData1, nData2, pEventData,instance->nodeID());
}

/*static*/
DL_ERRORTYPE IDownloader::OnFillThisBufferDone(
		DL_IN DL_HANDLETYPE hComponent,
		DL_IN DL_PTR pAppData,
		DL_IN DL_BUFFERHEADERTYPE *pBuffer) {
	IDownloader *instance = static_cast<IDownloader*>(pAppData);
	if (instance->mDying == DL_TRUE) {
		return DL_ErrorNone;
	}
	return instance->owner()->OnFillThisBufferDone(pBuffer ,instance->nodeID());
}



LIYL_NAMESPACE_END
