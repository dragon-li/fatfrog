/*************************************************************************
  > File Name: ll_http_multi_component.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2017年06月02日 星期五 17时58分51秒
 ************************************************************************/
#ifndef LL_MULTI_HTTP_COMPOPENT_H
#define LL_MULTI_HTTP_COMPOPENT_H 

#include "ll_component.h"
#include <map>
using std::map;
//MultiHTTPComponent will deal with startLoading,stopLoading and so on,And
//keep with the operation doing in correctly state
LIYL_NAMESPACE_START

class MultiHTTPComponent : public  DefaultComponent {
    public:
        MultiHTTPComponent(
                const char *name,
                const DL_CALLBACKTYPE *callbacks,
                DL_PTR appData,
                DL_COMPONENTTYPE **component);

        virtual DL_ERRORTYPE initCheck() const;

        virtual void prepareForDestruction(); 

        virtual ~MultiHTTPComponent();

        //used by add a new httpTask
        DL_ERRORTYPE addTaskBufferInfo(DL_S64 position,DL_S32 length,DL_S32 index);
		
        DL_ERRORTYPE delTaskBufferInfo(DL_S32 index);
        //get the filled buffer
		DL_BUFFERHEADERTYPE* getAlreadyBuffer(DL_S32 index);
		//fill the taskBuffer
        DL_ERRORTYPE fillThisTaskBuffer(DL_S32 index,DL_BYTE pData,DL_S32 size);
        void releaseTaskBuffers();

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

        void onSendCommand( DL_COMMANDTYPE cmd, DL_U32 param);
        void onChangeState(DL_STATETYPE state) ;
        void checkTransitions();
        void onReset();

	public:
        Mutex mLock;
        DL_BUFFERHEADERTYPE* mLastestHeadBuffer;
        Mutex mFreeDataLock;
        DL_BOOL mFreeFlag;
        DL_BUFFERHEADERTYPE* mLastestDataBuffer;
        //static Mutex mOnEventLock;
        DL_S32 mTaskNumMax;

    private:
		DL_STATETYPE mState;
        DL_STATETYPE mTargetState;
        DL_PTR mCurlHandle;
        DL_S64 mFilledDataBytes;
        DL_S64 mPostDataBytes;
        

//////////////////////////////////////////       
		typedef struct BufferInfo {
		     DL_S64 absOffset;	
			 DL_S32 size;
			 DL_S32 readOffset;
			 DL_S32 writeOffset;
			 DL_BYTE data;
		}BufferInfo;
		typedef std::map<DL_S32,BufferInfo*> tTaskBuffer;
        inline DL_S32 getFilledSize(BufferInfo* info){
			return info->writeOffset-info->readOffset;
		}
        inline DL_BOOL isReadCompleted(BufferInfo* info){
			return (info->readOffset == (info->size))?DL_TRUE:DL_FALSE;
		}
	public:
    	Mutex mTaskBufferLock;
		DL_S32 mCurrentTaskIndex;
		tTaskBuffer mTaskBuffers;


//////////////////////////////////////////       
        DISALLOW_EVIL_CONSTRUCTORS(MultiHTTPComponent);
};

LIYL_NAMESPACE_END

#endif  //LL_MULTI_HTTP_COMPOPENT_H
