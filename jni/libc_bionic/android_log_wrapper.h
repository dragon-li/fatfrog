/****************************************************
*
*  @Author: liyl- liyunlong880325@gmail.com
*  Last modified: 2018-01-18 21:59
*  @Filename: ./android_log_wrapper.h
*****************************************************/

#ifndef __LIYL_ANDROID_LOG_WRAPPER_H__
#define __LIYL_ANDROID_LOG_WRAPPER_H__
#include <android/log.h>

#if 1
#ifndef MODULE_NAME
#define MODULE_NAME  "malloc_debug"
#endif

#define LLOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, MODULE_NAME, __VA_ARGS__)
#define LLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__)
#define LLOGI(...) __android_log_print(ANDROID_LOG_INFO, MODULE_NAME, __VA_ARGS__)
#define LLOGW(...) __android_log_print(ANDROID_LOG_WARN,MODULE_NAME, __VA_ARGS__)
#define LLOGE(...) __android_log_print(ANDROID_LOG_ERROR,MODULE_NAME, __VA_ARGS__)
#define LLOGF(...) __android_log_print(ANDROID_LOG_FATAL,MODULE_NAME, __VA_ARGS__)

#define LASSERT(cond, ...) if (!(cond)) {__android_log_assert(#cond, MODULE_NAME, __VA_ARGS__);}
#else
#define LLOGV(...)
#define LLOGD(...)
#define LLOGI(...)
#define LLOGW(...)
#define LLOGE(...)
#define LLOGF(...)
#define LASSERT(cond, ...)
#endif


#define PRINTLONGSTR(MY_STR,...) if( MY_STR != NULL) {                  \
	std::string tmpStr(MY_STR);                                         \
	int size = tmpStr.size();                                           \
	int unit = 500;                                                     \
	int multi = size/unit;                                              \
	int remainder = size%unit;                                          \
	int index = 0;                                                      \
	for(;index < multi; index++) {                                      \
                                                                        \
        LLOGW("str[%d] : %s",index,tmpStr.substr(index*unit,unit).c_str());   \
	}                                                                   \
        LLOGW("str[%d] : %s",index,tmpStr.substr(index*unit,remainder).c_str());             \
                                                                        \
}                                                                       

#endif // __LIYL_ANDROID_LOG_WRAPPER_H__ 
