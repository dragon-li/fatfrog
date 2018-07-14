/*************************************************************************
    > File Name: define.h
    > Author: liyunlong
    > Mail: liyunlong_88@126.com 
    > Created Time: 2017年09月30日 星期六 20时37分16秒
 ************************************************************************/

/* Declare this library function hidden and internal */
#if defined(_WIN32)
#define LIBLOG_HIDDEN
#else
#define LIBLOG_HIDDEN __attribute__((visibility("hidden")))
#endif

/* Declare this library function visible and external */
#if defined(_WIN32)
#define LIBLOG_ABI_PUBLIC
#else
#define LIBLOG_ABI_PUBLIC __attribute__((visibility("default")))
#endif

/* Declare this library function visible but private */
#define LIBLOG_ABI_PRIVATE LIBLOG_ABI_PUBLIC


#define LIYL_NAMESPACE_START namespace liyl {
#define LIYL_NAMESPACE_END }; //namespace liyl
#define USING_NAMESPACE_LIYL using namespace liyl;

#if defined(__cplusplus)
#define	LIYL_BEGIN_DECLS		extern "C" {
#define	LIYL_END_DECLS		}
#define	liyl_static_cast(x,y)	static_cast<x>(y)
#else
#define	LIYL_BEGIN_DECLS
#define	LIYL_END_DECLS
#define	liyl_static_cast(x,y)	(x)y
#endif
