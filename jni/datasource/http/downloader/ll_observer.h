/*************************************************************************
  > File Name: cache_observer.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年05月26日 星期五 15时21分20秒
 ************************************************************************/

#ifndef NET_DLOBSERVER_H
#define NET_DLOBSERVER_H
#include <list>

#include "error.h"

using std::string;
using std::list;

LIYL_NAMESPACE_START

class DLObserver :virtual public RefBase {
    public:
        DLObserver() {
            mId = -1;
        }    
        virtual void setNodeId(node_id id) {
            mId = id;
        }
        virtual status_t onMessagesRecive(std::list<downloader_message> &messages) { LLOGD("liyl DLObserver");return OK;}
        virtual status_t onMessageRecive(downloader_message &message) { LLOGD("liyl DLObserver");return OK;}
        virtual ~DLObserver() {}    
    protected:
        node_id mId;
    private:
		DISALLOW_EVIL_CONSTRUCTORS(DLObserver);
};
LIYL_NAMESPACE_END

#endif//NET_DLOBSERVER_H
