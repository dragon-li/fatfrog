/*************************************************************************
  > File Name: dl_utils.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年06月13日 星期二 12时04分20秒
 ************************************************************************/

#ifndef DL_UTILS_CPP
#define DL_UTILS_CPP
#include "error.h"
#include "dl_utils.h"




//////////////////////////////////////////////
status_t createDataSpec(DataSpec ** ppdataSpec) {

    DataSpec* ptr = (DataSpec*)malloc(sizeof(DataSpec));
    if(ptr == NULL) {
        return -ENOMEM; 
    }
    memset(ptr,0,sizeof(DataSpec));
    *ppdataSpec = ptr;
    return 0;//OK;
}



void releaseDataSpec(DataSpec * pDataSpec) {
    if(pDataSpec == NULL) {
        return;
    }
    memset(pDataSpec,0,sizeof(DataSpec));
    free(pDataSpec);

}


#endif //DL_UTILS_CPP

