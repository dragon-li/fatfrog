/****************************************************
 *
 *  @Author: liyl- liyunlong880325@gmail.com
 *  Last modified: 2018-08-12 13:14
 *  @Filename: EGLUtils.h
 *****************************************************/
#ifndef _EGLUTILS_H
#define _EGLUTILS_H

#include <stdint.h>
#include <stdlib.h>

#include <android/native_window.h>
#include "../../foundation/system/core/utils/Errors.h"
#include "../../prebuild/opengl/EGL/egl.h"


// ----------------------------------------------------------------------------
LIYL_NAMESPACE_START
// ----------------------------------------------------------------------------

class EGLUtils
{
    public:

        static inline const char *strerror(EGLint err);

        static inline status_t selectConfigForPixelFormat(
                EGLDisplay dpy,
                EGLint const* attrs,
                int32_t format,
                EGLConfig* outConfig);

        static inline status_t selectConfigForNativeWindow(
                EGLDisplay dpy,
                EGLint const* attrs,
                EGLNativeWindowType window,
                EGLConfig* outConfig);
};

// ----------------------------------------------------------------------------

const char *EGLUtils::strerror(EGLint err)
{
    switch (err){
        case EGL_SUCCESS:           return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:   return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:        return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:         return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:     return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG:        return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT:       return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:       return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH:         return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER:     return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE:       return "EGL_BAD_SURFACE";
        case EGL_CONTEXT_LOST:      return "EGL_CONTEXT_LOST";
        default: return "UNKNOWN";
    }
}

status_t EGLUtils::selectConfigForPixelFormat(
        EGLDisplay dpy,
        EGLint const* attrs,
        int32_t format,
        EGLConfig* outConfig)
{
    EGLint numConfigs = -1, n=0;

    if (!attrs)
        return BAD_VALUE;

    if (outConfig == NULL)
        return BAD_VALUE;

    // Get all the "potential match" configs...
    if (eglGetConfigs(dpy, NULL, 0, &numConfigs) == EGL_FALSE)
        return BAD_VALUE;

    EGLConfig* const configs = (EGLConfig*)malloc(sizeof(EGLConfig)*numConfigs);
    if (eglChooseConfig(dpy, attrs, configs, numConfigs, &n) == EGL_FALSE) {
        free(configs);
        return BAD_VALUE;
    }

    int i;
    EGLConfig config = NULL;
    for (i=0 ; i<n ; i++) {
        EGLint nativeVisualId = 0;
        eglGetConfigAttrib(dpy, configs[i], EGL_NATIVE_VISUAL_ID, &nativeVisualId);
        if (nativeVisualId>0 && format == nativeVisualId) {
            config = configs[i];
            break;
        }
    }

    free(configs);

    if (i<n) {
        *outConfig = config;
        return NO_ERROR;
    }

    return NAME_NOT_FOUND;
}

status_t EGLUtils::selectConfigForNativeWindow(
        EGLDisplay dpy,
        EGLint const* attrs,
        EGLNativeWindowType window,
        EGLConfig* outConfig)
{
    int err;
    int format;

    if (!window)
        return BAD_VALUE;

    if ((err = ANativeWindow_getFormat(window)) < 0) {
        return err;
    }

    return selectConfigForPixelFormat(dpy, attrs, format, outConfig);
}

// ----------------------------------------------------------------------------
LIYL_NAMESPACE_END
// ----------------------------------------------------------------------------

#endif /* _EGLUTILS_H */
