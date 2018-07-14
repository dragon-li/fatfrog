/*************************************************************************
  > File Name: dl_utils.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年06月13日 星期二 12时04分20秒
 ************************************************************************/

#ifndef DL_UTILS_H 
#define DL_UTILS_H
#include <stdlib.h>
#include "dl_common_ext.h"

////////////////////////////DataSpec////////
status_t createDataSpec(DataSpec ** ppdataSpec); 

void resetDataSpec(DataSpec * pDataSpec); 

void releaseDataSpec(DataSpec * pDataSpec); 


#endif //DL_UTILS_H

