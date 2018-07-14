/*************************************************************************
  > File Name: dl_core.h
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 年06月01日 星期四 16时57分14秒
 ************************************************************************/


#ifndef DL_Core_h
#define DL_Core_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    /* Each DL header shall include all required header files to allow the
     *  header to compile without errors.  The includes below are required
     *  for this header file to compile successfully
     */

#include "dl_type.h"
#include "dl_common_ext.h"


    /** The DL_COMMANDTYPE enumeration is used to specify the action in the
     *  DL_SendCommand macro.
     *  @ingroup core
     */
    typedef enum DL_COMMANDTYPE
    {
      DL_CommandStateSet =  0x00001000,
      DL_CommandStopAsync,
      DL_CommandMax = 0X7FFFFFFF
  } DL_COMMANDTYPE;


    typedef enum DL_STATETYPE
    {
      DL_StateInvalid = 0,      /**< component has detected that it's internal data
                                structures are corrupted to the point that
                                it cannot determine it's state properly */
      DL_StateInit,      /**< component has been loaded but has not completed
                         initialization.  The DL_SetParameter macro
                         and the DL_GetParameter macro are the only
                         valid macros allowed to be sent to the
                         component in this state. */
      DL_StateIdle,        /**< component initialization has been completed
                           successfully and the component is ready to
                           to start. */
      DL_StateExecuting,   /**< component has accepted the start command and
                           is processing data (if data is available) */
      DL_StatePause,       /**< component has received pause command */

      DL_StateDeinit,       /**< component has received close command */

      DL_StateWaitForResources, /**< component is waiting for resources, either after
                                preemption or before it gets the resources requested.
                                See specification for complete details. */
      DL_StateVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
      DL_StateMax = 0x7FFFFFFF
  } DL_STATETYPE;

    /** The DL_ERRORTYPE enumeration defines the standard DL Errors.  These
     *  errors should cover most of the common failure cases.  However,
     *  vendors are free to add additional error messages of their own as
     *  long as they follow these rules:
     *  .  Vendor error messages shall be in the range of 0x90000000 to
     *      x9000FFFF.
     *  .  Vendor error messages shall be defined in a header file provided
     *      with the component.  No error messages are allowed that are
     *      not defined.
     */
    typedef enum DL_ERRORTYPE
    {
      DL_ErrorNone = 0,

      /** There were insufficient resources to perform the requested operation */
      DL_ErrorInsufficientResources = (DL_S32) 0x80001000,

      /** There was an error, but the cause of the error could not be determined */
      DL_ErrorUndefined = (DL_S32) 0x80001001,

      /** The component name string was not valid */
      DL_ErrorInvalidComponentName = (DL_S32) 0x80001002,

      /** No component with the specified name string was found */
      DL_ErrorComponentNotFound = (DL_S32) 0x80001003,

      /** The component specified did not have a "DL_ComponentInit" or
      "DL_ComponentDeInit entry point */
      DL_ErrorInvalidComponent = (DL_S32) 0x80001004,

      /** One or more parameters were not valid */
      DL_ErrorBadParameter = (DL_S32) 0x80001005,

      /** The requested function is not implemented */
      DL_ErrorNotImplemented = (DL_S32) 0x80001006,

      /** The component is in the state DL_StateInvalid */
      DL_ErrorInvalidState = (DL_S32) 0x80001007,
      /** The component don't support this setting*/
      DL_ErrorUnsupportedSetting = (DL_S32) 0x80001008,

      /** The component don't known this error*/
      DL_ErrorUnKnown = (DL_S32) 0x80001009,

      /** The component http session return error */
      DL_ErrorProtocol = (DL_S32) 0x80001010,

      /** The component timeout*/
      DL_ErrorTimeout = (DL_S32) 0x80001011,


      DL_ErrorVendorStartUnused = (DL_S32)0x90000000, /**< Reserved region for introducing Vendor Extensions */
      DL_ErrorMax = 0x7FFFFFFF
  } DL_ERRORTYPE;

    /** @ingroup core */
    typedef DL_ERRORTYPE (* DL_COMPONENTINITTYPE)(DL_IN  DL_HANDLETYPE hComponent);


    typedef enum DL_BUFFER_FLAGS {
        DL_BUFFERFLAG_NEEDFREE    = (DL_U32)0x00000001,
        DL_BUFFERFLAG_FLUSHBUFFER = (DL_U32)0x00000002,
    } DL_BUFFER_FLAGS;

    /** @ingroup buf */
    typedef struct DL_BUFFERHEADERTYPE
    {
      DL_U32 nSize;              /**< size of the structure in bytes */
      DL_VERSIONTYPE nVersion;   /**< DL specification version information */
      DL_U8* pBuffer;            /**< Pointer to actual block of memory
                                 that is acting as the buffer */
      DL_U32 nAllocLen;          /**< size of the buffer allocated, in bytes */
      DL_U32 nFilledLen;         /**< number of bytes currently in the
                                 buffer */
      DL_U32 nOffset;            /**< start offset of valid data in bytes from
                                 the start of the buffer */
      DL_PTR pAppPrivate;        /**< pointer to any data the application
                                 wants to associate with this buffer */
      DL_PTR pDownloaderPrivate;   /**< pointer to any data the platform */
      DL_U32 nFlags;           /**< buffer specific flags  */
      DL_PTR pPreNode;           /** pPreNode is pre DL_BUFFERHEADERTYPE pointer */
      DL_PTR pNextNode;           /** pPreNode is next DL_BUFFERHEADERTYPE pointer */
  } DL_BUFFERHEADERTYPE;

    /** @ingroup comp */
    typedef enum DL_EVENTTYPE
    {
      DL_EventCmdComplete,         /**< component has sucessfully completed a command */
      DL_EventError,               /**< component has detected an error condition */
      DL_EventSettingChange,       /**< component has detected an setting change*/
      DL_EventSendCmd,             /**< component send a cmd for async operation*/
      DL_EventVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
      DL_EventMax = 0x7FFFFFFF
  } DL_EVENTTYPE;

    typedef struct DL_CALLBACKTYPE
    {
      /** The EventHandler method is used to notify the application when an
      event of interest occurs.  Events are defined in the DL_EVENTTYPE
      enumeration.  Please see that enumeration for details of what will
      be returned for each type of event. Callbacks should not return
      an error to the component, so if an error occurs, the application
      shall handle it internally.  This is a blocking call.

*/

      DL_ERRORTYPE (*EventHandler)(
              DL_IN DL_HANDLETYPE hComponent,
              DL_IN DL_PTR pAppData,
              DL_IN DL_EVENTTYPE eEvent,
              DL_IN DL_U32 nData1,
              DL_IN DL_U32 nData2,
              DL_IN DL_PTR pEventData);

      /** The FillThisBufferDone method is used to return filled buffers from an
      output port back to the application for emptying and then reuse.
      This is a blocking call so the application should not attempt to
      empty the buffers during this call, but should queue the buffers
      and empty them in another thread.  There is no error return, so
      the application shall handle any errors generated internally.  The
      application shall also update the buffer header to indicate the
      number of bytes placed into the buffer.
      */
      DL_ERRORTYPE (*FillThisBufferDone)(
              DL_OUT DL_HANDLETYPE hComponent,
              DL_OUT DL_PTR pAppData,
              DL_OUT DL_BUFFERHEADERTYPE* pBuffer);

  } DL_CALLBACKTYPE;


    /** GetComponentVersion will return information about the component.
    This is a blocking call.  This macro will go directly from the
    application to the component (via a core macro).  The
    component will return from this call within  msec.
    */
#define DL_GetComponentVersion(                            \
        hComponent,                                         \
        pComponentName,                                     \
        pComponentVersion,                                  \
        pSpecVersion,                                       \
        pComponentUUID)                                     \
    ((DL_COMPONENTTYPE*)hComponent)->GetComponentVersion(  \
        hComponent,                                         \
        pComponentName,                                     \
        pComponentVersion,                                  \
        pSpecVersion,                                       \
        pComponentUUID)                 /* Macro End */


    /** Send a command to the component.  This call is a non-blocking call.
    The component should check the parameters and then queue the command
    to the component thread to be executed.  The component thread shall
    send the EventHandler() callback at the conclusion of the command.
    This macro will go directly from the application to the component (via
    a core macro).  The component will return from this call within  msec.
    */
#define DL_SendCommand(                                    \
        hComponent,                                        \
        Cmd,                                               \
        nParam,                                            \
        pCmdData)                                          \
    ((DL_COMPONENTTYPE*)hComponent)->SendCommand(         \
        hComponent,                                        \
        Cmd,                                               \
        nParam,                                            \
        pCmdData)                          /* Macro End */


    /** The DL_GetConfig macro will get one of the configuration structures
    from a component.  This macro can be invoked anytime after the
    component has been loaded.  The nParamIndex call parameter is used to
    indicate which structure is being requested from the component.  The
    application shall allocate the correct structure and shall fill in the
    structure size and version information before invoking this macro.
    If the component has not had this configuration parameter sent before,
    then the component should return a set of valid DEFAULT values for the
    component.  This is a blocking call.

    @ingroup comp
    */
#define DL_GetConfig(                                      \
        hComponent,                                        \
        nConfigIndex,                                      \
        pComponentConfigStructure)                         \
    ((DL_COMPONENTTYPE*)hComponent)->GetConfig(            \
        hComponent,                                        \
        nConfigIndex,                                      \
        pComponentConfigStructure)       /* Macro End */


    /** The DL_SetConfig macro will send one of the configuration
    structures to a component.  Each structure shall be sent one at a time,
    each in a separate invocation of the macro.  This macro can be invoked
    anytime after the component has been loaded.  The application shall
    allocate the correct structure and shall fill in the structure size
    and version information (as well as the actual data) before invoking
    this macro.  The application is free to dispose of this structure after
    the call as the component is required to copy any data it shall retain.
    This is a blocking call.

*/
#define DL_SetConfig(                                      \
        hComponent,                                        \
        nConfigIndex,                                      \
        pComponentConfigStructure)                         \
    ((DL_COMPONENTTYPE*)hComponent)->SetConfig(            \
        hComponent,                                        \
        nConfigIndex,                                      \
        pComponentConfigStructure)       /* Macro End */


    /** The DL_GetState macro will invoke the component to get the current
    state of the component and place the state value into the location
    pointed to by pState.

*/
#define DL_GetState(                                      \
        hComponent,                                       \
        pState)                                           \
    ((DL_COMPONENTTYPE*)hComponent)->GetState(            \
        hComponent,                                       \
        pState)                         /* Macro End */


    /** The DL_PrePareLoading macro will open a connection with
     * dataSpec that discript http request
     *
     */
#define DL_PrepareLoading(                               \
        hComponent,                                      \
        dataSpec)                                        \
    ((DL_COMPONENTTYPE*)hComponent)->PrepareLoading(     \
        hComponent,                                      \
        dataSpec)                        /* Macro End */




    /** The DL_StartLoading macro will open a connection with
     * dataSpec that discript http request
     *
     */
#define DL_StartLoading(                                 \
        hComponent,                                      \
        dataSpec)                                        \
    ((DL_COMPONENTTYPE*)hComponent)->StartLoading(       \
        hComponent,                                      \
        dataSpec)                        /* Macro End */


    /** The DL_StopLoading macro will disconnect a connection
     *
     */
#define DL_StopLoading(                                 \
        hComponent)                                     \
    ((DL_COMPONENTTYPE*)hComponent)->StopLoading(       \
        hComponent)                       /* Macro End */

    /** The DL_PauseLoading macro will disconnect a connection
     *
     */
#define DL_PauseLoading(                                \
        hComponent)                                     \
    ((DL_COMPONENTTYPE*)hComponent)->PauseLoading(      \
        hComponent)                       /* Macro End */



    /** The DL_ResumeLoading macro will resume a connection
     *
     */
#define DL_ResumeLoading(                                 \
        hComponent)                                       \
    ((DL_COMPONENTTYPE*)hComponent)->ResumeLoading(       \
        hComponent)                       /* Macro End */



    /** The DL_Init method is used to initialize the DL core.  It shall be the
    first call made into DL and it should only be executed one time without
    an interviening DL_Deinit call.
    */
    DL_API DL_ERRORTYPE DL_APIENTRY DL_Init(void);


    /** The DL_Deinit method is used to deinitialize the DL core.  It shall be
    the last call made into DL. In the event that the core determines that
    thare are components loaded when this call is made, the core may return
    with an error rather than try to unload the components.

*/
    DL_API DL_ERRORTYPE DL_APIENTRY DL_Deinit(void);

    /** The DL_GetHandle method will locate the component specified by the
    component name given, load that component into memory and then invoke
    the component's methods to create an instance of the component.
    */
    DL_API DL_ERRORTYPE DL_APIENTRY DL_GetHandle(
            DL_OUT DL_HANDLETYPE* pHandle,
            DL_IN  DL_STRING cComponentName,
            DL_IN  DL_PTR pAppData,
            DL_IN  DL_CALLBACKTYPE* pCallBacks);


    /** The DL_FreeHandle method will free a handle allocated by the DL_GetHandle
    method.  If the component reference count goes to zero, the component will
    be unloaded from memory.

    The core should return from this call within 20 msec when the component is
    in the DL_StateLoaded state.
    */
    DL_API DL_ERRORTYPE DL_APIENTRY DL_FreeHandle(
            DL_IN  DL_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
