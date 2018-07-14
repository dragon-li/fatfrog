/*************************************************************************
  > File Name: pthread_gettid.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年11月02日 星期四 14时38分46秒
 ************************************************************************/
#ifndef LIYL_PTHREAD_GETTID_H
#define LIYL_PTHREAD_GETTID_H

#include <pthread.h>
#include <sys/system_properties.h>
#include "../core/define.h"

LIYL_BEGIN_DECLS

typedef struct pthread_internal_t_4_4_4
{
    struct pthread_internal_t_4_4_4*  next;
    struct pthread_internal_t_4_4_4*  prev;
    pthread_attr_t              attr;
    pid_t                       tid;
    bool                        allocated_on_heap;
    pthread_cond_t              join_cond;
    void*                       return_value;
    int                         internal_flags;
    __pthread_cleanup_t*        cleanup_stack;
    void**                      tls;         /* thread-local storage area */

    void* alternate_signal_stack;

    /*
     *      * The dynamic linker implements dlerror(3), which makes it hard for us to implement this
     *           * per-thread buffer by simply using malloc(3) and free(3).
     *                */
#define __BIONIC_DLERROR_BUFFER_SIZE 512
    char dlerror_buffer[__BIONIC_DLERROR_BUFFER_SIZE];
} pthread_internal_t_4_4_4;

typedef struct pthread_internal_t_7_1_1
{
    struct pthread_internal_t_7_1_1*  next;
    struct pthread_internal_t_7_1_1*  prev;
    pid_t                       tid;
    //TODO FIXME
} pthread_internal_t_7_1_1;



//4.4
pid_t inline pthread_gettid_4_4_4(pthread_t t) {
    pthread_internal_t_4_4_4* thread = reinterpret_cast<pthread_internal_t_4_4_4*>(t);
    return thread ? thread->tid : -1; 
}
//5.0 6.0 7.0 8.0
pid_t inline pthread_gettid_7_1_1(pthread_t t) {
    pthread_internal_t_7_1_1*  thread = reinterpret_cast<pthread_internal_t_7_1_1*>(t);
    return thread ? thread->tid : -1; 
}

#define LIYL_ANDROID_SDK_VERSION "ro.build.version.sdk"
pid_t pthread_gettid(pthread_t t) {
    char value[PROP_VALUE_MAX];
    if (__system_property_get(LIYL_ANDROID_SDK_VERSION, value) == 0 || value[0] == '\0') {
        return -1;
    }
    int sdkVersionNum = atoi(value);
    if(sdkVersionNum == 19) {
        return pthread_gettid_4_4_4(t);
    }else if(sdkVersionNum <= 24) {
        return pthread_gettid_7_1_1(t);
    }else {
        //should not be here ,don't support
        return -1;
    }

}
LIYL_END_DECLS

#endif //LIYL_PTHREAD_GETTID_H
