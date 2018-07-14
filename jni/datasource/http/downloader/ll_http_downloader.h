/*************************************************************************
  > File Name: ll_http_downloader.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年05月26日 星期五 15时15分15秒
 ************************************************************************/
#ifndef LL_DOWNLOADER_H
#define LL_DOWNLOADER_H

#include "ll_downloader.h"

LIYL_NAMESPACE_START
// HTTPDownloader deal with HTTP connetion post msg;
class HTTPDownloader :public IDownloader {
    public:
        HTTPDownloader(){
            mHTTPComponent = NULL;
        }

        virtual status_t initSetup(void *owner, const sp<DLObserver> &observer, const char *name);
        virtual ~HTTPDownloader();
        virtual status_t freeNode(DownLoaderFactory* mFactory);
        virtual status_t initCheck();                
        virtual status_t clear();
        virtual status_t setNotifyFun(void* cookie,notify_callback_f notifyFun); 
        virtual status_t onLoadStateChange();
        virtual status_t getConfig(DL_INDEXTYPE index, void *params, size_t size);

        virtual DL_BOOL handleMessage(/*const*/ downloader_message &msg);
    protected:
    private:
        void onProcessHeaderCB(DL_STRING pData,DataSpec *dataSpec);
        void onProcessSessionStatusCB(DL_U32 status,DataSpec *pDataSpec);

        DL_HANDLETYPE mHTTPComponent;
        Mutex mClearComponentLock;
        DISALLOW_EVIL_CONSTRUCTORS(HTTPDownloader);
};

LIYL_NAMESPACE_END

#endif
