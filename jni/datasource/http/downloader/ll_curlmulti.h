#ifndef YK_MULTI_CURL_H_

#define YK_MULTI_CURL_H_


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
//#include <sys/types.h>

#include "error.h"

extern "C" {
#include "../../../external/curl/include/curl/curl.h"
}


#include "dl_common_ext.h"


#define TASKCOUNTSMAX 4

typedef void* DLMultiHandle;
typedef void* DLHandle;
// Download callback function prototype
typedef void (*MultiDownloadCBFunc)(DLHandle handle/*DLCurl*/, void* pContext/*pAPP*/, int type, void* pData , int size);


typedef struct http_task{
    CURL *curl;
    void *thread;

    char *url;
    char *getUrl;

    bool b_done;
    bool b_chunked;
    bool b_forceexit;
    bool b_mallocbuffer;

    int i_code;
    int i_capacity;
    int64_t i_offset;
    int64_t i_streamsize;
    int i_readoffset;
    int i_writeoffset;
    int i_alreadydone;
    int i_timeout;
    int i_index;

    double f_bandWidthBps;
    int32_t mConnId;
    struct timeval t_start;
    struct curl_slist *p_slist;

    mutable liyl::Mutex lock;
} http_task;

#define NOTIFY_TASK_INDEX  101
typedef struct http_thread{
    CURLM *cm;
    unsigned task_index;
    int64_t ftbd_offset;
    DL_State mState;
    struct http_task *task[100];
    struct http_task *notifyTask;
    DataSpec* dataSpec;
    int32_t i_taskNumMax;
    bool mStillRunning;
    MultiDownloadCBFunc onEventCB;
    void* pContext;
    mutable liyl::Mutex mApiLock;
    mutable liyl::Condition stateConditon;
    bool hasDownloadError;

    //speed compute
    int64_t i_filledsize;
    double  f_speed_KBps;
    struct timeval progress_last_time;
    int64_t progress_last_pos;

    mutable liyl::Mutex mInfoLock;
    long long http_code;                //response code for http protocol
    double namelookup_time;            //the time used for dns looking up
    double redirect_time;            //the time used during host redirect
    double connect_time;            //the time used for build network connection
    double appconnect_time;            //the time used from the start until the SSL connect/handshake
    double pretransfer_time;        //the time used from the start until the file transfer is just about to begin
    double starttransfer_time;        //the time used from the start until the first byte is received
    double download_size;            //the total download data in bytes
    double download_speed;            //the average download speed bytes/s
    std::string effective_url;            //the last redirected url
    std::string effective_ip;            //the last redirected remote IP address
    double content_length;            //the last responsed content-length for http protocol
    std::string  content_type;
    int    last_phase_code;          //http session process
    int    action_id;                //http session process err

} http_thread;



void *init_http_thread();
void deinit_http_thread(http_thread* thread);
int start_http_thread(http_thread* thread);
int stop_http_thread(void *thread);

http_task *add_http_task(void* pThread,const char *url, int64_t startoffset, int64_t size,int timeout_in_ms, int32_t cid, std::vector<std::string> &headerProps);

int del_http_task(http_task *task);
void del_http_task_l(http_task *task);
int getDownloadInfo(void* handle,HTTPDownloadStatusInfo* info);

static void *threadFunc2(void *arg);

static size_t curl_write_header(void *ptr, size_t size, size_t nmemb,void *priv);

static int32_t curl_write_data(void * buffer, int32_t size2, int32_t nmemb, void * priv);

static int on_progress_cb (http_task* task, double dltotal, double dlnow, double ultotal, double ulnow);


static int convert_response_code(http_task* task,int code);

// Create the download instance handle
DLMultiHandle DL_CreateMultiHandle();

// Close the download instance handle when it's never used
void DL_CloseMultiHandle(DLMultiHandle handle);

// reset the download instance handle when it's reused
void DL_ResetMultiHandle(DLMultiHandle handle);


// prepare a download task
int DL_PrepareMultiDownload(DLMultiHandle handle, const char* strUrl, int64_t startoffset, int64_t size, bool isAsync, MultiDownloadCBFunc pCBFunc, void* pContext, std::vector<std::string> &headerProps,int sessionTimout);

//start a download task
int DL_StartMultiDownload(DLMultiHandle handle,bool isAsync);

int DL_PauseMultiDownload(DLMultiHandle handle);

int DL_ResumeMultiDownload(DLMultiHandle handle);

// Stop the current performing download task
int DL_StopMutiDownload(DLMultiHandle handle, int64_t nanoseconds=0);



DLHandle DL_AddSingleTask(DLMultiHandle handle,const char *url, int64_t offset, int64_t size, int timeout, int32_t cid, std::vector<std::string> &headerProps);

int DL_DelSingleTask(DLMultiHandle multiHandle,DLHandle handle);

// Get the download related information
int DL_GetMultiDownloadInfo(DLMultiHandle handle, HTTPDownloadStatusInfo* info);



#endif //YK_MULTI_CURL_H_
