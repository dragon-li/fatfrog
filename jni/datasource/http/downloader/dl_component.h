/*************************************************************************
  > File Name: dl_component.h
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 7年06月01日 星期四 17时29分46秒
 ************************************************************************/


#ifndef DL_Component_h
#define DL_Component_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "dl_core.h"

    /** The DL_HANDLETYPE structure defines the component handle.  The component
     *  handle is used to access all of the component's public methods and also
     *  contains pointers to the component's private data area.  The component
     *  handle is initialized by the DL core (with help from the component)
     *  during the process of loading the component.  After the component is
     *  successfully loaded, the application can safely access any of the
     *  component's public functions (although some may return an error because
     *  the state is inappropriate for the access).
     *
     *  @ingroup comp
     */
    typedef struct DL_COMPONENTTYPE
    {
        /** The size of this structure, in bytes.  It is the responsibility
          of the allocator of this structure to fill in this value.  Since
          this structure is allocated by the GetHandle function, this
          function will fill in this value. */
        DL_U32 nSize;

        /** nVersion is the version of the DL specification that the structure
          is built against.  It is the responsibility of the creator of this
          structure to initialize this value and every user of this structure
          should verify that it knows how to use the exact version of
          this structure found herein. */
        DL_VERSIONTYPE nVersion;

        /** pComponentPrivate is a pointer to the component private data area.
          This member is allocated and initialized by the component when the
          component is first loaded.  The application should not access this
          data area. */
        DL_PTR pComponentPrivate;

        /** pApplicationPrivate is a pointer that is a parameter to the
          DL_GetHandle method, and contains an application private value
          provided by the IL client.  This application private data is
          returned to the IL Client by DL in all callbacks */
        DL_PTR pApplicationPrivate;

        /** refer to DL_GetComponentVersion in DL_core.h or the DL IL
          specification for details on the GetComponentVersion method.
          */
        DL_ERRORTYPE (*GetComponentVersion)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_OUT DL_STRING pComponentName,
                DL_OUT DL_VERSIONTYPE* pComponentVersion,
                DL_OUT DL_VERSIONTYPE* pSpecVersion,
                DL_OUT DL_UUIDTYPE* pComponentUUID);

        /** refer to DL_SendCommand in DL_core.h or the DL IL
          specification for details on the SendCommand method.
          */
        DL_ERRORTYPE (*SendCommand)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_IN  DL_COMMANDTYPE Cmd,
                DL_IN  DL_U32 nParam1,
                DL_IN  DL_PTR pCmdData);


        /** refer to DL_GetConfig in DL_core.h or the DL IL
          specification for details on the GetConfig method.
          */
        DL_ERRORTYPE (*GetConfig)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_IN  DL_INDEXTYPE nIndex,
                DL_INOUT DL_PTR pComponentConfigStructure);


        /** refer to DL_SetConfig in DL_core.h or the DL IL
          specification for details on the SetConfig method.
          */
        DL_ERRORTYPE (*SetConfig)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_IN  DL_INDEXTYPE nIndex,
                DL_IN  DL_PTR pComponentConfigStructure);



        /** refer to DL_GetState in DL_core.h or the DL IL
          specification for details on the GetState method.
          */
        DL_ERRORTYPE (*GetState)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_OUT DL_STATETYPE* pState);

       /** refer to DL_PrepareLoading in DL_core.h or the DL IL
          specification for details on the PrepareLoading method.
          @ingroup buf
          */
        DL_ERRORTYPE (*PrepareLoading)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_IN  DataSpec* dataSpec);
	

        /** refer to DL_StartLoading in DL_core.h or the DL IL
          specification for details on the StartLoading method.
          @ingroup buf
          */
        DL_ERRORTYPE (*StartLoading)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_IN  DataSpec* dataSpec);
		
        /** refer to DL_StopLoading in DL_core.h or the DL IL
          specification for details on the StopLoading method.
          @ingroup buf
          */
        DL_ERRORTYPE (*StopLoading)(
                DL_IN  DL_HANDLETYPE hComponent);

        /** refer to DL_PauseLoading in DL_core.h or the DL IL
          specification for details on the PauseLoading method.
          @ingroup buf
          */
        DL_ERRORTYPE (*PauseLoading)(
                DL_IN  DL_HANDLETYPE hComponent);


        /** refer to DL_ResumeLoading in DL_core.h or the DL IL
          specification for details on the ResumeLoading method.
          @ingroup buf
          */
        DL_ERRORTYPE (*ResumeLoading)(
                DL_IN  DL_HANDLETYPE hComponent);


        /** The SetCallbacks method is used by the core to specify the callback
          structure from the application to the component.  This is a blocking
         */
        DL_ERRORTYPE (*SetCallbacks)(
                DL_IN  DL_HANDLETYPE hComponent,
                DL_IN  DL_CALLBACKTYPE* pCallbacks,
                DL_IN  DL_PTR pAppData);

        /** ComponentDeInit method is used to deinitialize the component
          providing a means to free any resources allocated at component
          initialization.  NOTE:  After this call the component handle is not valid for further use.
         */
        DL_ERRORTYPE (*ComponentDeInit)(
                DL_IN  DL_HANDLETYPE hComponent);

    } DL_COMPONENTTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
