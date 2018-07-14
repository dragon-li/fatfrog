// #define LOG_NDEBUG 0

#define LOG_TAG "LLCurlMulti"

#include "ll_curlmulti.h"
#include "ll_curl_utils.h"

#include <ctype.h>
#include <vector>

#define USER_AGENT_APPLE "Mozilla/5.0"
#define USER_AGENT_LIYL "Mozilla/5.0"
#define TIMEOUT_IN_MS 30000

using std::vector;
USING_NAMESPACE_LIYL

void *init_http_thread() {

    http_thread *thread = (http_thread *)malloc(sizeof(http_thread ));
    memset(thread, 0x0, sizeof(http_thread));
    thread->task_index = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    thread->cm = curl_multi_init();
    for(int i=0;i< 100;i++)
        thread->task[i] = NULL;
    thread->mStillRunning = false;
    thread->hasDownloadError = false;
    thread->i_filledsize  = 0ll;
    thread->f_speed_KBps = 0;
    thread->progress_last_pos = 0ll;
    gettimeofday(&(thread->progress_last_time), NULL);

    thread->last_phase_code  =0;
    thread->action_id        =0;

    thread->mState = DL_STATE_INIT;
    return (void *)thread;
}

void deinit_http_thread(http_thread* thread){
    {
        Mutex::Autolock threadLock(thread->mApiLock);

        for (int i = 0 ; i< 100; i++) {
            http_task *task = thread->task[i];
            if (task != NULL) {
                LLOGV("<%s>:%d,free task[%d] = %p\n",__FUNCTION__, __LINE__, i, task);
                if(!task->b_done) {
                    curl_multi_remove_handle(thread->cm, task->curl);
                    curl_easy_cleanup(task->curl);
                }

                {
                    Mutex::Autolock autolock(task->lock);
                    del_http_task_l(task);
                }
                free(task);
                thread->task[i] = NULL;

            }
        }
        if(thread->notifyTask != NULL) {
            {
                Mutex::Autolock autolock((thread->notifyTask)->lock);
                del_http_task_l(thread->notifyTask);
            }
            free(thread->notifyTask);
            thread->notifyTask = NULL;
        }

        thread->mState = DL_STATE_INIT;
    }
    curl_multi_cleanup(thread->cm);
    curl_global_cleanup();

    free(thread);

}

int start_http_thread(http_thread * thread) {
    pthread_t pid;
    pthread_create(&pid, NULL, threadFunc2, thread);
    //success
    Mutex::Autolock threadLock(thread->mApiLock);
    thread->mStillRunning = true;
    thread->mState = DL_STATE_BUSY;
    return DL_OK;
}

int stop_http_thread(void *priv) {
    ENTER_FUNC;
    http_thread *thread = static_cast<http_thread *>(priv);
    if(thread == NULL) {
        return DL_INVAL_HANDLE;
    }
    Mutex::Autolock threadLock(thread->mApiLock);
    thread->mStillRunning = false;
    int ret = DL_OK;
    if(thread->mState == DL_STATE_BUSY) {
        //timeout nanosecond
        int errCode = (thread->stateConditon.waitRelative(thread->mApiLock,3000000000ll));
        if (errCode == -ETIMEDOUT) {

            LLOGE("%s:%d - waitRelative timeout:\n", __FUNCTION__, __LINE__);
            ret = DL_TIMEOUT;
        }
        thread->mState = DL_STATE_IDLE;
    }

    ESC_FUNC;
    return ret;
}

http_task *add_http_task(void* pThread,const char *url, int64_t start, int64_t size, int timeout, int32_t cid,  std::vector<std::string> &headerProps)
{
    ENTER_FUNC;

    http_thread *thread = static_cast<http_thread *>(pThread);
    LLOGD("[%d] %s url= %s start=%lld size = %lld %d",cid, __FUNCTION__, url, start, size,thread->task_index);
    if (!url || start < 0 || size <= 0) {
        ESC_FUNC;
        return NULL;
    }
    //if call the stop when starting; then don't allowed to add task.
    if((thread->mState == DL_STATE_BUSY) && !(thread->mStillRunning)) {
        ESC_FUNC;
        return NULL;
    }

    http_task *task = (http_task *)malloc(sizeof(http_task));
    memset(task, 0x0, sizeof(struct http_task));

    task->url = strdup(url);
    task->thread = thread;

    task->i_capacity = size;
    task->b_mallocbuffer = false;

    task->mConnId       = cid;
    task->i_timeout     = timeout;
    task->i_offset      = start;
    task->i_streamsize  = size;
    task->i_alreadydone = 0LL;
    task->i_readoffset  = task->i_writeoffset = 0;
    task->f_bandWidthBps = -1.0;

    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, task->url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_write_data);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curl_write_header);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, task);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, task);

    //liyl add  set the http protocol related properties
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 30L);



    //liyl set the progress callback

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, (curl_progress_callback ) on_progress_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);

    //liyl https, Skip certificate validation as default.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);




    //set header
    curl_slist_free_all(task->p_slist);
    task->p_slist= NULL;
    for (int i = 0; i < headerProps.size(); i++) {
        LLOGD("headers->items[%d] = %s",i,headerProps[i].c_str());
        task->p_slist = curl_slist_append(task->p_slist, headerProps[i].c_str());
    }
    if (task->p_slist != NULL) {
        CURLcode err = CURLE_OK;
        err = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, task->p_slist);
        if (err != CURLE_OK) {
            LLOGE("[%d] Curl curl_easy_setopt failed: %d---",task->mConnId, err);
        }
    }
    //TODO
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT_LIYL);
    // set the http range property
    char range[64] = { 0 };
    if (size != 0) {
        snprintf(range, 64, "%lld-%lld", start, start + size - 1);
    } /*else {
        strcpy(range, "0-");
        }*/
    if(curl_easy_setopt(curl, CURLOPT_RANGE, range) != CURLE_OK) {
        LLOGE("range set failed %s ,will be ignore",range);
    }


    CURLMcode errcode = curl_multi_add_handle(thread->cm, curl);
    CHECK(errcode == CURLM_OK); 

    task->curl = curl;
    task->b_done = false;
    task->b_forceexit = false;

    //locked
    {
        Mutex::Autolock threadLock(thread->mApiLock);
        thread->task[thread->task_index % 100] = task;
        task->i_index = thread->task_index;
        thread->task_index ++;
        for (int i = 0 ; i < 100; i++) {
            http_task *task = thread->task[i];
            if (task != NULL) {
                LLOGV("[%d] task[%d] forceexit %d, done %d\n",task->mConnId, i, task->b_forceexit, task->b_done);
            }
        }
    }

    long long absPosition = start;
    thread->onEventCB(task, thread->pContext, DL_ET_ADDTASK_CMPL,&absPosition, size);
    gettimeofday (&task->t_start, NULL);
    ESC_FUNC;
    return task;
}


int del_http_task(http_task *task) {

    LLOGD("[%d] %s %d",task!=NULL?task->mConnId:-1, __FUNCTION__,task!=NULL?task->i_index:-1);
    if (task) {
        Mutex::Autolock autolock(task->lock);
        del_http_task_l(task);
    }
    free(task);
    return DL_OK;
}


void del_http_task_l(http_task *task) {

    task->b_forceexit = true;
    //TODO need wait stopped
    if (task->p_slist) {
        curl_slist_free_all(task->p_slist);
        task->p_slist = NULL;
    }

    if (task->url) {
        free(task->url);
        task->url = NULL;
    }

    if (task->getUrl) {
        free(task->getUrl);
        task->getUrl = NULL;
    }

}






static int32_t curl_write_data(void * buffer, int32_t size2, int32_t nmemb, void * priv){

    ENTER_FUNC;
    http_task *task = static_cast<http_task *>(priv);
    int32_t size = nmemb * size2;

    struct timeval s;
    gettimeofday (&s, NULL);
    if(task != NULL) {
        Mutex::Autolock autoLock(task->lock);
        int diff = (s.tv_sec - task->t_start.tv_sec)*1000
            + (s.tv_usec - task->t_start.tv_usec)/1000;
        bool a = (diff > task->i_timeout);
        if (task->b_forceexit || a) {
            LLOGW("[%d] force exit %d, or download timeout(%d > %d ms) %d",
                    task->mConnId, task->b_forceexit, diff, task->i_timeout, a);
            ESC_FUNC;
            return 0;
        }

        task->i_alreadydone += size;
        http_thread * pThread = (http_thread*)(task->thread);
        pThread->i_filledsize += size;
        pThread->onEventCB(task, pThread->pContext, DL_ET_DATA,buffer, size);

    }
    ESC_FUNC;

    return  nmemb * size2;
}

static size_t curl_write_header(void *ptr, size_t size2, size_t nmemb,void *priv) {
    ENTER_FUNC;
    http_task *task = static_cast<http_task *>(priv);
    int32_t size = nmemb * size2;
    //Mutex::Autolock autoLock(task->lock);


    http_thread * pThread = (http_thread*)(task->thread);
    pThread->onEventCB(task, pThread->pContext, DL_ET_HEADER,ptr, size);
    ESC_FUNC;
    return size;
}

static void *threadFunc2(void *arg)
{
    pthread_detach(pthread_self());
    http_thread * thread = (http_thread *)arg;
    CURLM *cm = thread->cm;

    CURLMsg *msg;
    long L;
    int M, Q, U = -1;
    int firstRunning = 1;
    struct timeval T;

    //you receive CURLM_CALL_MULTI_PERFORM, this basically means that you should call curl_multi_perform again,
    curl_multi_perform(cm, &U);
    U = U > 0 ?U:firstRunning;//first perform maybe failed,U was upredictable;
    while (thread->mStillRunning && U > 0) {

        LLOGD("current running task is %d",U);
        fd_set R, W, E;
        FD_ZERO(&R);
        FD_ZERO(&W);
        FD_ZERO(&E);
        M = -1;

        curl_multi_fdset(cm, &R, &W, &E, &M);

        /*Not to exit thread loop temporary*/

        curl_multi_timeout(cm, &L);
        if (L == -1)
            L = 100;

        T.tv_sec = L/1000;
        T.tv_usec = (L%1000)*1000;

        int rc = select(M+1, &R, &W, &E, &T);
        switch(rc) {
            case -1:
                /* select error */
                //API locked
                {
                    Mutex::Autolock threadLock(thread->mApiLock);
                    thread->mStillRunning = 0;
                }
                LLOGD ("select() returns error !!\n");
                break;
            case 0:
            default:
                {
                    //LLOGD("<%s>:%d,rc=%d\n",__FILE__,__LINE__,rc);
                    curl_multi_perform(cm, &U);
                }
                break;
        }

        if(!thread->mStillRunning) break;
        while ((msg = curl_multi_info_read(cm, &Q))) {
            if(!thread->mStillRunning) { break;

            }
            if (msg->msg == CURLMSG_DONE) {

                CURL *e = msg->easy_handle;
                long code = 0;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);
                LLOGD("threadFunc msg ret: %d,%ld %d - %s <%p>\n",
                        msg->data.result, code, U, curl_easy_strerror(msg->data.result),e);

                double BandWidthBps = 0;
                // the average download speed that measured for the complete download, in bytes/second
                curl_easy_getinfo(msg->easy_handle, CURLINFO_SPEED_DOWNLOAD, &BandWidthBps);
                LLOGD("The download bitrate is %d kBs\n",(int32_t)(BandWidthBps /1024));
                for (int i = 0 ; i< 100; i++) {
                    if(!thread->mStillRunning) {
                        break;
                    }
                    http_task *task = thread->task[i];
                    if ((task != NULL) && task->curl == e) {
                        Mutex::Autolock threadLock(thread->mApiLock);//mutex for getDownload
                        {
                            Mutex::Autolock autoLock(task->lock);
                            bool a = (task->i_streamsize == task->i_alreadydone);
                            LLOGD("[%d]  thread->pContext[%p] thread->task[%02d], stream %-9lld, got %-9d B.%s",task->mConnId,thread->pContext, i, task->i_streamsize, task->i_alreadydone,a?"ok":"ng");
                            task->f_bandWidthBps = BandWidthBps;
                            task->i_code = code;
                            curl_multi_remove_handle(thread->cm, task->curl);
                            curl_easy_cleanup(task->curl);
                            int responseCode = convert_response_code( task,static_cast<int>(task->i_code));
                            if (!task->b_forceexit && responseCode == DL_OK && a) {
                                //TODO                               //sendMessage(task, 100);
                                thread->onEventCB(task, thread->pContext, DL_ET_SUBTASK_DATA_CMPL, (void*)NULL,0);
                            }else {

                                thread->onEventCB(task, thread->pContext, DL_ET_SUBTASK_ERR, (void*)&responseCode,0);
                                thread->hasDownloadError = true;
                            }
                            task->b_done = true;
                            del_http_task_l(task);
                        }
                        free(task);
                        thread->task[i] = NULL;
                        break;
                    }
                }
            } else {
                LLOGD( "E: CURLMsg (%d)\n", msg->msg);
            }
        }

    }

    LLOGD("<%s>:%d,exit done\n",__FILE__,__LINE__);
    //TODO notify download completely

    {

        Mutex::Autolock threadLock(thread->mApiLock);
        thread->mState = DL_STATE_IDLE;
        thread->stateConditon.broadcast();
        if(!(thread->hasDownloadError)) {
            thread->onEventCB(thread->notifyTask, thread->pContext, DL_ET_DATA_CMPL, (void*)NULL,0);
            thread->hasDownloadError = false;
        }
    }
    pthread_exit((void*)0);
    return NULL;
}


// Create the download instance handle
DLMultiHandle DL_CreateMultiHandle() {
    return init_http_thread();
}

// Close the download instance handle when it's never used
void DL_CloseMultiHandle(DLMultiHandle handle){
    http_thread* thread = (http_thread*) handle;
    deinit_http_thread(thread);

    thread = NULL;
}

// Close the download instance handle when it's never used
void DL_ResetMultiHandle(DLMultiHandle handle){
    //UNUSED
    return;
}



// prepare a download task
int DL_PrepareMultiDownload(DLMultiHandle handle, const char* strUrl, int64_t start, int64_t size, bool isAsync, MultiDownloadCBFunc pCBFunc, void* pContext,std::vector<std::string> &headerProps,int sessionTimeout)
{
    // add http_task
    if(handle == NULL) {
        LLOGE("handle is NULL");
        return DL_INVAL_HANDLE;
    }
    if(pCBFunc == NULL || pContext == NULL) {
        LLOGE("pCBFunc is NULL");
        return DL_INVAL_PARAM;
    }
    http_thread* thread = (http_thread*) handle;
    http_task *notifyTask = (http_task *)malloc(sizeof(http_task));
    if(notifyTask == NULL) {
        return DL_OUT_OF_MEMORY;
    }
    memset(notifyTask, 0x0, sizeof(struct http_task));
    notifyTask->url = strdup(strUrl);
    notifyTask->thread = thread;
    //notifyTask->mConnId= 0;
    notifyTask->i_index = NOTIFY_TASK_INDEX;
    {
        Mutex::Autolock threadLock(thread->mApiLock);
        thread->pContext = pContext;
        thread->onEventCB = pCBFunc;
        thread->ftbd_offset = start;
        thread->mState = DL_STATE_IDLE;
        thread->notifyTask = notifyTask;
    }
    int taskCount   = TASKCOUNTSMAX;
    if(thread->i_taskNumMax > 0) {
        taskCount = thread->i_taskNumMax;
    }
    int avgSubSize  = size/taskCount;
    int residueSize = size-(taskCount*avgSubSize);

    for(int i = 0;i < taskCount-1;i++) {

        add_http_task(thread,strUrl,start+i*avgSubSize,avgSubSize,sessionTimeout > 0? sessionTimeout:TIMEOUT_IN_MS,0/*cid*/,headerProps);
    }
    add_http_task(thread,strUrl,start+(taskCount-1)*avgSubSize,avgSubSize+residueSize,sessionTimeout > 0?sessionTimeout:TIMEOUT_IN_MS,0/*cid*/,headerProps);
    return DL_OK;
}

//start a download task
int DL_StartMultiDownload(DLMultiHandle handle,bool isAsync) {
    (void)isAsync;
    http_thread* thread = (http_thread*) handle;
    return start_http_thread(thread);
}

int DL_PauseMultiDownload(DLMultiHandle handle) {

    //UNUSED
    return DL_OK;
}

int DL_ResumeMultiDownload(DLMultiHandle handle){

    //UNUSED
    return DL_OK;
}

// Stop the current performing download task
int DL_StopMutiDownload(DLMultiHandle handle, int64_t nanoseconds/*UNUSED*/){
    (void)nanoseconds;
    http_thread* thread = (http_thread*) handle;

    return stop_http_thread(thread);
}




DLHandle DL_AddSingleTask(DLMultiHandle handle,const char *url, int64_t offset, int64_t size, int timeout, int32_t cid,std::vector<std::string> &headerProps) {

    return (DLHandle)(add_http_task(handle,url,offset,size,timeout,cid,headerProps));
}

int DL_DelSingleTask(DLMultiHandle multiHandel, DLHandle handle) {
    http_thread* thread = (http_thread*) multiHandel;
    http_task* task = (http_task*) handle;
    CHECK(thread == task->thread);

    if(thread->task[task->i_index] == task) {
        LLOGV("delete the task[%d]",task->i_index);
        {

            Mutex::Autolock threadLock(thread->mApiLock);
            if(!task->b_done) {
                curl_multi_remove_handle(thread->cm, task->curl);
                curl_easy_cleanup(task->curl);
            }


            {
                Mutex::Autolock autolock(task->lock);
                del_http_task_l(task);
            }
            thread->task[task->i_index] = NULL;
            free(task);
        }
        return DL_OK;

    }else {
        LLOGE("the goal was gone!!");
        return DL_INVAL_HANDLE;
    }


}



// Get the download related information
int DL_GetMultiDownloadInfo(DLMultiHandle handle, HTTPDownloadStatusInfo* info) {
    //UNUSED
    if((NULL == handle) ||( NULL == info)) {
        return DL_INVAL_PARAM;
    }

    int ret = getDownloadInfo(handle,  info);
    return ret;
}

// Convert the response code to the system defined error code by the network protocol
/*static*/
int convert_response_code(http_task* task,int code)
{
    LLOGD("responseCode = %d",code);
    int ret = DL_OK;
    switch(code) {
        case 400:
            ret = DL_EHTTP_BAD_REQUEST;
            break;
        case 401:
            ret = DL_EHTTP_UNAUTHORIZED;
            break;
        case 403:
            ret = DL_EHTTP_FORBIDDEN;
            break;
        case 404:
            ret = DL_EHTTP_NOT_FOUND;
            break;
        case 408:
            ret = DL_EHTTP_TIMEOUT;
            break;
        case 416:
            ret = DL_EHTTP_RANGE_ERROR;
            break;
        case 504:
            ret = DL_EHTTP_504;
            break;
        default: {
                     if (code >= 400 && code < 500) {
                         ret = DL_EHTTP_OTHER_4XX;
                         break;
                     }
                     if (code >= 500 && code < 600) {
                         ret = DL_EHTTP_SERVER_ERROR;
                         break;
                     }
                     break;
                 }
    }


    return ret;
}




// Libcurl progress report callback function
// invoked by the libcurl during download process
/*static*/
int on_progress_cb (http_task* task, double dltotal, double dlnow, double ultotal, double ulnow) {
    if (task == NULL) {
        LLOGE("task is NULL\n");
        return 0;
    }

    int64_t dlsize, ulsize;
    dlsize = (int64_t) dlnow;
    ulsize = (int64_t) ulnow;
    http_thread* thread = (http_thread*)(task->thread);
    // return the download stopping flag to terminate the download work of libcurl
    if ((thread == NULL) || (!thread->mStillRunning) ) {
        LLOGE("%s:%d - http_task :%p download stop now", __FUNCTION__, __LINE__,task);
        return 1;
    }



    Mutex::Autolock lock(thread->mInfoLock);
    struct timeval curr_time;
    uint64_t time_diff_ms;
    uint64_t dlsize_diff;
    gettimeofday(&curr_time, NULL);
    time_diff_ms = (curr_time.tv_sec - thread->progress_last_time.tv_sec) * 1000
        + (curr_time.tv_usec - thread->progress_last_time.tv_usec) / 1000;
    dlsize_diff = thread->i_filledsize - thread->progress_last_pos;
    if ((time_diff_ms > 500)) {
        thread->f_speed_KBps = dlsize_diff / time_diff_ms;
        thread->download_speed = thread->f_speed_KBps;
        thread->download_size = dlsize_diff;
        thread->progress_last_pos = thread->i_filledsize;
        thread->progress_last_time = curr_time;
        LLOGD("thread->f_speed_KBps = %f",thread->f_speed_KBps);
    }



    return 0;
}


// Get the download related information
int getDownloadInfo(void* handle, HTTPDownloadStatusInfo* info)
{
    if (NULL == handle) {
        LLOGE("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        return DL_INVAL_HANDLE;
    }

    if (NULL == info) {
        LLOGE("%s:%d invalid param", __FUNCTION__, __LINE__);
        return DL_INVAL_PARAM;
    }
    http_thread* thread = (http_thread*) handle;
    Mutex::Autolock threadLock(thread->mApiLock);
    Mutex::Autolock infoLock(thread->mInfoLock);

    CURLcode code;
    //get download speed
    thread->download_speed = 0;
    for (int i = 0 ; i< thread->i_taskNumMax; i++) {
        http_task *task = thread->task[i];
        if (task != NULL) {

            double speed;
            code = curl_easy_getinfo(task->curl, CURLINFO_SPEED_DOWNLOAD, &speed);
            if (CURLE_OK != code) {
                LLOGE("%s:%d - DLCurl:%p curl_easy_getinfo download speed error code=%d", __FUNCTION__, __LINE__,task,code);
            }else {
                thread->download_speed += speed;
            }



        }
    }

    info->http_code        = thread->http_code;              //response code for http protocol
    info->namelookup_time  = thread->namelookup_time;          //the time used for dns looking up
    info->redirect_time    = thread->redirect_time;          //the time used during host redirect
    info->connect_time     = thread->connect_time;          //the time used for build network connection
    info->appconnect_time  = thread->appconnect_time;          //the time used from the start until the SSL connect/handshake
    info->pretransfer_time = thread->pretransfer_time;      //the time used from the start until the file transfer is just about to begin
    info->starttransfer_time= thread->starttransfer_time;      //the time used from the start until the first byte is received
    info->download_size    = thread->download_size;          //the total download data in bytes
    info->download_speed   = thread->download_speed;          //the average download speed bytes/s
    info->content_length   = thread->content_length;          //the last responsed content-length for http protocol
    info->content_type     = strdup(thread->content_type.c_str());
    info->effective_url    = strdup(thread->effective_url.c_str());          //the last redirected url
    info->effective_ip     = strdup(thread->effective_ip.c_str());          //the last redirected remote IP address
    LLOGV("thread->download_speed = %lf",thread->download_speed);


    return DL_OK;
}

#undef waitRelative
