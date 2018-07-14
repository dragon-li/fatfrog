/*************************************************************************
  > File Name: test_component.h
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 7年06月01日 星期四 20时25分45秒
 ************************************************************************/
#ifndef DEFAULT_COMPOPENT_H
#define DEFAULT_COMPOPENT_H

#include "error.h"
#include "dl_component.h"

LIYL_NAMESPACE_START

    class DefaultComponent : virtual public  RefBase {
        public:
            DefaultComponent(
                    const char *name,
                    const DL_CALLBACKTYPE *callbacks,
                    DL_PTR appData,
                    DL_COMPONENTTYPE **component);

            virtual DL_ERRORTYPE initCheck() const;
            virtual DL_ERRORTYPE setConsumedBytes(DL_S32 size);

            void setLibHandle(void *libHandle);
            void *libHandle() const;

            virtual void prepareForDestruction() {}

            virtual ~DefaultComponent();
            DL_STRING getName() {return mName;}
        protected:

            const char *name() const;

            void notify(
                    DL_EVENTTYPE event,
                    DL_U32 data1, DL_U32 data2, DL_PTR data);

            void notifyFillBufferDone(DL_BUFFERHEADERTYPE *header);

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
            DL_STRING mName;
            const DL_CALLBACKTYPE *mCallbacks;
            DL_COMPONENTTYPE *mComponent;

            void *mLibHandle;

            static DL_ERRORTYPE SendCommandWrapper(
                    DL_HANDLETYPE component,
                    DL_COMMANDTYPE cmd,
                    DL_U32 param,
                    DL_PTR data);

           static DL_ERRORTYPE GetConfigWrapper(
                    DL_HANDLETYPE component,
                    DL_INDEXTYPE index,
                    DL_PTR params);

            static DL_ERRORTYPE SetConfigWrapper(
                    DL_HANDLETYPE component,
                    DL_INDEXTYPE index,
                    DL_PTR params);
           static DL_ERRORTYPE FillThisBufferWrapper(
                    DL_HANDLETYPE component,
                    DL_BUFFERHEADERTYPE *buffer);

            static DL_ERRORTYPE GetStateWrapper(
                    DL_HANDLETYPE component,
                    DL_STATETYPE *state);

            static DL_ERRORTYPE PrepareLoadingWrapper(
                    DL_HANDLETYPE component,
                    DataSpec* dataSpec);

            static DL_ERRORTYPE StartLoadingWrapper(
                    DL_HANDLETYPE component,
                    DataSpec* dataSpec);

            static DL_ERRORTYPE StopLoadingWrapper(
                    DL_HANDLETYPE component);

            static DL_ERRORTYPE PauseLoadingWrapper(
                    DL_HANDLETYPE component);

            static DL_ERRORTYPE ResumeLoadingWrapper(
                    DL_HANDLETYPE component);

            static DL_ERRORTYPE ResumeLoadingAtWrapper(
                    DL_HANDLETYPE component,
                    DataSpec* dataSpec);



            DISALLOW_EVIL_CONSTRUCTORS(DefaultComponent);
    };

LIYL_NAMESPACE_END

#endif  //Default_COMPOPENT_H 

