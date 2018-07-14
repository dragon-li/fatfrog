/*************************************************************************
  > File Name: ll_dummy_observer.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com
  > Created Time: 2017年06月12日 星期一 17时02分21秒
 ************************************************************************/

#ifndef LL_DUMMY_OBSEVER_CPP
#define LL_DUMMY_OBSEVER_CPP
#include <list>
#include <string>
#include <fstream> 
#include "ll_dummy_observer.h"
#include "dl_utils.h"
#undef LOG_TAG
#define LOG_TAG "DummyObserver"


#define NODE_NUM  10
LIYL_NAMESPACE_START
DummyObserver::DummyObserver(LL_LoadManager* loaderMGR) {
    mLoaderManager = loaderMGR;
    stopFlag = DL_FALSE;
    content_length = 0;
    gettimeofday(&start_dl_time, NULL);


#ifdef DL_DEBUG_FLAG
    isCreateFile = false;
    writedSize = 0;
    fp = NULL;
#endif



}

status_t DummyObserver::onMessagesRecive(std::list<downloader_message> &messages) {
    ENTER_FUNC;
    for (std::list<downloader_message>::iterator it = messages.begin(); it != messages.end(); ) {
        handleMessage(*it);
        it = messages.erase(it);
    }
    ESC_FUNC;
    return OK;
}

void DummyObserver::handleMessage(downloader_message & msg) {

    ENTER_FUNC;
    std::string data;
    switch(msg.data1) {
        case DL_ET_HEADER:
			{
				LLOGV("header %p",msg.pEventData);
                DL_BUFFERHEADERTYPE* tmpBuf = (DL_BUFFERHEADERTYPE*)(msg.pEventData);
				//free tmpBuff when procceced
				if((tmpBuf->nFilledLen+tmpBuf->nOffset) == tmpBuf->nAllocLen) {
					LLOGD("node[%d] free tmpBuf->pBuffer %p",msg.node,tmpBuf->pBuffer);
					free(tmpBuf->pBuffer);
				}
				tmpBuf->pPreNode = NULL;
				tmpBuf->pNextNode= NULL;
				delete ((DL_BUFFERHEADERTYPE*)(tmpBuf));

#ifdef DL_DEBUG_FLAG
				node_id mNodeID = msg.node;
				char* tmp = (char*)malloc(64);
				sprintf(tmp,"/sdcard/dl/test%d.mp4",mNodeID);

				if( (!isCreateFile && (fp=fopen(tmp, "wb+")) == NULL) ){
					LLOGE("Cannot open file");
				}else {
					isCreateFile = true;
					fseek(fp, 0, SEEK_SET);
				}
				free(tmp);
				tmp = NULL;
#endif
			}
			break;
		case DL_ET_DATA:
			{
                if(stopFlag == DL_TRUE){
                    LLOGW("download task stop");
                    //return;
                }
                DL_BUFFERHEADERTYPE* tmpBuf = (DL_BUFFERHEADERTYPE*)(msg.pEventData);
                LLOGV("Buffer %p",tmpBuf);
                LLOGV("pBuffer %p",tmpBuf->pBuffer);
                LLOGV("Buffer->nFilledLen = %ld",tmpBuf->nFilledLen);
                LLOGV("pBuffer->nOffset   = %ld",tmpBuf->nOffset);
                content_length = content_length+tmpBuf->nFilledLen;
#ifdef DL_DEBUG_FLAG
                fwrite(tmpBuf->pBuffer + tmpBuf->nOffset, tmpBuf->nFilledLen, 1, fp);
                writedSize += tmpBuf->nFilledLen;
#endif

                /*if((tmpBuf->nFilledLen+tmpBuf->nOffset) == tmpBuf->nAllocLen) {
                    LLOGV("node[%d] free tmpBuf->pBuffer %p",msg.node,tmpBuf->pBuffer);
                    free(tmpBuf->pBuffer);
                }*/
                if(tmpBuf->pBuffer != NULL) {
                    LLOGV("node[%d] free tmpBuf->pBuffer %p",msg.node,tmpBuf->pBuffer);
                    free(tmpBuf->pBuffer);
                }


                tmpBuf->pBuffer  = NULL;
                tmpBuf->pPreNode = NULL;
                tmpBuf->pNextNode= NULL;
                delete ((DL_BUFFERHEADERTYPE*)(tmpBuf));

            }
            break;

        case DL_ET_STOP_CMPL:
            LLOGV("DL_HTTP_STOP_CMPL_CB");
            stopFlag = DL_TRUE;
            break;
        case DL_ET_DATA_CMPL:
            {
                LLOGD("DL_HTTP_RESPONSE_CMPL_CB node[%d] content_length = %ld",msg.node,content_length);
                struct timeval curr_time;
                uint64_t time_diff_ms;
                gettimeofday(&curr_time, NULL);
                time_diff_ms = (curr_time.tv_sec - start_dl_time.tv_sec) * 1000 
                    + (curr_time.tv_usec - start_dl_time.tv_usec) / 1000;
                LLOGD("node[%d] downloadCostTimeMs = %lld",msg.node,time_diff_ms);


            }
            break;
        case DL_ET_ERR:
            {
                LLOGE("DL_HTTP_RESPONSE_ERR_CB node[%d] content_length = %ld",msg.node,content_length);

                struct timeval curr_time;
                uint64_t time_diff_ms;
                gettimeofday(&curr_time, NULL);
                time_diff_ms = (curr_time.tv_sec - start_dl_time.tv_sec) * 1000 
                    + (curr_time.tv_usec - start_dl_time.tv_usec) / 1000;
                LLOGE("node[%d] downloadCostTimeMs = %lld",msg.node,time_diff_ms);


            }

            break;
        default:
            LLOGE("should not be here!!");
            break;
    }

    ESC_FUNC;
}


DummyObserver::~DummyObserver() {

    ENTER_FUNC;
#ifdef DL_DEBUG_FLAG
    LOGD("~YKDownloadObserver, mId: %d writedSize=%lld", mId,writedSize);
    if(fp != NULL) {
        fflush(fp);
        fclose(fp);
        fp = NULL;
    }
    isCreateFile = false;
    writedSize = 0;
#endif

    ESC_FUNC;
}
LIYL_NAMESPACE_END

#endif//NET_DUMMY_OBSEVER_CPP
