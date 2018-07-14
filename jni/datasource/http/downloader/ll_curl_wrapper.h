/*************************************************************************
  > File Name: ll_curl_wrapper.h
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 7年06月05日 星期一 14时26分02秒
 ************************************************************************/

#ifndef _LL_CURL_DL_H
#define _LL_CURL_DL_H
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <string>
#include <stddef.h>
#include <map>

#include "error.h"

extern "C" {
#include "../../../external/curl/include/curl/curl.h"
}


#include "dl_common_ext.h"


using namespace std;
USING_NAMESPACE_LIYL;
typedef void* (*DLThreadFunc)(void*);

// Macro definitions
#define CONNECTION_TIMEOUT 30
#define CONNECTION_RETRYCOUNT 3

typedef void* DLHandle;
typedef void (*DownloadCBFunc)(DLHandle handle/*DLCurl*/, void* pContext/*pAPP*/, int type, void* pData, int size);
// the real instance handle for the download instance
struct DLCurl
{
    // download thread
    pthread_t thread;

    DL_S64 pos;                // abs position
    DL_S64 offset;             // current offset 
    std::string last_url;
    std::string last_ip;
    // flags
    DL_BOOL stopped;             // running control & status
    DL_S32 protocol;                  // download protocol
    CURLcode net_error_code;
    // download callback function
    DownloadCBFunc onEventCB;
    DL_PTR pContext;
    // The below will not be reset after initialized
    CURL *curl;
    DL_State state;
    // system parameters
    DL_S64 tcpConnectTimeoutMs;
    DL_S64 tcpReadTimeoutMs;
    // user defined headers
    struct curl_slist *hhlist;
    struct curl_slist *resolve;
    // mutex & conditions
    Mutex apiMutex;
    Condition taskCond;
    Mutex taskMutex;
    //download progress related data
    struct timeval last_progress_time;
    struct timeval dl_start_time;
    // speed Bps
    DL_S32 speedBps;
    DL_S32 connectRetryCount;
    // buffers
    char error_string[CURL_ERROR_SIZE]; // error string buffer
};
// Buffer for transferring downloaded payload data to the outer program temporarily
#define BUFFER_SIZE_MAX CURL_MAX_WRITE_SIZE
#define BUFFER_SIZE_MIN 1024
// Create the download instance handle
DLHandle DL_CreateHandle();

// Close the download instance handle when it's never used
void DL_CloseHandle(DLHandle handle);

// Close the download instance handle when it's never used
void DL_ResetHandle(DLHandle handle);

// prepare a download task
int DL_PrepareDownload(DLHandle handle, const char* strUrl, int start, int size, bool isAsync, DownloadCBFunc pCBFunc, void* pContext, std::vector<std::string> &headerProps);

//start a download task
int DL_StartDownload(DLHandle handle,bool isAsync);

// Set the system parameter
int DL_SetParameter(DLHandle handle, int key, void* pData, int size);

// Get the system parameter
int DL_GetParameter(DLHandle handle, int key, void** ppData, int* size);

// Stop the current performing download task
int DL_StopDownload(DLHandle handle, int64_t nanoseconds=0);

// Get the download related information
int DL_GetDownloadInfo(DLHandle handle, HTTPDownloadStatusInfo* info);

int DL_GetDownloadSpeed(DLHandle handle,double* speed);
//Add host name to IP address resolves
int DL_AddResolves(DLHandle handle,const std::string &host, const std::vector<std::string> &ipList);

//set download speed (bytes/s)
int DL_SetDownloadSpeed(DLHandle handle, bool enable, int64_t low, int64_t high);

#endif //_LL_CURL_DL_H

