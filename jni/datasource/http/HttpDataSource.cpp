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

//#define LOG_NDEBUG 0
#define LOG_TAG "HttpDataSource"
#include "../../foundation/system/core/utils/Log.h"
#include "HttpDataSource.h"


#define SAVE_BACKUP     0

LIYL_NAMESPACE_START

HttpDataSource::HttpDataSource()
    : mOffset(0),
    mFinalResult(OK),
    mBackupFile(NULL),
    mSize(0ll),
    mDataSpec(NULL),
    mLoadManager(NULL), 
    mCloseFlag(false) {
#if SAVE_BACKUP
        mBackupFile = fopen("/data/misc/backup.ts", "wb");
        CHECK(mBackupFile != NULL);
#endif
    }

HttpDataSource::~HttpDataSource() {
    if (mBackupFile != NULL) {
        fclose(mBackupFile);
        mBackupFile = NULL;
    }
    mFinalResult = OK;
    mBackupFile = NULL;
    mSize = 0ll;
    mDataSpec = NULL;
    mLoadManager = NULL; 
    mCloseFlag = false;
}

//////////////////////////////////////////////////////////////
void HttpDataSource::open(const char* uri,const KeyedVector<AString,AString> *header,AString* contentType) {
    ENTER_FUNC;
    Mutex::Autolock autoLock(mLock);
    if(mLoadManager == NULL) {
        mLoadManager =  LL_LoadManager::getInstance().get();
    }
    mUri.append(uri);
    mUserAgent.append("User-Agent: Android6.0");
    CHECK(mDataSpec == NULL);
    mDataSpec = new DataSpec();
    mDataSpec->position = 0ll;
    mDataSpec->connectTimeoutMs= 20*1000;
    mDataSpec->length = 1000ll;
    mDataSpec->url = mUri.c_str(); 
    mDataSpec->headUserAgent = mUserAgent.c_str(); 
    node_id tmpId = -1;
    mLoadManager->allocateNode(NULL/*name*/,mDataSpec/*dataSpec*/,this/*observer*/,&tmpId);
    setNodeId(tmpId);
    ESC_FUNC;
}

status_t HttpDataSource::connectAtOffset(off64_t offset,size_t size) {
    ENTER_FUNC;
    Mutex::Autolock autoLock(mLock);
    CHECK(mDataSpec != NULL);
    mDataSpec->position = offset;
    mDataSpec->length = size;
    mLoadManager->prepareLoader(mId,mDataSpec);
    mLoadManager->startLoader(mId);
    ESC_FUNC;
    return OK;
}

status_t HttpDataSource::disconnect() {
    ENTER_FUNC;
    Mutex::Autolock autoLock(mLock);
    status_t ret = mLoadManager->stopLoader(mId);
    ESC_FUNC;
    return ret;
}

void HttpDataSource::close() {
    ENTER_FUNC;
    {
        Mutex::Autolock closeLock(mCloseLock);
        if(mCloseFlag == true) {
            return;
        }else {
            mCloseFlag = true;
        }
    }

    Mutex::Autolock autoLock(mLock);
    mLoadManager->freeNode(mId);
    setNodeId(-1);
    reset_l();
    CHECK(mDataSpec != NULL);
    free(mDataSpec);
    mDataSpec = NULL;
    mUri.clear();
    mUserAgent.clear();
    ESC_FUNC;
}

//////////////////////////////////////////////////////////////

status_t HttpDataSource::onMessagesRecive(std::list<downloader_message> &messages) {
    ENTER_FUNC;
    for (std::list<downloader_message>::iterator it = messages.begin(); it != messages.end(); ) {
        handleMessage(*it);
        it = messages.erase(it);
    }
    ESC_FUNC;
    return OK;
}

void HttpDataSource::handleMessage(downloader_message & msg) {
    //ENTER_FUNC;
    switch(msg.data1) {
        case DL_ET_HEADER:
            {
                LLOGV("header %p",msg.pEventData);
                DL_BUFFERHEADERTYPE* tmpBuf = (DL_BUFFERHEADERTYPE*)(msg.pEventData);
                //free tmpBuff when procceced
                if((tmpBuf->nFilledLen+tmpBuf->nOffset) == tmpBuf->nAllocLen) {
                    LLOGD("node[%d] free tmpBuf->pBuffer %s",msg.node,tmpBuf->pBuffer);
                    parseHeader((const char*)(tmpBuf->pBuffer));
                    free(tmpBuf->pBuffer);
                }
                tmpBuf->pPreNode = NULL;
                tmpBuf->pNextNode= NULL;
                delete ((DL_BUFFERHEADERTYPE*)(tmpBuf));

            }
            break;
        case DL_ET_DATA:
            {
                DL_BUFFERHEADERTYPE* tmpBuf = (DL_BUFFERHEADERTYPE*)(msg.pEventData);
                LLOGD("Buffer %p,pBuffer %p",tmpBuf,tmpBuf->pBuffer);
                LLOGV("Buffer->nFilledLen = %ld Buffer->nOffset = %ld",tmpBuf->nFilledLen,tmpBuf->nOffset);

                CHECK_LE(tmpBuf->nFilledLen, tmpBuf->nAllocLen);
                sp<ABuffer> buffer = new ABuffer(tmpBuf->pBuffer,tmpBuf->nAllocLen);
                buffer->setRange(tmpBuf->nOffset,tmpBuf->nFilledLen);
                buffer->setOwnsData(true);//Be Careful,free data by ABuffer
                queueBuffer(buffer);

                tmpBuf->pBuffer  = NULL;
                tmpBuf->pPreNode = NULL;
                tmpBuf->pNextNode= NULL;
                delete ((DL_BUFFERHEADERTYPE*)(tmpBuf));

            }
            break;

        case DL_ET_STOP_CMPL:
            LLOGV("DL_HTTP_STOP_CMPL_CB");
            break;
        case DL_ET_DATA_CMPL:
            {
                LLOGV("DL_HTTP_RESPONSE_CMPL_CB node[%d] ",msg.node);
                queueEOS(ERROR_END_OF_STREAM);

            }
            break;
        case DL_ET_ERR:
            {
                LLOGE("DL_HTTP_RESPONSE_ERR_CB node[%d] err = %d",msg.node,-msg.data2);
                status_t err = (status_t)(-msg.data2);
                queueEOS(err);

            }

            break;
        default:
            LLOGE("should not be here!!");
            break;
    }

    //ESC_FUNC;
}


status_t HttpDataSource::parseHeader(const char* line) {
    AString lineStr(line);
    if(lineStr.startsWith("Server")) {
    }else if(lineStr.startsWith("Date")) {
    }else if(lineStr.startsWith("Content-Length")) {
    }else if(lineStr.startsWith("Content-Encoding")) {
    }else if(lineStr.startsWith("Location")) {
    }else if(lineStr.startsWith("Last-Modified")) {
    }else if(lineStr.startsWith("Accept-Ranges")) {
    }else if(lineStr.startsWith("Expires")) {
    }else if(lineStr.startsWith("Cache-Control")) {
    }else if(lineStr.startsWith("ETag")) {
    }else {
    }
    return OK;

}

//////////////////////////////////////////////////////////////

size_t HttpDataSource::countQueuedBuffers() {
    Mutex::Autolock autoLock(mLock);

    return mBufferQueue.size();
}

ssize_t HttpDataSource::readAtNonBlocking(
        off64_t offset, void *data, size_t size) {
    Mutex::Autolock autoLock(mLock);

    if (offset != mOffset) {
        LLOGE("Attempt at reading non-sequentially from HttpDataSource.");
        return -EPIPE;
    }

    size_t totalAvailable = 0;
    for (List<sp<ABuffer> >::iterator it = mBufferQueue.begin();
            it != mBufferQueue.end(); ++it) {
        sp<ABuffer> buffer = *it;

        totalAvailable += buffer->size();

        if (totalAvailable >= size) {
            break;
        }
    }

    if (totalAvailable < size) {
        return mFinalResult == OK ? -EWOULDBLOCK : mFinalResult;
    }

    return readAt_l(offset, data, size);
}

ssize_t HttpDataSource::readAt(off64_t offset, void *data, size_t size) {
    Mutex::Autolock autoLock(mLock);
    return readAt_l(offset, data, size);
}

ssize_t HttpDataSource::readAt_l(off64_t offset, void *data, size_t size) {
    if (offset != mOffset) {
        LLOGE("Attempt at reading non-sequentially from HttpDataSource.");
        return -EPIPE;
    }

    size_t sizeDone = 0;

    while (sizeDone < size) {
        while (mBufferQueue.empty() && mFinalResult == OK) {
            mCondition.wait(mLock);
        }

        if (mBufferQueue.empty()) {
            if (sizeDone > 0) {
                mOffset += sizeDone;
                return sizeDone;
            }

            return mFinalResult;
        }

        sp<ABuffer> buffer = *mBufferQueue.begin();

        size_t copy = size - sizeDone;

        if (copy > buffer->size()) {
            copy = buffer->size();
        }

        memcpy((uint8_t *)data + sizeDone, buffer->data(), copy);

        sizeDone += copy;

        buffer->setRange(buffer->offset() + copy, buffer->size() - copy);

        if (buffer->size() == 0) {
            mBufferQueue.erase(mBufferQueue.begin());
        }
    }

    mOffset += sizeDone;

    return sizeDone;
}

void HttpDataSource::queueBuffer(sp<ABuffer> &buffer) {
    Mutex::Autolock closeLock(mCloseLock);
    if(mCloseFlag == true) {
        buffer.clear();
        return;
    }
    Mutex::Autolock autoLock(mLock);

    if (mFinalResult != OK) {
        return;
    }

#if SAVE_BACKUP
    if (mBackupFile != NULL) {
        CHECK_EQ(fwrite(buffer->data(), 1, buffer->size(), mBackupFile),
                buffer->size());
    }
#endif

    mBufferQueue.push_back(buffer);
    mCondition.broadcast();
}

void HttpDataSource::queueEOS(status_t finalResult) {
    Mutex::Autolock closeLock(mCloseLock);
    if(mCloseFlag == true) {
        return;
    }
    CHECK_NE(finalResult, (status_t)OK);
    Mutex::Autolock autoLock(mLock);

    mFinalResult = finalResult;
    mCondition.broadcast();
}

void HttpDataSource::reset() {
    Mutex::Autolock autoLock(mLock);
    // XXX FIXME: If we've done a partial read and waiting for more buffers,
    // we'll mix old and new data...
    reset_l();
}

void HttpDataSource::reset_l() {
    mFinalResult = OK;
    mBufferQueue.clear();
}

AString HttpDataSource::getMIMEType() const {
    return mMIMEType;
}

AString HttpDataSource::toString() const {
    return mUri;
}

status_t HttpDataSource::getSize(off64_t *size) {
    *size = mSize;
    return OK;
}

LIYL_NAMESPACE_END
