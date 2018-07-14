/*************************************************************************
  > File Name: ILL_loadmanager.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 年05月28日 星期日 20时47分01秒
 ************************************************************************/

#include "ll_loadmanager.h"

#undef LOG_TAG
#define LOG_TAG "LL_LoadManager"

LIYL_NAMESPACE_START


#undef ENTER_FUNC
#undef ESC_FUNC
#define ENTER_FUNC do{ LLOGV("func enter %s  %d",__func__,__LINE__);}while(0)
#define ESC_FUNC do{ LLOGV("func esc %s  %d",__func__,__LINE__);}while(0)
#define ENTER_FUNC_NODE(NODE) do{ LLOGV("func enter node[%d] %s  %d",(NODE),__func__,__LINE__);}while(0)
#define ESC_FUNC_NODE(NODE) do{ LLOGV("func esc ndoe[%d] %s  %d",(NODE),__func__,__LINE__);}while(0)


typedef std::map<node_id,sp<CallbackDispatcher> > stl_map_CallbackDisp;

typedef std::map<node_id,sp<IDownloader> > stl_map_IDownloader;

Mutex LL_LoadManager::sLock;


LL_LoadManager::LL_LoadManager() {
    ENTER_FUNC;
    mNodeCounter = 0; 
    //TODO
    DownloaderFactory::registerBuiltinFactories();
    ESC_FUNC;
}

LL_LoadManager::~LL_LoadManager() {
    ENTER_FUNC;
    DownloaderFactory::unregisterFactory(DL_TYPE_HTTP);
    ESC_FUNC;
}

/*static*/
sp<LL_LoadManager>& LL_LoadManager::getInstance() {
    Mutex::Autolock lock(sLock);
    //C++ standard : don't delete local static
    static sp<LL_LoadManager> sInstance = new LL_LoadManager();
    return sInstance;
}


status_t LL_LoadManager::allocateNode( const char *name,DataSpec *dataSpec, const sp<liyl::DLObserver> &observer, node_id *node ) {
    ENTER_FUNC;
    Mutex::Autolock autoLock(mLock);

    *node = 0;

    downloader_type type = DownloaderFactory::getDownloadType(dataSpec->url);
    sp<IDownloader> instance = DownloaderFactory::createDownloader(type ,NULL/*pObserver*/,(notify_callback_f)NULL/*TODO*/,(pid_t)-1/*TODO*/);

    //TODO need init Downloader for HTTP or P2P
    if(instance.get() == NULL) {
        LLOGE("IDownloader init failed!!");
        return UNEXPECTED_NULL;

    }

    instance->setDataSpec(dataSpec);
    instance->initSetup(this,observer,NULL);

    *node = makeNodeID_l(instance);
    LLOGD("allocate download[%ld]",static_cast<DL_S32>(*node));

    sp<CallbackDispatcher> callDis =  new CallbackDispatcher(instance.get());
    if(mDispatchers.insert(map<node_id,sp<CallbackDispatcher> >::value_type(*node, callDis)).second) {
        LLOGV("dispatcher inserted success");
    }else {
        LLOGW("dispatcher[%d] inserted failed ",*node);
    }

    //TODO need init Buffer;
    instance->setHandle(*node,NULL);

    ESC_FUNC;
    return OK;
}

status_t LL_LoadManager::freeNode(node_id node) {
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    CHECK(instance.get() != NULL);
    if(instance == NULL) {
        LLOGD("cannot find instance (%d)", node);
        return OK;
    }
    status_t err = instance->freeNode(NULL/*mMaster*/);

    {
        Mutex::Autolock autoLock(mLock);
        stl_map_CallbackDisp::iterator it = mDispatchers.find(node);
        if(it != mDispatchers.end()){
            mDispatchers.erase(it);
            LLOGV("erase the dispatcher by node = %d !",node);
        }else {
            LLOGW("can't find Dispatcher by node = %d!",node);
        }
    }

    ESC_FUNC_NODE(node);
    return err;
}

status_t LL_LoadManager::prepareLoader(node_id node,DataSpec *dataSpec){
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC;
        return NAME_NOT_FOUND;
    }

    status_t ret = instance->prepareLoading(dataSpec);

    ESC_FUNC_NODE(node);

    return ret;
}

status_t LL_LoadManager::startLoader(node_id node) {
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC_NODE(node);
        return NAME_NOT_FOUND;
    }

    status_t ret = instance->startLoading(NULL/*dataSpec*/);

    ESC_FUNC_NODE(node);

    return ret;
}
status_t LL_LoadManager::stopLoader(node_id node ) {
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC_NODE(node);
        return NAME_NOT_FOUND;
    }

    status_t ret = instance->stopLoading(/*dataSpec*/);

    ESC_FUNC_NODE(node);

    return ret;
}

status_t LL_LoadManager::checkLoader(node_id node){return OK;}                                                                                         

status_t LL_LoadManager::pauseLoader(node_id node) { 
    ENTER_FUNC;
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC;
        return NAME_NOT_FOUND;
    }

    status_t ret = instance->pauseLoading(/*dataSpec*/);

    ESC_FUNC;

    return ret;
}

status_t LL_LoadManager::resumeLoader(node_id node) {
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC_NODE(node);
        return NAME_NOT_FOUND;
    }

    status_t ret = instance->resumeLoading(/*dataSpec*/);

    ESC_FUNC_NODE(node);

    return ret;
}

status_t LL_LoadManager::getConfig(DL_INDEXTYPE index,void *params ,size_t size,node_id node) {
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    {
        //workaround for the liyl call this method asynchronized
        Mutex::Autolock autoLock(mLock);
        if(instance == NULL) {
            ESC_FUNC_NODE(node);
            return NAME_NOT_FOUND;
        }
    }
    ESC_FUNC_NODE(node);
    return instance->getConfig( index, params, size);
}

status_t LL_LoadManager::setConfig( DL_INDEXTYPE index, const void *params, size_t size,node_id node ) {
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC_NODE(node);
        return NAME_NOT_FOUND;
    }

    ESC_FUNC_NODE(node);
    return instance->setConfig(
            index, params, size);
}

status_t LL_LoadManager::getState( DL_STATETYPE* state,node_id node){
    ENTER_FUNC_NODE(node);
    sp<IDownloader> instance = findInstance(node);
    if(instance.get() == NULL) {
        ESC_FUNC_NODE(node);
        return NAME_NOT_FOUND;
    }
    ESC_FUNC_NODE(node);
    return instance->getState(
            state);
}




DL_ERRORTYPE LL_LoadManager::OnEvent(
        DL_IN DL_EVENTTYPE eEvent,
        DL_IN DL_U32 nData1,
        DL_IN DL_U32 nData2,
        DL_IN DL_PTR pEventData,node_id node) {

    if (eEvent == DL_EventCmdComplete &&
            nData1 == DL_CommandStateSet) {
        if (nData2 == DL_StateExecuting) {
            LLOGD("node[%d] compenont was DL_StateExecuting,already",node);
        } else if (nData2 == DL_StateInit) {
            LLOGD("node[%d] compenont was DL_StateInit,already",node);
        }
        return DL_ErrorNone;
    }

    if(eEvent == DL_EventError) {
        LLOGE("node[%d] compenont encounter a error %ld ",node,nData2);
    }


    downloader_message msg;
    msg.type = (DL_U32)eEvent;
    msg.node = (node_id)node;
    msg.data1 = (DL_U32)nData1;
    msg.data2 = (DL_U32)nData2;
    msg.pEventData = (DL_PTR)pEventData;



    sp<CallbackDispatcher> dispatcher = findDispatcher(node);
    if(dispatcher != NULL) {
        dispatcher->post(msg);
    }else {
        LLOGE("dispatcher[%d] is NULL !!",node);
        return DL_ErrorComponentNotFound;
    }

    return  DL_ErrorNone;
}



DL_ERRORTYPE LL_LoadManager::OnFillThisBufferDone( DL_BUFFERHEADERTYPE* buffer,node_id node) {
    LLOGV("OnFillThisBufferDone %d",node);

    //note :msg need delete after deal with by reciever
    CHECK(buffer != NULL);
    //LLOGV("buffer size = %ld pData = %s",((DL_BUFFERHEADERTYPE*)buffer)->nFilledLen,((DL_BUFFERHEADERTYPE*)buffer)->pBuffer);
    downloader_message msg;
    msg.type = (DL_U32)DL_EventCmdComplete;
    msg.node = (node_id)node;
    msg.data1 = (DL_U32)DL_ET_DATA;
    msg.data2 = (DL_U32)0;
    msg.pEventData = (DL_PTR)buffer;

    sp<CallbackDispatcher> dispatcher = findDispatcher(node);
    if(dispatcher != NULL) {
        dispatcher->post(msg);
    }else {
        LLOGE("dispatcher[%d] is NULL !!",node);
        return DL_ErrorComponentNotFound; 
    }

    return DL_ErrorNone;
}

node_id LL_LoadManager::makeNodeID_l(sp<IDownloader> instance) {
    // mLock is already held.

    node_id node = (node_id)++mNodeCounter;
    if(mNodeIDToInstance.insert(stl_map_IDownloader::value_type(node,instance )).second) {

    }else {
        LLOGW("downloader[%d] inserted failed",node);
    }

    return node;
}

const sp<IDownloader> LL_LoadManager::findInstance(node_id node) {
    Mutex::Autolock autoLock(mLock);

    stl_map_IDownloader::iterator it = mNodeIDToInstance.find(node);
    if(it == mNodeIDToInstance.end()) {
        return NULL;
    }else {
        return it->second;
    }

}


sp<CallbackDispatcher> LL_LoadManager::findDispatcher(node_id node) {
    Mutex::Autolock autoLock(mLock);

    stl_map_CallbackDisp::iterator it = mDispatchers.find(node);
    if(it == mDispatchers.end()) {
        return NULL;
    }else {
        return it->second;
    }

}

void LL_LoadManager::invalidateNodeID(node_id node) {
    Mutex::Autolock autoLock(mLock);
    invalidateNodeID_l(node);
}

void LL_LoadManager::invalidateNodeID_l(node_id node) {
    // mLock is held.
    mNodeIDToInstance.erase(node);
}

#undef ENTER_FUNC
#undef ESC_FUNC 

LIYL_NAMESPACE_END
