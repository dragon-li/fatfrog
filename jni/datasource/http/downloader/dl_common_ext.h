/*************************************************************************
  > File Name: dl_common_ext.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年06月01日 星期四 17时47分41秒
 ************************************************************************/

#ifndef DL_COMMON_EXT_H 
#define DL_COMMON_EXT_H 

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    // use this type to return error codes
#ifdef _WIN32
    typedef int         status_t;
#else
#include <stdint.h>
    typedef int32_t     status_t;
#endif

#ifndef LIYL_DOWNLOAD_TYPE
#define LIYL_DOWNLOAD_TYPE 1

#define DL_IN
#define DL_OUT
#define node_id int32_t 
#define buffer_id void*



#endif

#define FLAG_ALLOW_GZIP 0x00000001
#define FLAG_ALLOW_CACHING_UNKNOWN_LENGTH 0x00000010
#define FLAG_SWITCH_MULTI_DOWNLOADER 0x00000100

#include "dl_type.h"

    typedef  enum DL_HTTP_SESSION_STATUS {

        OPEN   = 1<<0,
        DNSR   = 1<<1,
        CONNECT= 1<<2,
        REQSND = 1<<3,
        HEADRCV= 1<<4,
        DATARCV= 1<<5,
        CLOSE  = 1<<6,

        EOPEN   = 1<<16,
        EDNSR   = 1<<17,
        ECONNECT= 1<<18,
        EREQSND = 1<<19,
        EHEADRCV= 1<<20,
        ECLOSE  = 1<<21,

    } DL_HTTP_SESSION_STATUS;

    // Download related information data structure
    typedef struct HTTPDownloadStatusInfo
    {
        DL_S64 http_code;                 //response code for http protocol
        DL_S64 namelookup_time;           //the time used for dns looking up
        DL_S64 redirect_time;             //the time used during host redirect
        DL_S64 connect_time;              //the time used for build network connection
        DL_S64 appconnect_time;           //the time used from the start until the SSL connect/handshake
        DL_S64 pretransfer_time;          //the time used from the start until the file transfer is just about to begin
        DL_S64 starttransfer_time;        //the time used from the start until the first byte is received
        DL_S64 download_size;             //the total download data in bytes
        DL_S64 download_speed;            //the average download speed bytes/s
        DL_CONST_STRING effective_url;    //the last redirected url
        DL_CONST_STRING effective_ip;     //the last redirected remote IP address
        DL_S64 content_length;            //the last responsed content-length for http protocol
        DL_CONST_STRING content_type;     //the last responsed content-type for http protocol

        //detail WRAPPERCurl.debug_data
        DL_S64 last_phase_code;           //the last phase code of http session 
        DL_S64 last_tcp_errorCode;        //the last error code of tcp session 

    } HTTPDownloadStatusInfo;



    typedef struct HttpHeaderResponse {
        DL_STRING url;

        DL_S32 lineNum;
        /**
         ** http header response string.
         **/
        DL_STRING headerLines[64];
        /**
         ** The http body length.
         **/
        DL_S64 contentLength;
        /**
         ** response code 
         ***/
        DL_S32 responseCode;
        /**
         ** rangeStartOffset 
         **/
        DL_S64 rangeStartOffset;
        /**
         ** rangeEndOffset 
         **/
        DL_S64 rangeEndOffset;

        /**
         ** relocation url 
         **/
        DL_STRING location;
        /**
         ** Via: 
         **/
        DL_STRING viaInfo;

        /**
         ** Request forbidden Code 
         **/
        DL_S32 forbiddenCode;

    } HttpHeaderResponse;




    typedef struct DataSpec {
        DL_CONST_STRING url;
        /**
         ** Body for a POST request, null otherwise.
         **/
        //DL_BYTE postBody[];
        /**
         ** The absolute position of the data in the full stream.
         **/
        DL_S64 absoluteStreamPosition;
        /**
         ** The position of the data when read from {@link #uri}.
         ***/
        DL_S64 position;
        /**
         ** The length of the data, or {LENGTH_UNSET}.
         **/
        DL_S64 length;
        /**
         ** A key that uniquely identifies the original stream. Used for cache indexing. May be null if the
         **/
        DL_CONST_STRING key;
        /**
         ** Request flags. Currently #FLAG_ALLOW_GZIP and
         **/
        DL_S32 flags;
        /**
         ** header vectormap;
         **/
        DL_CONST_STRING headUserAgent;

        /**DL_HTTP_SESSION_STATUS
         **/
        DL_U32 downloadStatus;

        DL_S32 connectTimeoutMs;

        DL_S32 readTimeoutMs;

        DL_S32 multiHttpTastNum; //the task number per Downloader
    } DataSpec;


    typedef enum DL_INDEXTYPE {

        DL_IndexComponentStartUnused = 0x01000000,
        DL_IndexComponentInitInfo ,        //init params
        DL_IndexComponentDownloadInfo,     //download process info
        DL_IndexComponentDnsIPlist,        //http dns setting
        DL_IndexComponentDownloadSpeedSetting,  //http recieve speed
        DL_IndexComponentDownloadSpeedGet,  //http recieve speed
        DL_IndexComponentMAX = 0x7fffffff,
    } DL_INDEXTYPE;


    typedef enum downloader_type 
    {
        DL_TYPE_HTTP = 0,
        DL_TYPE_P2P,
        DL_TYPE_FILE,
        DL_TYPE_TEST,
        DL_TYPE_MAX, // should not here
    } downloader_type;

    typedef status_t (*notify_callback_f)(int,int,int,void*);


    enum DL_EventType
    {
        DL_ET_HEADER        = 0x00004000,//Header
        DL_ET_DATA,                      //Payload
        DL_ET_ERR,                       //ERROR
        DL_ET_HTTPDNS,                   //invalid ip
        DL_ET_CURL_DEBUG,                //curl callback
        DL_ET_DATA_CMPL,                 //curl perform complete
        DL_ET_PROGRESS,                  //progress report
        DL_ET_STOP_CMPL,                 //progress report
        DL_ET_ADDTASK_CMPL,              //add a task    completed
        DL_ET_SUBTASK_DATA_CMPL,         //a task data receiver cmpl
        DL_ET_SUBTASK_ERR,               //a task data receiver err 
        DL_ET_MAX,

    };


    typedef enum DL_HTTP_ERROR{
        DL_OK               = 0,        //download success
        DL_INVAL_HANDLE     = 1000,     // invalid handle
        DL_ALREADY_PREPARED ,
        DL_ALREADY_STARTED  ,
        DL_INVAL_PARAM      ,           //invalid parameter
        DL_RT_ERR           ,           //runtime error
        DL_OUT_OF_MEMORY    ,           //out of memory
        DL_FAILED_INIT      ,           //initialization failed
        DL_TIMEOUT          ,           //timeout
        DL_USER_CANCEL      ,           //user canceld
        DL_ETCP_CONN_FAILED ,           //network connection failed
        DL_INVALID_URL      ,           //invalid url format

        /*https protocol */
        DL_EHTTP_OVER_REDIRECT = 2000,  //too many redirects
        DL_EHTTP_BAD_REQUEST   ,        //HTTP 400
        DL_EHTTP_UNAUTHORIZED  ,        //HTTP 401
        DL_EHTTP_FORBIDDEN     ,        //HTTP 403
        DL_EHTTP_NOT_FOUND     ,        //HTTP 404
        DL_EHTTP_TIMEOUT       ,        //HTTP 408
        DL_EHTTP_RANGE_ERROR   ,        //HTTP 416
        DL_EHTTP_OTHER_4XX     ,        //HTTP 4xx
        DL_EHTTP_504           ,        //HTTP 504
        DL_EHTTP_SERVER_ERROR  ,        //HTTP 5xx
        DL_EHTTP_MAX,        //HTTP threshold

    }DL_HTTP_ERROR;

    enum DL_State
    {
        DL_STATE_INIT,
        DL_STATE_IDLE,
        DL_STATE_BUSY,
        DL_STATE_INVALIDE,
    };


    typedef enum {
        ip_v4 = 0,
        ip_v6,
    } ip_type_t;

    typedef struct downloader_message {
        DL_U32 type;
        node_id node;
        DL_U32 data1;
        DL_U32 data2;
        DL_PTR pEventData;

    }downloader_message;


    typedef struct speedlimitparam {
        DL_S64 high;
        DL_S64 low;
        DL_BOOL enable;
    }speedlimitparam;




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
