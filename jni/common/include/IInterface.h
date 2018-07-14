/*************************************************************************
	> File Name: IINTERFACE.h
	> Author: liyunlong
	> Mail: liyunlong_88@126.com 
	> Created Time: 2018年02月26日 星期一 21时55分11秒
 ************************************************************************/

#ifndef LIYL_IINTERFACE_H
#define LIYL_IINTERFACE_H
#include "../../foundation/system/core/define.h"
#include "../../foundation/system/core/utils/RefBase.h"


namespace liyl {

class IInterface : public virtual RefBase
{
	public:
		IInterface();

	protected:
		virtual                     ~IInterface();
};

}; // namespace liyl


//IINTERFACE
#define LIYL_DECLARE_META_INTERFACE(INTERFACE)                          \
	I##INTERFACE();                                                     \
    virtual ~I##INTERFACE();                                            \


#define LIYL_IMPLEMENT_META_INTERFACE(INTERFACE, NAME)                  \
	I##INTERFACE::I##INTERFACE() { }                                    \
    I##INTERFACE::~I##INTERFACE() { }                                   \


#endif //LIYL_IINTERFACE_H
