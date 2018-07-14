/*************************************************************************
	> File Name: config.h
	> Author: liyunlong
	> Mail: liyunlong_88@126.com 
	> Created Time: 2018年01月11日 星期四 17时13分24秒
 ************************************************************************/
#ifndef LIYL_LIBC_BIONIC_DLCONFIG_H
#define LIYL_LIBC_BIONIC_DLCONFIG_H
/* Configure dlmalloc. */

#define DEBUG 1
//#define HAVE_ANDROID_LOGGER 1
#define FOOTERS 1
//#define MALLOC_ALIGNMENT 128U

#define HAVE_GETPAGESIZE 1
#define MALLOC_INSPECT_ALL 1
#define MSPACES 0
#define REALLOC_ZERO_BYTES_FREES 1
#define USE_DL_PREFIX 1
#define USE_LOCKS 1
#define LOCK_AT_FORK 1
#define USE_RECURSIVE_LOCK 0
#define USE_SPIN_LOCKS 0
#define DEFAULT_MMAP_THRESHOLD (64U * 1024U)



#define malloc_getpagesize getpagesize()

#endif// LIBC_BIONIC_DLCONFIG_H
