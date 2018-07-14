/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIYL_HTTP_DATA_SOURCE_H_

#define LIYL_HTTP_DATA_SOURCE_H_ 

#include "../api/DataSource.h"
#include "../../foundation/media/include/ABuffer.h"
#include "downloader/ll_observer.h"
#include "downloader/ll_loadmanager.h"

LIYL_NAMESPACE_START


class HttpDataSource : public DataSource ,public DLObserver {
    public:
        HttpDataSource();

        virtual void open(const char* uri,const KeyedVector<AString,AString> *header = NULL,AString* contentType = NULL);
        virtual status_t connectAtOffset(off64_t offset,size_t size);
        virtual ssize_t readAt(off64_t offset, void *data, size_t size);
        ssize_t readAtNonBlocking(off64_t offset, void *data, size_t size);
        virtual status_t disconnect();
        virtual void close(); 

        virtual AString getMIMEType() const;
        virtual AString toString() const;
        virtual status_t getSize(off64_t *size);

        void queueBuffer(sp<ABuffer> &buffer);
        void queueEOS(status_t finalResult);
        void reset();

        size_t countQueuedBuffers();
    protected:
         ////////////////////////////////////////////
        virtual status_t onMessagesRecive(std::list<downloader_message> &messages);
        void handleMessage(downloader_message & msg);
        ////////////////////////////////////////////
        virtual ~HttpDataSource();

    private:
        Mutex mLock;
        Condition mCondition;

        off64_t mOffset;
        List<sp<ABuffer> > mBufferQueue;
        status_t mFinalResult;

        FILE *mBackupFile;

        AString mUri;
        AString mUserAgent;
		AString mMIMEType;
		off64_t mSize;
        DataSpec* mDataSpec;
        LL_LoadManager* mLoadManager;

        Mutex mCloseLock;
		bool  mCloseFlag;
		
        ssize_t readAt_l(off64_t offset, void *data, size_t size);
        void reset_l();
		status_t parseHeader(const char* line);

        DISALLOW_EVIL_CONSTRUCTORS(HttpDataSource);
};

LIYL_NAMESPACE_END

#endif  // LIYL_HTTP_DATA_SOURCE_H_ 
