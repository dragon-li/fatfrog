/*
 * ll_downloader.h
 *
 *  Created on: 年月日
 *      Author: wangqianpeng */

#ifndef CACHE_DOWNLOADER_H
#define CACHE_DOWNLOADER_H 
#include <string>
#include <list>

#include "error.h"
#include "ll_observer.h"
#include "ll_loadmanager.h"
#include "dl_common_ext.h"

using std::string;
using std::list;

LIYL_NAMESPACE_START

class DLObserver;
class DownLoaderFactory;
class LL_LoadManager;

typedef status_t (*notify_callback_f)(int,int,int,void*);



class IDownloader :virtual public RefBase {
    public:
        IDownloader();

        virtual status_t initSetup(void *owner, const sp<DLObserver> &observer, const char *name){ENTER_FUNC;ESC_FUNC;return NO_ERROR;}
        virtual LL_LoadManager *owner();
        virtual sp<DLObserver> observer();
        void setHandle(node_id node_id, DL_HANDLETYPE handle);
        void setDataSpec(DataSpec* dataSpec);
        node_id nodeID();

        virtual status_t freeNode(DownLoaderFactory* mFactory);
        virtual status_t initCheck(){ENTER_FUNC;ESC_FUNC;return NO_ERROR;}
        virtual status_t clear(){ENTER_FUNC;ESC_FUNC;return INVALID_OPERATION;}                         
        virtual status_t setNotifyFun(void* cookie,notify_callback_f notifyFun){ENTER_FUNC;ESC_FUNC;return INVALID_OPERATION;}                                                                                         


        virtual status_t onLoadComplete(node_id node,DataSpec* dataSpec,DL_S64 elapsedRealtimeMs,DL_S64 loadDurationMs){ENTER_FUNC;ESC_FUNC;return INVALID_OPERATION;}

        virtual status_t onLoadCanceled(node_id node,DataSpec* dataSpec,DL_S64 elapsedRealtimeMs,DL_S64 loadDurationMs){ENTER_FUNC;ESC_FUNC;return INVALID_OPERATION;}                         

        virtual status_t onLoadError(node_id node,DataSpec* dataSpec,DL_S64 elapsedRealtimeMs,DL_S64 loadDurationMs){ENTER_FUNC;ESC_FUNC;return INVALID_OPERATION;}
        virtual status_t onLoadStateChange(){ENTER_FUNC;ESC_FUNC;return INVALID_OPERATION;}



        virtual status_t prepareLoading(DataSpec *dataSpec);
        virtual status_t startLoading(DataSpec *dataSpec);
        virtual status_t stopLoading();
        virtual status_t pauseLoading();
        virtual status_t resumeLoading();

        virtual status_t getConfig(DL_INDEXTYPE index, void *params, size_t size);
        virtual status_t setConfig(DL_INDEXTYPE index, const void *params, size_t size);
        virtual status_t getState(DL_STATETYPE* state);

        virtual DL_BOOL handleMessage(/*const*/ downloader_message &msg){ENTER_FUNC;ESC_FUNC;return DL_FALSE ;}
        virtual void onEventHandle(downloader_message &msg){ENTER_FUNC;ESC_FUNC;return ;}
        // handles messages and removes them from the list ,maybe be used in future.
        void onMessages(std::list<downloader_message> &messages);
        status_t sendCommand( DL_COMMANDTYPE cmd, DL_S32 param);
        static status_t StatusFromDLError(DL_ERRORTYPE err); 
        virtual ~IDownloader();

        static DL_CALLBACKTYPE kCallbacks;  
        Mutex mLock;
        LL_LoadManager* mOwner;
        node_id mNodeID;
        DL_HANDLETYPE mHandle;
        DataSpec *mDataSpec;
        std::string mDataSpecUrl;
        std::string mDataSpecKey;
        std::string mDataSpecUserAgent;


        sp<DLObserver> mObserver;
        volatile DL_BOOL mDying;

    protected:
        static DL_ERRORTYPE OnEvent(
                DL_IN DL_HANDLETYPE hComponent,
                DL_IN DL_PTR pAppData,
                DL_IN DL_EVENTTYPE eEvent,
                DL_IN DL_U32 nData1,
                DL_IN DL_U32 nData2,
                DL_IN DL_PTR pEventData);

        static DL_ERRORTYPE OnFillThisBufferDone(
                DL_IN DL_HANDLETYPE hComponent,
                DL_IN DL_PTR pAppData,
                DL_IN DL_BUFFERHEADERTYPE *pBuffer);
    private:
        DISALLOW_EVIL_CONSTRUCTORS(IDownloader);

};

LIYL_NAMESPACE_END

#endif /* CACHE_DOWNLOADER_H_ */
