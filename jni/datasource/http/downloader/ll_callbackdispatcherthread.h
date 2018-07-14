/*************************************************************************
  > File Name: cache_callbackdispatcherthread.h
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 年月27日 星期六 16时02分57秒
 ************************************************************************/
#ifndef CACHE_CALLBACKDISPATCHERTHREAD_H
#define CACHE_CALLBACKDISPATCHERTHREAD_H 
#include <string>
#include <list>

#include "error.h"

using std::string;
using std::list;



LIYL_NAMESPACE_START
////////////////////////////////////////////////////////////////////////////////

// This provides the underlying Thread used by CallbackDispatcher.
// Note that deriving CallbackDispatcher from Thread does not work.

class CallbackDispatcher;
class IDownloader;


// This provides the underlying Thread used by CallbackDispatcher.
// Note that deriving CallbackDispatcher from Thread does not work.

struct CallbackDispatcherThread : public Thread {
    CallbackDispatcherThread(CallbackDispatcher *dispatcher)
        : mDispatcher(dispatcher) {
        }

    private:
    CallbackDispatcher *mDispatcher;

    bool threadLoop();

    CallbackDispatcherThread(const CallbackDispatcherThread &);
    CallbackDispatcherThread &operator=(const CallbackDispatcherThread &);
};


////////////////////////////////////////////////////////////////////////////////

class CallbackDispatcher : virtual public RefBase {
    public:
        CallbackDispatcher(IDownloader *owner);

        void post(const downloader_message &msg);


        bool loop();

        virtual ~CallbackDispatcher();
    protected:

    private:
        Mutex mLock;

        IDownloader *mOwner;
        bool mDone;
        Condition mQueueChanged;
        std::list<downloader_message> mQueue;

        sp<CallbackDispatcherThread> mThread;

        void dispatch(std::list<downloader_message> &messages);

        CallbackDispatcher(const CallbackDispatcher &);
        CallbackDispatcher &operator=(const CallbackDispatcher &);
};





////////////////////////////////////////////////////////////////////////////////

LIYL_NAMESPACE_END
extern int threadloop(void *);
#endif
