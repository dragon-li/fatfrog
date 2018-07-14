/*************************************************************************
  > File Name: ll_callbackdispatcherthread.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年05月28日 星期日 15时49分27秒
 ************************************************************************/

#include "ll_callbackdispatcherthread.h"
#include "ll_downloader.h"


LIYL_NAMESPACE_START


CallbackDispatcher::CallbackDispatcher(IDownloader *owner)
    : mOwner(owner),
    mDone(false) {
        mThread = new CallbackDispatcherThread(this);
        mThread->run("DLCallbackDisp", LIYL_PRIORITY_FOREGROUND);

    }

CallbackDispatcher::~CallbackDispatcher() {
    ENTER_FUNC;
    {
        Mutex::Autolock autoLock(mLock);

        mDone = true;
        mQueueChanged.signal();
    }

    // A join on self can happen if the last ref to CallbackDispatcher
    // is released within the CallbackDispatcherThread loop
    status_t status = mThread->join();
    if (status != WOULD_BLOCK) {
        // Other than join to self, the only other error return codes are
        // whatever start() returns, and we don't override that
        //TODO 
        CHECK_EQ(status, (status_t)NO_ERROR);
    }
    ESC_FUNC;
}


void CallbackDispatcher::post(const downloader_message &msg) {
    Mutex::Autolock autoLock(mLock);
    mQueue.push_back(msg);
    mQueueChanged.signal();
}




void CallbackDispatcher::dispatch(std::list<downloader_message> &messages) {
    if (mOwner == NULL) {
        LLOGV("Would have dispatched the messages to a node that's already gone.");
        return;
    }
    mOwner->onMessages(messages);
}

bool CallbackDispatcher::loop() {
    for (;;) {
        std::list<downloader_message> messages;
        {
            Mutex::Autolock autoLock(mLock);
            while (!mDone && mQueue.empty()) {
                mQueueChanged.wait(mLock);
            }

            if (mDone) {
                break;
            }

            messages.swap(mQueue);
        }

        dispatch(messages);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool CallbackDispatcherThread::threadLoop() { 
    return mDispatcher->loop(); 
}

////////////////////////////////////////////////////////////////////////////////


LIYL_NAMESPACE_END
