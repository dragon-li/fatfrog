/*************************************************************************
  > File Name: ILL_loadmanager.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com
  > Created Time: 2017年05月24日 星期三 14时44分54秒
 ************************************************************************/

#ifndef LL_LOADMANAGER_H_
#define LL_LOADMANAGER_H_
#include <string>
#include <vector>
#include <map>

#include "error.h"
#include "ll_observer.h"
#include "ll_downloader.h"
#include "ll_callbackdispatcherthread.h"
#include "ll_downloader_factory.h"

using std::string;
using std::vector;
using std::map;

LIYL_NAMESPACE_START
class CallBackDispatcherThread;
class CallBackDispatcher;
class DownloaderFactory;

class LL_LoadManager: virtual public RefBase {

    public:
        virtual status_t getConfig( DL_INDEXTYPE index, void *params, size_t size,node_id node = 0);

        virtual status_t setConfig( DL_INDEXTYPE index, const void *params, size_t size,node_id node = 0);

        virtual status_t getState( DL_STATETYPE* state,node_id node = 0);
        virtual status_t allocateNode( const char *name,DataSpec *dataSpec, const sp<liyl::DLObserver> &observer, node_id *node = 0);
        virtual status_t freeNode(node_id node = 0);
        virtual status_t prepareLoader(node_id node = 0,DataSpec *dataSpec = NULL);
        virtual status_t startLoader(node_id node = 0);
        virtual status_t stopLoader(node_id node = 0);
        virtual status_t pauseLoader(node_id node = 0); //UNUSED
        virtual status_t resumeLoader(node_id node = 0);//UNUSED
        virtual status_t checkLoader(node_id node = 0);                                                                                         
        DL_ERRORTYPE OnEvent(
                DL_IN DL_EVENTTYPE eEvent,
                DL_IN DL_U32 nData1,
                DL_IN DL_U32 nData2,
                DL_IN DL_PTR pEventData,node_id node = 0);


        DL_ERRORTYPE OnFillThisBufferDone( DL_IN DL_BUFFERHEADERTYPE *pBuffer,node_id node = 0);

        void invalidateNodeID(node_id node = 0);

        virtual ~LL_LoadManager();

        static sp<LL_LoadManager>& getInstance();
        static bool hasInstance();
    protected:
    private:

        LL_LoadManager();
        static Mutex sLock;
        //static sp<LL_LoadManager> sInstance;
        Mutex mLock;
        size_t mNodeCounter;

        map<node_id,sp<IDownloader> > mNodeIDToInstance;
        map<node_id, sp<CallbackDispatcher> > mDispatchers;

        node_id makeNodeID_l(sp<IDownloader> instance);
        const sp<IDownloader>  findInstance(node_id node);
        sp<CallbackDispatcher> findDispatcher(node_id node);

        void invalidateNodeID_l(node_id node);


        DISALLOW_EVIL_CONSTRUCTORS(LL_LoadManager);    


};

LIYL_NAMESPACE_END
#endif /* LL_LOADMANAGER_H_ */


