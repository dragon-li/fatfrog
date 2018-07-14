/*************************************************************************
	> File Name: dl_vender_ext.h
	> Author: liyunlong
	> Mail: liyunlong_88@126.com 
	> Created Time: 2017年06月14日 星期三 21时12分59秒
 ************************************************************************/

#ifndef DL_VENDOR_EXT_H
#define DL_VENDOR_EXT_H
//Download module error code definition
enum VendorErrorCode
{
    VENDOR_OK               = 0,        //download success
    VENDOR_INVAL_HANDLE     = 1000,     // invalid handle
    VENDOR_ALREADY_PREPARED,
    VENDOR_ALREADY_STARTED,
    VENDOR_INVAL_PARAM      ,           //invalid parameter
    VENDOR_RT_ERR           ,           //runtime error
    VENDOR_OUT_OF_MEMORY    ,           //out of memory
    VENDOR_FAILED_INIT      ,           //initialization failed
    VENDOR_TIMEOUT          ,           //timeout
    VENDOR_USER_CANCEL      ,           //user canceld
    VENDOR_ETCP_CONN_FAILED ,           //network connection failed
    VENDOR_INVALID_URL      ,           //invalid url format

    /* http/https protocol */
    VENDOR__EHTTP_OVER_REDIRECT = 2000,  //too many redirects
    VENDOR_EHTTP_BAD_REQUEST   ,        //HTTP 400
    VENDOR_EHTTP_UNAUTHORIZED  ,        //HTTP 401
    VENDOR_EHTTP_FORBIDDEN     ,        //HTTP 403
    VENDOR_EHTTP_NOT_FOUND     ,        //HTTP 404
    VENDOR_EHTTP_TIMEOUT       ,        //HTTP 408
    VENDOR_EHTTP_RANGE_ERROR   ,        //HTTP 416
    VENDOR_EHTTP_OTHER_4XX     ,        //HTTP 4xx
    VENDOR_EHTTP_504           ,        //HTTP 504
    VENDOR_EHTTP_SERVER_ERROR  ,        //HTTP 5xx
    VENDOR_EHTTP_MAX,        //HTTP threshold

};
#endif //DL_VENDER_EXT_H

