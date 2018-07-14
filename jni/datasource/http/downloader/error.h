/*************************************************************************
  > File Name: error.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年05月24日 星期三 20时17分41秒
 ************************************************************************/

#ifndef JNI_LIYL_ERROR_H
#define JNI_LIYL_ERROR_H

#include "../../../foundation/system/core/define.h"
#include "../../../foundation/system/core/utils/RefBase.h"
#include "../../../foundation/system/core/utils/Errors.h"

#include "dl_component.h"


#include "../../../foundation/system/core/threads.h"
#include "../../../foundation/media/include/ADebug.h"
#include "../../../foundation/media/include/ABuffer.h"

#if 1
#undef LLOGV
#define LLOGV(...)
#endif

LIYL_NAMESPACE_START

LIYL_NAMESPACE_END

#endif //JNI_LIYL_ERROR_H
