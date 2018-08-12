/*************************************************************************
  > File Name: AndroidOpenglRender.h
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2018年08月12日 星期日 12时22分01秒
 ************************************************************************/
#ifndef _ANDROID_OPENGL_RENDER_H
#define _ANDROID_OPENGL_RENDER_H
#include "../../prebuild/opengl/EGL/egl.h"
#include "../../prebuild/opengl/GLES2/gl2.h"
#include "../../prebuild/opengl/GLES2/gl2ext.h"

#include "EGLUtils.h"
#include "../api/IRender.h"

LIYL_NAMESPACE_START
class AndroidOpenglRender : public IRender {
    public:
        virtual status_t init(void* window , bool fullscreen, int32_t width, int32_t height);
        virtual void     destory();
        virtual void     setWxHAndCrop(const int32_t width, const int32_t height,const float left, const float top, const float right, const float bottom);
        virtual status_t renderFrame(void* data , int32_t size, int32_t yuvType);
        virtual          ~AndroidOpenglRender(); 
    private:
        void printGLString(const char* name,GLenum s);
        void checkEglError(const char* op, EGLBoolean returnVal = EGL_TRUE);
        void checkGlError(const char* op);
        GLuint loadShader(GLenum shaderType, const char* pSource);
        GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
        bool setupGraphics(int32_t w,int32_t h);
        void printEGLConfiguration(EGLDisplay dpy, EGLConfig config);
        bool printEGLConfigurations(EGLDisplay dpy);
        void initializeTexture(int32_t name , int32_t id , int32_t width, int32_t height);
        void setupTextures();

        GLuint mTextureIds[3];// texture id of Y,U,V texture
        GLint yTex, uTex ,vTex;
        GLuint mProgram;
        GLuint mvPositionHandle;
        GLuint mvTextureHandle;
        EGLContext mContext;
        void*  mWindow;
        EGLSurface mSurface;
        EGLDisplay mDisplay;
        EGLint mWidth;
        EGLint mHeight;
};
LIYL_NAMESPACE_END
#endif //_ANDROID_OPENGL_RENDER_H
