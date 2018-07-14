/*************************************************************************
  > File Name: ll_dummy_observer.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年06月12日 星期一 17时02分21秒
 ************************************************************************/

#ifndef NET_DUMMY_OBSEVER_H
#define NET_DUMMY_OBSEVER_H
#include <list>
#include <string>

#include "ll_observer.h"
#include "ll_loadmanager.h"
#include "dl_utils.h"

using std::string;
using std::list;

LIYL_NAMESPACE_START


class DummyObserver :public DLObserver{
    public:
        DummyObserver(LL_LoadManager*);
        virtual status_t onMessagesRecive(std::list<downloader_message> &messages); 
        virtual ~DummyObserver();   
    protected:
    private:
        LL_LoadManager *mLoaderManager;
        DL_BOOL stopFlag;
        node_id mNode;
        DL_S32 content_length;
        struct timeval start_dl_time;
//#define DL_DEBUG_FLAG 1
#ifdef DL_DEBUG_FLAG
        bool isCreateFile;
        int64_t writedSize;
        FILE *fp;
#endif

        void handleMessage(downloader_message & msg);
        DISALLOW_EVIL_CONSTRUCTORS(DummyObserver);
};


LIYL_NAMESPACE_END

#endif//NET_DUMMY_OBSEVER_H
