/****************************************************
 *
 *  @Author: liyl- liyunlong880325@gmail.com
 *  Last modified: 2018-08-12 12:05
 *  @Filename: IDataSource.h
 *  @Description: 
 *****************************************************/
#ifndef LIYL_IRENDER_H
#define LIYL_IRENDER_H

#include "../../common/include/IInterface.h"
#include "../../foundation/media/include/ABase.h"
#include "../../foundation/system/core/utils/Errors.h"

LIYL_NAMESPACE_START

// A binder interface for implementing a stagefright DataSource remotely.
class IRender : public IInterface {
    public:
        LIYL_DECLARE_META_INTERFACE(Render);
        virtual status_t init(void* window , bool fullscreen, int32_t width, int32_t height) = 0;
        virtual void     destory() = 0;
        virtual void     setWxHAndCrop(const int32_t width, const int32_t height,const float left, const float top, const float right, const float bottom) = 0;
        virtual status_t renderFrame(void* data,int32_t size ,int32_t yuvType) = 0;
    private:
        DISALLOW_EVIL_CONSTRUCTORS(IRender);
};

LIYL_NAMESPACE_END

#endif // LIYL_IRENDER_H
