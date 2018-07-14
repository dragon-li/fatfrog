/*************************************************************************
    > File Name: test_component.cpp
    > Author: liyunlong
    > Mail: liyunlong_88@.com 
    > Created Time: 7年06月01日 星期四 20时45分56秒
 ************************************************************************/

#include "ll_component.h"

#undef LOG_TAG
#define LOG_TAG "DefaultComponent"

LIYL_NAMESPACE_START


DefaultComponent::DefaultComponent(
        const char *name,
        const DL_CALLBACKTYPE *callbacks,
        DL_PTR appData,
        DL_COMPONENTTYPE **component)
    : mName(const_cast<char*>(name)),
    mCallbacks(callbacks),
    mComponent(new DL_COMPONENTTYPE),
    mLibHandle(NULL) {
        *component = NULL;
        mComponent->nSize = sizeof(*mComponent);
        mComponent->nVersion.s.nVersionMajor = 1;
        mComponent->nVersion.s.nVersionMinor = 0;
        mComponent->nVersion.s.nRevision = 0;
        mComponent->nVersion.s.nStep = 0;
        mComponent->pComponentPrivate = this;
        mComponent->pApplicationPrivate = appData;

        mComponent->GetComponentVersion = NULL;
        mComponent->SendCommand = SendCommandWrapper;
        mComponent->GetConfig = GetConfigWrapper;
        mComponent->SetConfig = SetConfigWrapper;
        mComponent->GetState = GetStateWrapper;
        mComponent->PrepareLoading = PrepareLoadingWrapper;
        mComponent->StartLoading = StartLoadingWrapper;
        mComponent->StopLoading = StopLoadingWrapper;
        mComponent->PauseLoading = PauseLoadingWrapper;
        mComponent->ResumeLoading = ResumeLoadingWrapper;
        mComponent->SetCallbacks = NULL;
        mComponent->ComponentDeInit = NULL;

        *component = mComponent;
        LLOGD("this = %p mComponent = %p",this,mComponent);
    }

DefaultComponent::~DefaultComponent() {
    ENTER_FUNC;
    if(mComponent != NULL)
    delete mComponent;
    mComponent = NULL;
    ESC_FUNC;
}

void DefaultComponent::setLibHandle(void *libHandle) {
    CHECK(libHandle != NULL);
    mLibHandle = libHandle;
}

void *DefaultComponent::libHandle() const {
    return mLibHandle;
}

DL_ERRORTYPE DefaultComponent::initCheck() const {
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::setConsumedBytes(DL_S32 size) {
	(void)size;
    return DL_ErrorNone;
}



const char *DefaultComponent::name() const {
    return mName;
}

void DefaultComponent::notify(
        DL_EVENTTYPE event,
        DL_U32 data1, DL_U32 data2, DL_PTR data) {
    (*mCallbacks->EventHandler)(
            mComponent,
            mComponent->pApplicationPrivate,
            event,
            data1,
            data2,
            data);
}


void DefaultComponent::notifyFillBufferDone(DL_BUFFERHEADERTYPE *header) {
    (*mCallbacks->FillThisBufferDone)(
            mComponent, mComponent->pApplicationPrivate, header);
}

// static
DL_ERRORTYPE DefaultComponent::SendCommandWrapper(
        DL_HANDLETYPE component,
        DL_COMMANDTYPE cmd,
        DL_U32 param,
        DL_PTR data) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->sendCommand(cmd, param, data);
}




// static
DL_ERRORTYPE DefaultComponent::GetConfigWrapper(
        DL_HANDLETYPE component,
        DL_INDEXTYPE index,
        DL_PTR params) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getConfig(index, params);
}

// static
DL_ERRORTYPE DefaultComponent::SetConfigWrapper(
        DL_HANDLETYPE component,
        DL_INDEXTYPE index,
        DL_PTR params) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->setConfig(index, params);
}




// static
DL_ERRORTYPE DefaultComponent::GetStateWrapper(
        DL_HANDLETYPE component,
        DL_STATETYPE *state) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getState(state);
}

// static
DL_ERRORTYPE DefaultComponent::PrepareLoadingWrapper(
        DL_HANDLETYPE component,
        DataSpec* dataSpec) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->prepareLoading(dataSpec);
}


// static
DL_ERRORTYPE DefaultComponent::StartLoadingWrapper(
        DL_HANDLETYPE component,
        DataSpec* dataSpec) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->startLoading(dataSpec);
}

// static
DL_ERRORTYPE DefaultComponent::StopLoadingWrapper(
        DL_HANDLETYPE component) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->stopLoading();
}


// static
DL_ERRORTYPE DefaultComponent::PauseLoadingWrapper(
        DL_HANDLETYPE component) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->pauseLoading();
}


// static
DL_ERRORTYPE DefaultComponent::ResumeLoadingWrapper(
        DL_HANDLETYPE component) {
    DefaultComponent *me =
        (DefaultComponent *)
        ((DL_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->resumeLoading();
}

////////////////////////////////////////////////////////////////////////////////

DL_ERRORTYPE DefaultComponent::sendCommand(
        DL_COMMANDTYPE /* cmd */, DL_U32 /* param */, DL_PTR /* data */) {
    return DL_ErrorNone;
}
DL_ERRORTYPE DefaultComponent::getConfig(
        DL_INDEXTYPE /* index */, DL_PTR /* params */) {
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::setConfig(
        DL_INDEXTYPE /* index */, const DL_PTR /* params */) {
    return DL_ErrorNone;
}





DL_ERRORTYPE DefaultComponent::getState(DL_STATETYPE * /* state */) {
    //return DL_ErrorUndefined;
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::prepareLoading(
        DataSpec* /* dataSpec*/) {
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::startLoading(
        DataSpec* /* dataSpec*/) {
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::stopLoading() {
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::pauseLoading() {
    return DL_ErrorNone;
}

DL_ERRORTYPE DefaultComponent::resumeLoading() {
    return DL_ErrorNone;
}



LIYL_NAMESPACE_END
