/*************************************************************************
  > File Name: ll_curl_wrapper.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com
  > Created Time: 2017年06月05日 星期一 17时38分38秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <algorithm>
#include "dl_log.h"
#include "ll_curl_utils.h"
#include "ll_curl_wrapper.h"
#include <vector>

using std::vector;

#undef LOG_TAG
#define LOG_TAG "DL_CURL"

// Global initialization for download module

static std::map<std::string,std::string>& getDL_Protocol() {

    //C++ standard : don't delete local static
    static std::map<std::string, std::string>* p_DL_Protocol = new std::map<std::string,std::string>();
    return *p_DL_Protocol;
}
static void setupDefaultPort()
{
    std::map<std::string, std::string>& DL_Protocol = getDL_Protocol();
    DL_Protocol["http"] = "80";
    DL_Protocol["https"] = "443";
}
static int global_init_ok = 0;
class Global_init
{
    public:
        Global_init()
        {
            ENTER_FUNC;
            CURLcode ret =curl_global_init(CURL_GLOBAL_ALL);
            dl_openssl_thread_setup();
            setupDefaultPort();
            if (ret == CURLE_OK) {
                global_init_ok = 1;
                dl_log_info("%s\n", curl_version());
            } else {
                dl_log_err("curl_global_init: %s\n", curl_easy_strerror(ret));
            }
            ESC_FUNC;
        }
        ~Global_init()
        {
            ENTER_FUNC;
            dl_openssl_thread_cleanup();
            curl_global_cleanup();
            global_init_ok = 0;
            ESC_FUNC;
        }
};
static Global_init global_init;




// reset the download instance data before the new task
static void reset_handle(DLCurl* dlcurl)
{
    if (dlcurl == NULL) {
        dl_log_all("%s:%d dlcurl is NULL", __FUNCTION__, __LINE__);
        return;
    }
    dlcurl->thread = 0;
    dlcurl->speedBps = 0l;
    dlcurl->pos = 0ll;
    dlcurl->offset = 0ll;
    dlcurl->connectRetryCount = 0l;
    dlcurl->stopped = DL_FALSE;
    dlcurl->protocol = 0;
    dlcurl->net_error_code = CURLE_OK;
    dlcurl->onEventCB = NULL;
    dlcurl->pContext = NULL;
    dlcurl->last_url.clear();
    dlcurl->last_ip.clear();
    memset(&dlcurl->last_progress_time, 0, sizeof(dlcurl->last_progress_time));
    memset(&dlcurl->dl_start_time, 0, sizeof(dlcurl->dl_start_time));

}

// Convert the response code to the system defined error code by the network protocol
static int convert_response_code(DLCurl* dlcurl,int code)
{
    int ret = DL_OK;
    if (code == CURLE_OK) {
        long response;
        // Get the http response code
        curl_easy_getinfo(dlcurl->curl, CURLINFO_RESPONSE_CODE, &response);

        switch(response) {
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
                         if (response >= 400 && response < 500) {
                             ret = DL_EHTTP_OTHER_4XX;
                             break;
                         }
                         if (response >= 500 && response < 600) {
                             ret = DL_EHTTP_SERVER_ERROR;
                             break;
                         }
                         break;
                     }
        }
    } else {
        switch(code) {
            case CURLE_URL_MALFORMAT:
                ret = DL_INVALID_URL;
                break;
            case CURLE_COULDNT_CONNECT:
                ret = DL_ETCP_CONN_FAILED;
                break;
            case CURLE_OPERATION_TIMEDOUT:
                ret = DL_TIMEOUT;
                break;
            case CURLE_TOO_MANY_REDIRECTS:
                ret = DL_EHTTP_OVER_REDIRECT;
                break;
            case CURLE_ABORTED_BY_CALLBACK:
                ret = DL_USER_CANCEL;
                break;
            default:
                ret = code;
                break;
        }
    }

    return ret;
}

static bool reConnect(DLCurl *dlcurl, int response)
{
    bool doConn = false;
    if(dlcurl->stopped == DL_FALSE) {
        if(response == CURLE_COULDNT_CONNECT
                || response == DL_TIMEOUT){
            if(!dlcurl->last_ip.empty()){ 
                dl_log_debug("%s:%d DLCurl:%p before onEventCB()",__FUNCTION__,__LINE__,dlcurl);
                dlcurl->onEventCB(dlcurl, dlcurl->pContext, DL_ET_HTTPDNS,(void*)(dlcurl->last_ip.c_str()), dlcurl->last_ip.size());
                dl_log_debug("%s:%d DLCurl:%p after onEventCB()",__FUNCTION__,__LINE__,dlcurl);
                dl_log_all("%s:%d - DLCurl:%p invalid ip:%s ",__FUNCTION__,__LINE__,dlcurl,dlcurl->last_ip.c_str());
            }
            if(dlcurl->connectRetryCount < CONNECTION_RETRYCOUNT) {
                dlcurl->connectRetryCount++;
                doConn = true;
            }else {
                dlcurl->connectRetryCount = 0;
            }
        }
    }

    return doConn;
}
static int dl_curl_perform(DLCurl *dlcurl)
{
    int response = 0;
    do {
        // perform the download task
        dl_log_all("%s:%d - DLCurl:%p curl_easy_perform start", __FUNCTION__, __LINE__,dlcurl);
        CURLcode code = curl_easy_perform(dlcurl->curl);
        dl_log_all("%s:%d - DLCurl:%p curl_easy_perform end", __FUNCTION__, __LINE__,dlcurl);

        dlcurl->net_error_code = code;
        // Convert the responsed code to the system defined error code
        response = convert_response_code(dlcurl, code);

        //get effective url
        char * pChar = NULL;
        code = curl_easy_getinfo(dlcurl->curl, CURLINFO_EFFECTIVE_URL, &pChar);
        if (pChar) {
            dlcurl->last_url = pChar;
        }
        if (CURLE_OK != code) {
            dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo redirect ip error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
        }

        char * pIP = NULL;
        code = curl_easy_getinfo(dlcurl->curl, CURLINFO_PRIMARY_IP, &pIP);
        if (pIP) {
            dlcurl->last_ip = pIP;
        }
        if (CURLE_OK != code) {
            dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo redirect ip error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
        }

        if(reConnect(dlcurl,response))
            continue;

        break;
    }while(1);

    return response;
}
// the download task worker routine thread
static void*  dl_curl_thread (void * handle)
{
    DLCurl *dlcurl = (DLCurl*) handle;
    int response = dl_curl_perform(dlcurl);

    if (response == DL_OK) {
        // TODO need fixed
        // When the download task completed successfully,
        // send back the remained payload data to the outer program
        dlcurl->onEventCB(dlcurl, dlcurl->pContext, DL_ET_DATA_CMPL, (void*)NULL,0);
    }

    // Send the error code to the outer program
    dl_log_debug("%s:%d DLCurl:%p before onEventCB()",__FUNCTION__,__LINE__,dlcurl);
    dlcurl->onEventCB(dlcurl, dlcurl->pContext, DL_ET_ERR, &response, sizeof(response));
    dl_log_debug("%s:%d DLCurl:%p after onEventCB()",__FUNCTION__,__LINE__,dlcurl);

    // It's time to change the download task status to idle
    // and send the task finished signal
    {
        liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
        dlcurl->state = DL_STATE_IDLE;
        dlcurl->taskCond.broadcast();
        dl_log_all("%s:%d - DLCurl:%p aready download %d Byte dlcurl->taskCond.signalAll() sended", __FUNCTION__, __LINE__,dlcurl,(int)(dlcurl->pos));
    }
    //liyl add

    pthread_exit((void*)0);

    return (void*)(long)response;
}




// Payload data arrived callback function
// transfer the data to the outer program
static size_t dl_curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    DLCurl* dlcurl = (DLCurl*) userdata;
    //liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
    if (dlcurl->stopped == DL_TRUE) {
        dl_log_info("dlcurl was stopped)\n");
        return 0;
    }

    dlcurl->onEventCB(dlcurl, dlcurl->pContext, DL_ET_DATA,ptr, realsize);

    {
        dlcurl->pos += realsize;
        dlcurl->offset += realsize;
    }
    return realsize;
}
// Libcurl progress report callback function
// invoked by the libcurl during download process
static int dl_curl_progress_cb (DLCurl* dlcurl, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if (dlcurl == NULL) {
        dl_log_err("dlcurl is NULL\n");
        return 0;
    }
    int64_t dlsize, ulsize;
    dlsize = (int64_t) dlnow;
    ulsize = (int64_t) ulnow;
    {
        struct timeval curr_time;
        uint64_t time_diff_ms;
        gettimeofday(&curr_time, NULL);
        time_diff_ms = (curr_time.tv_sec - dlcurl->last_progress_time.tv_sec) * 1000
            + (curr_time.tv_usec - dlcurl->last_progress_time.tv_usec) / 1000;

        if (time_diff_ms > 1000) {
            dl_log_debug("%s:%d - DLCurl:%p current position:%d", __FUNCTION__, __LINE__, dlcurl, (int )dlcurl->pos);
            dlcurl->last_progress_time = curr_time;
        }

    }


    // return the download stopping flag to terminate the download work of libcurl
    if (dlcurl->stopped == DL_TRUE) {
        dl_log_all("%s:%d - DLCurl:%p download stop now", __FUNCTION__, __LINE__,dlcurl);
        return 1;
    }


    return 0;
}



// Libcurl debugging callback
static int dl_curl_debug_cb(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    const char *text;
    (void) handle; /* prevent compiler warning */
    DLCurl* dlcurl = (DLCurl*) userp;

    if (dlcurl == NULL)
        return 0;

    if (dlcurl->stopped == DL_TRUE)
        return 0;


    switch(type) {
        case CURLINFO_TEXT:
            dl_log_info("== Info: %s", data);
        default: /* in case a new one is introduced to shock us */
            return 0;

        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
    }

    if (type == CURLINFO_DATA_IN || type == CURLINFO_DATA_OUT || type == CURLINFO_SSL_DATA_IN
            || type == CURLINFO_SSL_DATA_OUT) {

    }

    // dump the data to screen

    LLOGD("DLCurl %p %s %s",dlcurl,__func__,text);
    dlcurl->onEventCB(dlcurl, dlcurl->pContext, DL_ET_CURL_DEBUG,NULL/*UNUSED*/ ,0/*UNUSED*/ );
    return 0;
}

// Header data arrived callback
static size_t dl_curl_header_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    DLCurl* dlcurl = (DLCurl*) userdata;
    //liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
    int linesize = nmemb * size;
    if (dlcurl->stopped == DL_TRUE) {
        dl_log_err("dlcurl was stopped)\n");
        return 0;
    }
    dlcurl->onEventCB(dlcurl, dlcurl->pContext, DL_ET_HEADER, ptr, linesize);

    return linesize;

}


// Create the download instance handle
DLHandle DL_CreateHandle()
{
    ENTER_FUNC;
    DLCurl* dlcurl = NULL;

    if (global_init_ok) {
        dlcurl = new (std::nothrow) DLCurl;
        if (dlcurl != NULL) {
            // reset the download instance data
            reset_handle(dlcurl);

            // initialize the download instance data
            dlcurl->stopped = DL_FALSE;
            dlcurl->state  = DL_STATE_INIT;
            dlcurl->hhlist = NULL;
            dlcurl->resolve = NULL;
            dlcurl->curl = curl_easy_init();

            if (dlcurl->curl != NULL) {
                // set the error string buffer
                curl_easy_setopt(dlcurl->curl, CURLOPT_ERRORBUFFER, dlcurl->error_string);
            } else {
                delete (dlcurl);
                dlcurl = NULL;
            }
        }
    }
    dl_log_all("%s:%d - DLCurl:%p create download handle", __FUNCTION__, __LINE__,dlcurl);

    ESC_FUNC;
    return dlcurl;
}

// Close the download instance handle when it's never used
void DL_CloseHandle(DLHandle handle)
{
    if (handle == NULL) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        return;
    }

    DLCurl* dlcurl = (DLCurl*) handle;

    {
        dl_log_all("%s:%d - DLCurl:%p get apiMutex start", __FUNCTION__, __LINE__,dlcurl);
        liyl::Mutex::Autolock lock(dlcurl->apiMutex);
        dl_log_all("%s:%d - DLCurl:%p get apiMutex end", __FUNCTION__, __LINE__,dlcurl);

        // Send signal to libcurl stopping current downloading work
        dlcurl->stopped = DL_TRUE;

        {
            liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
            if (dlcurl->state == DL_STATE_BUSY) {
                // wait for the current running download thread to exit
                dl_log_all("%s:%d - DLCurl:%p dlcurl->taskCond.wait start", __FUNCTION__, __LINE__,dlcurl);
                dlcurl->taskCond.wait(dlcurl->apiMutex);
                dlcurl->state = DL_STATE_IDLE;
                dl_log_all("%s:%d - DLCurl:%p dlcurl->taskCond.wait end", __FUNCTION__, __LINE__,dlcurl);
            }
            dlcurl->state = DL_STATE_INIT;
        }

        if (dlcurl->curl) {
            // clean up the libcurl instance object
            curl_easy_cleanup(dlcurl->curl);
            dlcurl->curl = NULL;
        }

        curl_slist_free_all(dlcurl->hhlist);
        curl_slist_free_all(dlcurl->resolve);
    }
    dl_log_all("%s:%d close OK", __FUNCTION__, __LINE__);
    delete (dlcurl);
    dlcurl = NULL;


}

void DL_ResetHandle(DLHandle handle)
{
    ENTER_FUNC;
    if (handle == NULL) {
        dl_log_err("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        ESC_FUNC;
        return ;
    }
    DLCurl* dlcurl = (DLCurl*) handle;
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);
    reset_handle(dlcurl);
    ESC_FUNC;


}

// Commit a download task
int DL_PrepareDownload(DLHandle handle, const char* strUrl, int start, int size, bool isAsync, DownloadCBFunc pCBFunc, void* pContext, std::vector<std::string> &headerProps)
{
    ENTER_FUNC;
    if (handle == NULL) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        ESC_FUNC;
        return DL_INVAL_HANDLE;
    }
    if (pCBFunc == NULL) {
        dl_log_all("%s:%d pCBFunc is NULL", __FUNCTION__, __LINE__);
        ESC_FUNC;
        return DL_INVAL_PARAM;
    }

    DLCurl* dlcurl = (DLCurl*) handle;
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);

    {
        liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
        if (dlcurl->state == DL_STATE_IDLE) {
            dl_log_all("%s:%d - DLCurl:%p handle busy: a download will prepare again ,start = %d size=%d ", __FUNCTION__, __LINE__,dlcurl,start,size);
            //return DL_ALREADY_PREPARED;
        }else {
            dlcurl->state = DL_STATE_IDLE;
        }
    }

    int curlRet = DL_OK;
    CURL* curl = dlcurl->curl;

    // reset the download instance data before a new download task started
    reset_handle(dlcurl);

    dlcurl->pos = start;

    // set the payload data callback
    dlcurl->onEventCB = pCBFunc;
    dlcurl->pContext = pContext;

    curl_easy_reset(dlcurl->curl);
    curl_easy_setopt(dlcurl->curl, CURLOPT_URL, strUrl);

    //set http proxy
    const char *proxy_path = getenv("http_proxy");

    dl_log_debug("proxy_path = %s",proxy_path);
    bool use_proxy = (proxy_path != NULL) && !getenv("no_proxy") && strncasecmp(proxy_path, "http://",7);
    if(use_proxy) {
        dl_log_debug("proxy_path = %s",proxy_path);
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_path);
    }



    // set the progress callback
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, (curl_progress_callback ) dl_curl_progress_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, dlcurl);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);

    // set print header. warning: if CURLOPT_HEADER is true,then the header will not callback
    //curl_easy_setopt(curl, CURLOPT_HEADER, true);

    // set the header callback
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (curl_write_callback ) dl_curl_header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, dlcurl);

    // set the payload callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, dlcurl);

    // set the http protocol related properties
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 30L);

    // set the http range property
    char range[64] = { 0 };
    if (size != 0) {
        snprintf(range, 63, "%d-%d", start, start + size - 1);
    } else {
        strcpy(range, "0-");
    }

    dl_log_all("%s:%d http range: %s",__func__,__LINE__,range);
    if(curl_easy_setopt(curl, CURLOPT_RANGE, range) != CURLE_OK) {
        dl_log_err("range set failed %s ,will be ignore",range);
    }
    // set the user defined headers
    curl_slist_free_all(dlcurl->hhlist);
    dlcurl->hhlist = NULL;
    for (unsigned int i = 0; i < headerProps.size(); i++) {
        dlcurl->hhlist = curl_slist_append(dlcurl->hhlist, headerProps[i].c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, dlcurl->hhlist);

    // set hostname and IP address resolves
    dlcurl->last_url = strUrl;

    // debugging related flags
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, dl_curl_debug_cb);
    // curl_easy_setopt(curl, CURLOPT_DEBUGDATA, dlcurl);

    // use with CURLINFO_FILETIME
    curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

    // connection timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,CONNECTION_TIMEOUT);

    //https, Skip certificate validation as default.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    gettimeofday(&dlcurl->dl_start_time, 0);

    ESC_FUNC;
    return curlRet;

}

int DL_StartDownload(DLHandle handle ,bool isAsync) {
    ENTER_FUNC;
    if (handle == NULL) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        ESC_FUNC;
        return DL_INVAL_HANDLE;
    }

    DLCurl* dlcurl = (DLCurl*) handle;
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);

    {
        liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
        if (dlcurl->state == DL_STATE_BUSY) {
            dl_log_all("%s:%d - DLCurl:%p handle busy: a download is doing now", __FUNCTION__, __LINE__,dlcurl);
            ESC_FUNC;
            return DL_ALREADY_STARTED;
        }else {
            dlcurl->state = DL_STATE_BUSY;
        }
    }

    int curlRet = DL_OK;
    dl_log_all("%s:%d - DLHandle:%p start download url=%s", __FUNCTION__, __LINE__,dlcurl, dlcurl->last_url.c_str());
    if (isAsync) {
        pthread_create(&dlcurl->thread, NULL,dl_curl_thread, dlcurl);
        pthread_detach(dlcurl->thread);
    } else {
        curlRet =(long) dl_curl_thread(dlcurl);
    }
    ESC_FUNC;
    return curlRet;
}


int DL_PauseDownload(DLHandle handle) {
    if (handle == NULL) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        return DL_INVAL_HANDLE;
    }

    DLCurl* dlcurl = (DLCurl*) handle;
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);
    curl_easy_pause(dlcurl->curl,CURLPAUSE_RECV);
    int curlRet = DL_OK;
    dl_log_all("%s:%d - DLHandle:%p pause download url=%s", __FUNCTION__, __LINE__,dlcurl, dlcurl->last_url.c_str());
    //TODO
    return curlRet;
}

int DL_ResumeDownload(DLHandle handle) {
    if (handle == NULL) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        return DL_INVAL_HANDLE;
    }

    DLCurl* dlcurl = (DLCurl*) handle;
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);
    curl_easy_pause(dlcurl->curl,CURLPAUSE_RECV_CONT);
    int curlRet = DL_OK;
    dl_log_all("%s:%d - DLHandle:%p start download url=%s", __FUNCTION__, __LINE__,dlcurl, dlcurl->last_url.c_str());
    //TODO

    return curlRet;
}

// Set the system parameter
int DL_SetParameter(DLHandle handle, int key, void* pData, int size)
{
    return DL_OK;
}

// Get the system parameter
int DL_GetParameter(DLHandle handle, int key, void** ppData, int* size)
{ 
    return DL_OK;
}

// Stop the current performing download task
int DL_StopDownload(DLHandle handle, int64_t nanoseconds)
{
    ENTER_FUNC;
    int ret = 0;
    if (NULL == handle) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        ESC_FUNC;
        return DL_INVAL_HANDLE;
    }

    DLCurl * dlcurl = (DLCurl *) handle;
    dl_log_all("%s:%d - DLCurl:%p get apiMutex start", __FUNCTION__, __LINE__,dlcurl);
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);
    dl_log_all("%s:%d - DLCurl:%p get apiMutex end", __FUNCTION__, __LINE__,dlcurl);

    // send signal to libcurl stopping the current download work
    dl_log_all("%s:%d - DLCurl:%p stop download, set dlcurl->stopped=true", __FUNCTION__, __LINE__,dlcurl);
    dlcurl->stopped = DL_TRUE;

    {
        // wait for the running download work to be finished
        liyl::Mutex::Autolock lockTask(dlcurl->taskMutex);
        if (dlcurl->state == DL_STATE_BUSY) {
            if (nanoseconds == 0) {
                dl_log_all("%s:%d - DLCurl:%p dlcurl->taskCond.wait start", __FUNCTION__, __LINE__,dlcurl);
                ret = dlcurl->taskCond.wait(dlcurl->taskMutex);
                dl_log_all("%s:%d - DLCurl:%p dlcurl->taskCond.wait end", __FUNCTION__, __LINE__,dlcurl);
            } else {
                //FIXED ME
                dl_log_all("%s:%d - DLCurl:%p dlcurl->taskCond.waitRelative start", __FUNCTION__, __LINE__,dlcurl);
                ret = dlcurl->taskCond.waitRelative(dlcurl->taskMutex, nanoseconds);
                dl_log_all("%s:%d - DLCurl:%p dlcurl->taskCond.waitRelative end", __FUNCTION__, __LINE__,dlcurl);
                if (ret == -ETIMEDOUT) {
                    int log_seconds = nanoseconds / (1000*1000*1000);
                    dl_log_all("%s:%d - DLCurl:%p waitRelative() %d seconds timeout", __FUNCTION__, __LINE__, dlcurl,log_seconds);
                    ret = DL_TIMEOUT;
                }
            }
            dlcurl->state = DL_STATE_IDLE; //liyl add
        }
    }

    ESC_FUNC;
    return ret;
}

// Get the download related information
int DL_GetDownloadInfo(DLHandle handle, HTTPDownloadStatusInfo* info)
{
    ENTER_FUNC;
    if (NULL == handle) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        return DL_INVAL_HANDLE;
    }

    if (NULL == info) {
        dl_log_all("%s:%d invalid param", __FUNCTION__, __LINE__);
        return DL_INVAL_PARAM;
    }

    DLCurl * dlcurl = (DLCurl*) handle;
    liyl::Mutex::Autolock lock(dlcurl->apiMutex);
    CURLcode code;
    //get http code
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_RESPONSE_CODE, &(info->http_code));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo http_code error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get name_lookup time
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_NAMELOOKUP_TIME, &(info->namelookup_time));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo name_lookup time error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get redirect time
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_REDIRECT_TIME, &(info->redirect_time));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo redirect time error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get connect time
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_CONNECT_TIME, &(info->connect_time));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo connect error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get appconnect time
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_APPCONNECT_TIME, &(info->appconnect_time));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo appconnect error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get pretransfer time
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_PRETRANSFER_TIME, &(info->pretransfer_time));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo pretransfer error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get starttransfer time
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_STARTTRANSFER_TIME, &(info->starttransfer_time));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo starttransfer error code=%d", __FUNCTION__, __LINE__,dlcurl, code);
    }

    //get download size
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_SIZE_DOWNLOAD, &(info->download_size));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo download size error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get download speed
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_SPEED_DOWNLOAD, &(info->download_speed));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo download speed error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }else {
        dl_log_all("%d - DLCurl:%p curl_easy_getinfo download speed %d", __LINE__, dlcurl,static_cast<int>(info->download_speed));
    }

    //get effective url
    char * pUrl = NULL;
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_EFFECTIVE_URL, &pUrl);
    if (pUrl != NULL) {
        info->effective_url = strdup(pUrl);
    }
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo redirect url error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get primary ip address
    char * pIP = NULL;
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_PRIMARY_IP, &pIP);
    if (pIP != NULL) {
        info->effective_ip = strdup(pIP);
    }
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo redirect ip error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get content length
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &(info->content_length));
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo content length error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    //get content type
    char * pType = NULL;
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_CONTENT_TYPE, &pType);

    if (pType != NULL) {
        info->content_type = strdup(pType);
    }

    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo content type error code=%d", __FUNCTION__, __LINE__, dlcurl,code);
    }

    ESC_FUNC;
    return DL_OK;
}

// Get the download speed
int DL_GetDownloadSpeed(DLHandle handle, double * speed)
{
    if (NULL == handle) {
        dl_log_all("%s:%d handle is NULL", __FUNCTION__, __LINE__);
        return DL_INVAL_HANDLE;
    }

    if (NULL == speed) {
        dl_log_all("%s:%d invalid param", __FUNCTION__, __LINE__);
        return DL_INVAL_PARAM;
    }

    DLCurl * dlcurl = (DLCurl*) handle;
    CURLcode code;

    liyl::Mutex::Autolock lock(dlcurl->apiMutex);
    //get download speed
    code = curl_easy_getinfo(dlcurl->curl, CURLINFO_SPEED_DOWNLOAD, speed);
    if (CURLE_OK != code) {
        dl_log_all("%s:%d - DLCurl:%p curl_easy_getinfo download speed error code=%d", __FUNCTION__, __LINE__,dlcurl, code);
    }else {
        dl_log_all("%d - DLCurl:%p curl_easy_getinfo download speed %d", __LINE__, dlcurl,static_cast<int>(*speed));
    }

    return DL_OK;
}



//Add host name to IP address resolves ,NOTE must be before DL_STATE_BUSY
//dlcurl->hostIps will be cleared if host.empty() is true
//hostname will be removed from dlcurl->hostIps if ipList.empty() is true
int DL_AddResolves(DLHandle handle, const std::string &hostname,const std::vector<std::string> &ipList) {

    ENTER_FUNC;
    ESC_FUNC;
    return DL_INVAL_HANDLE;

}

//set download speed (bytes/s)
int DL_SetDownloadSpeed(DLHandle handle, bool enable, int64_t low, int64_t high)
{
    ENTER_FUNC;
    dl_log_all("%s:%d - DLHandle:%p DL_SetDownloadSpeed(%p,%d,%d,%d)", __FUNCTION__, __LINE__,handle,handle,enable,(int)low,(int)high);

    ESC_FUNC;
    return DL_INVAL_HANDLE;
}

