/*************************************************************************
  > File Name: cache_http_component.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年06月02日 星期五 17时58分51秒
 ************************************************************************/
#ifndef HTTP_COMPOPENT_H
#define HTTP_COMPOPENT_H

#include "ll_component.h"

//HTTPComponent will deal with startLoading,stopLoading and so on,And
//keep with the operation doing in correctly state
LIYL_NAMESPACE_START

class HTTPComponent : public  DefaultComponent {
    public:
        HTTPComponent(
                const char *name,
                const DL_CALLBACKTYPE *callbacks,
                DL_PTR appData,
                DL_COMPONENTTYPE **component);

        virtual DL_ERRORTYPE initCheck() const;

        virtual DL_ERRORTYPE setConsumedBytes(DL_S32 size);

        virtual void prepareForDestruction(); 

        virtual ~HTTPComponent();

    protected:


        virtual DL_ERRORTYPE sendCommand(
                DL_COMMANDTYPE cmd, DL_U32 param, DL_PTR data);

        virtual DL_ERRORTYPE getConfig(
                DL_INDEXTYPE index, DL_PTR params);

        virtual DL_ERRORTYPE setConfig(
                DL_INDEXTYPE index, const DL_PTR params);


        virtual DL_ERRORTYPE getState(DL_STATETYPE *state);

        virtual DL_ERRORTYPE prepareLoading( DataSpec* dataSpec);

        virtual DL_ERRORTYPE startLoading( DataSpec* dataSpec);

        virtual DL_ERRORTYPE stopLoading();

        virtual DL_ERRORTYPE pauseLoading();

        virtual DL_ERRORTYPE resumeLoading();


    private:
        static void onEventCB(DL_PTR handle/*WRAPPERCurl*/, void* pContext/*pAPP*/, int type, void* pData, int size);


        DL_ERRORTYPE getDownloadInfo_l(DL_PTR params);
        DL_ERRORTYPE getDownloadSpeed_l(DL_PTR params);
        DL_ERRORTYPE configHttpDnsIplist_l(DL_PTR params);
        DL_ERRORTYPE configDownloadSpeed_l(DL_PTR params);

        void onSendCommand( DL_COMMANDTYPE cmd, DL_U32 param);
        void onChangeState(DL_STATETYPE state) ;
        void checkTransitions();
        void onReset();

        Mutex mLock;
        DL_STATETYPE mState;
        DL_STATETYPE mTargetState;
        DL_PTR mCurlHandle;
        //static YKMutex mOnEventLock;
        Mutex mFreeDataLock;
        volatile DL_BOOL mFreeFlag;
        DL_BUFFERHEADERTYPE* mLastestHeadBuffer;
        DL_BUFFERHEADERTYPE* mLastestDataBuffer;
        DL_S32 mFilledSum;
        DL_S32 mConsumedSum;




        DISALLOW_EVIL_CONSTRUCTORS(HTTPComponent);
};

LIYL_NAMESPACE_END

#endif  //HTTP_COMPOPENT_H
