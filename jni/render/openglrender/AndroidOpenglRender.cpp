/*************************************************************************
  > File Name: AndroidOpenglRender.cpp
  > Author: liyunlong
  > Mail: liyunlong_88@126.com 
  > Created Time: 2018年08月12日 星期日 13时05分10秒
 ************************************************************************/
#include "AndroidOpenglRender.h"
#include "../../foundation/system/core/utils/Log.h"

LIYL_NAMESPACE_START
void AndroidOpenglRender::printGLString(const char *name, GLenum s) {
    // fprintf(stderr, "printGLString %s, %d\n", name, s);
    const char *v = (const char *) glGetString(s);
    LLOGE("GL %s = %s\n", name, v);
}

void AndroidOpenglRender::checkEglError(const char* op, EGLBoolean returnVal) {
    if (returnVal != EGL_TRUE) {
        LLOGE("%s() returned %d\n", op, returnVal);
    }

    for (EGLint error = eglGetError(); error != EGL_SUCCESS; error
            = eglGetError()) {
        LLOGE("after %s() eglError %s (0x%x)\n", op, EGLUtils::strerror(error), error);
    }
}

void AndroidOpenglRender::checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LLOGE("after %s() glError (0x%x)\n", op, error);
    }
}

GLuint AndroidOpenglRender::loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LLOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
            } else {
                LLOGE("Guessing at GL_INFO_LOG_LENGTH size\n");
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    LLOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint AndroidOpenglRender::createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LLOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

bool AndroidOpenglRender::setupGraphics(int w, int h) {
#define TOSTRING(X) #X
    const char vertextShader[] = {
        "attribute vec4 aPosition;\n"
            "attribute vec2 aTextureCoord;\n"
            "varying vec2 vTextureCoord;\n"
            "void main() {\n"
            "  gl_Position = aPosition;\n"
            "  vTextureCoord = aTextureCoord;\n"
            "}\n" };

    // The fragment shader.
  // Do YUV to RGB565 conversion.
    const char fragmentShader[] = {
        "precision mediump float;\n"
            "uniform sampler2D Ytex;\n"
            "uniform sampler2D Utex,Vtex;\n"
            "varying vec2 vTextureCoord;\n"
            "void main(void) {\n"
            "  float nx,ny,r,g,b,y,u,v;\n"
            "  mediump vec4 txl,ux,vx;"
            "  nx=vTextureCoord[0];\n"
            "  ny=vTextureCoord[1];\n"
            "  y=texture2D(Ytex,vec2(nx,ny)).r;\n"
            "  u=texture2D(Utex,vec2(nx,ny)).r;\n"
            "  v=texture2D(Vtex,vec2(nx,ny)).r;\n"

            //"  y = v;\n"+
            "  y=1.1643*(y-0.0625);\n"
            "  u=u-0.5;\n"
            "  v=v-0.5;\n"

            "  r=y+1.5958*v;\n"
            "  g=y-0.39173*u-0.81290*v;\n"
            "  b=y+2.017*u;\n"
            "  gl_FragColor=vec4(r,g,b,1.0);\n"
            "}\n" };

#undef TOSTRING
    mProgram = createProgram(vertextShader, fragmentShader);
    if (!mProgram) {
        return false;
    }
    mvPositionHandle = glGetAttribLocation(mProgram, "aPosition");
    checkGlError("glGetAttribLocation");
    LLOGE("glGetAttribLocation(\"vPosition\") = %d\n",
            mvPositionHandle);
    mvTextureHandle = glGetUniformLocation(mProgram, "aTextureCoord");
    checkGlError("glGetUniformLocation");
    LLOGE("glGetUniformLocation(\"vTextureCoord\") = %d\n", mvTextureHandle);

    yTex = glGetUniformLocation(mProgram,"Ytex");
    checkGlError("glGetUniformLocation YTex");

    uTex = glGetUniformLocation(mProgram,"Utex");
    checkGlError("glGetUniformLocation UTex");

    vTex = glGetUniformLocation(mProgram,"Vtex");
    checkGlError("glGetUniformLocation VTex");

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

void AndroidOpenglRender::printEGLConfiguration(EGLDisplay dpy, EGLConfig config) {

#define X(VAL) {VAL, #VAL}
    struct {EGLint attribute; const char* name;} names[] = {
        X(EGL_BUFFER_SIZE),
        X(EGL_ALPHA_SIZE),
        X(EGL_BLUE_SIZE),
        X(EGL_GREEN_SIZE),
        X(EGL_RED_SIZE),
        X(EGL_DEPTH_SIZE),
        X(EGL_STENCIL_SIZE),
        X(EGL_CONFIG_CAVEAT),
        X(EGL_CONFIG_ID),
        X(EGL_LEVEL),
        X(EGL_MAX_PBUFFER_HEIGHT),
        X(EGL_MAX_PBUFFER_PIXELS),
        X(EGL_MAX_PBUFFER_WIDTH),
        X(EGL_NATIVE_RENDERABLE),
        X(EGL_NATIVE_VISUAL_ID),
        X(EGL_NATIVE_VISUAL_TYPE),
        X(EGL_SAMPLES),
        X(EGL_SAMPLE_BUFFERS),
        X(EGL_SURFACE_TYPE),
        X(EGL_TRANSPARENT_TYPE),
        X(EGL_TRANSPARENT_RED_VALUE),
        X(EGL_TRANSPARENT_GREEN_VALUE),
        X(EGL_TRANSPARENT_BLUE_VALUE),
        X(EGL_BIND_TO_TEXTURE_RGB),
        X(EGL_BIND_TO_TEXTURE_RGBA),
        X(EGL_MIN_SWAP_INTERVAL),
        X(EGL_MAX_SWAP_INTERVAL),
        X(EGL_LUMINANCE_SIZE),
        X(EGL_ALPHA_MASK_SIZE),
        X(EGL_COLOR_BUFFER_TYPE),
        X(EGL_RENDERABLE_TYPE),
        X(EGL_CONFORMANT),
    };
#undef X

    for (size_t j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
        EGLint value = -1;
        EGLint returnVal = eglGetConfigAttrib(dpy, config, names[j].attribute, &value);
        EGLint error = eglGetError();
        if (returnVal && error == EGL_SUCCESS) {
            LLOGD(" %s: %d (0x%x)", names[j].name , value, value);
        }
    }
}

bool AndroidOpenglRender::printEGLConfigurations(EGLDisplay dpy) {
    EGLint numConfig = 0;
    EGLint returnVal = eglGetConfigs(dpy, NULL, 0, &numConfig);
    checkEglError("eglGetConfigs", returnVal);
    if (!returnVal) {
        return false;
    }

    LLOGD("Number of EGL configuration: %d\n", numConfig);

    EGLConfig* configs = (EGLConfig*) malloc(sizeof(EGLConfig) * numConfig);
    if (! configs) {
        LLOGE("Could not allocate configs.\n");
        return false;
    }

    returnVal = eglGetConfigs(dpy, configs, numConfig, &numConfig);
    checkEglError("eglGetConfigs", returnVal);
    if (!returnVal) {
        free(configs);
        return false;
    }

    for(int i = 0; i < numConfig; i++) {
        LLOGD("Configuration %d\n", i);
        printEGLConfiguration(dpy, configs[i]);
    }

    free(configs);
    return true;
}

void AndroidOpenglRender::initializeTexture(int32_t name , int32_t id , int32_t width, int32_t height) {
    glActiveTexture(name);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0,
            GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
}

void AndroidOpenglRender::setupTextures() {
    const GLsizei width = mWidth;
    const GLsizei height= mHeight;
    glGenTextures(3,mTextureIds);//Generate th Y,U,V texture;
    initializeTexture(GL_TEXTURE0,mTextureIds[0],width, height);
    initializeTexture(GL_TEXTURE1,mTextureIds[1],width/2, height/2);
    initializeTexture(GL_TEXTURE2,mTextureIds[2],width/2, height/2);
    checkGlError("setupTextures");
}

////////////////////////////////////////////API/////////////////////////////

status_t AndroidOpenglRender::init(void* nativeWindow, bool fullscreen , int32_t width, int32_t height) {
    EGLBoolean returnValue;
    EGLConfig myConfig = {0};

    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLint s_configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE };
    EGLint majorVersion;
    EGLint minorVersion;
    EGLContext context;
    EGLSurface surface;
    EGLint w, h;

    EGLDisplay dpy;

    checkEglError("<init>");
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    checkEglError("eglGetDisplay");
    if (dpy == EGL_NO_DISPLAY) {
        LLOGE("eglGetDisplay returned EGL_NO_DISPLAY.\n");
        return UNKNOWN_ERROR;
    }

    returnValue = eglInitialize(dpy, &majorVersion, &minorVersion);
    checkEglError("eglInitialize", returnValue);
    LLOGD("EGL version %d.%d\n", majorVersion, minorVersion);
    if (returnValue != EGL_TRUE) {
        LLOGE("eglInitialize failed\n");
        return UNKNOWN_ERROR;
    }

    if (!printEGLConfigurations(dpy)) {
        LLOGE("printEGLConfigurations failed\n");
        return UNKNOWN_ERROR;
    }

    checkEglError("printEGLConfigurations");

    EGLNativeWindowType window = (EGLNativeWindowType)nativeWindow;
    returnValue = EGLUtils::selectConfigForNativeWindow(dpy, s_configAttribs, window, &myConfig);
    if (returnValue) {
        LLOGE("EGLUtils::selectConfigForNativeWindow() returned %d", returnValue);
        return UNKNOWN_ERROR;
    }

    mWindow = nativeWindow;
    checkEglError("EGLUtils::selectConfigForNativeWindow");

    LLOGD("Chose this configuration:\n");
    printEGLConfiguration(dpy, myConfig);

    surface = eglCreateWindowSurface(dpy, myConfig, window, NULL);
    checkEglError("eglCreateWindowSurface");
    if (surface == EGL_NO_SURFACE) {
        LLOGE("gelCreateWindowSurface failed.\n");
        return UNKNOWN_ERROR;
    }
    mSurface = surface;
    context = eglCreateContext(dpy, myConfig, EGL_NO_CONTEXT, context_attribs);
    checkEglError("eglCreateContext");
    if (context == EGL_NO_CONTEXT) {
        LLOGE("eglCreateContext failed\n");
        return UNKNOWN_ERROR;
    }
    mContext = context;
    returnValue = eglMakeCurrent(dpy, surface, surface, context);
    checkEglError("eglMakeCurrent", returnValue);
    if (returnValue != EGL_TRUE) {
        return UNKNOWN_ERROR;
    }
    mWidth = width;
    mHeight= height;

    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    checkEglError("eglQuerySurface");
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    checkEglError("eglQuerySurface");
    LLOGD("Window dimensions: %d x %d\n", w, h);

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    if(!setupGraphics(w,h)) {
        LLOGE("setupGraphics failed");
        return UNKNOWN_ERROR;
    }

    setupTextures();
    return OK;

}

void     AndroidOpenglRender::destory() {
}

void     AndroidOpenglRender::setWxHAndCrop(const int32_t width, const int32_t height,const float left, const float top, const float right, const float bottom) {
}

status_t AndroidOpenglRender::renderFrame(void* data , int32_t size, int32_t yuvType) {
    LLOGD("Render size = %ld", size);
    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    const GLfloat vertices[20] = {
        // X, Y, Z, U, V
        -1, -1, 0, 0, 1, // Bottom Left
        1, -1, 0, 1, 1, //Bottom Right
        1, 1, 0, 1, 0, //Top Right
        -1, 1, 0, 0, 0 }; //Top Left
    glVertexAttribPointer(mvPositionHandle, 3, GL_FLOAT, false, 5 * sizeof(GLfloat), vertices);
    checkGlError("glVertexAttribPointer aPosition");

    glEnableVertexAttribArray(mvPositionHandle);
    checkGlError("glEnableVertexAttribArray mvPositionHandle");

    glVertexAttribPointer(mvTextureHandle, 2, GL_FLOAT, false, 5 * sizeof(GLfloat), &vertices[3]);
    checkGlError("glVertexAttribPointer mvTextureHandle");

    glEnableVertexAttribArray(mvTextureHandle);
    checkGlError("glEnableVertexAttribArray mvTextureHandle");

    glUniform1i(yTex,0);
    checkGlError("glUniform1i yTex");

    glUniform1i(uTex,1);
    checkGlError("glUniform1i uTex");

    glUniform1i(vTex,2);
    checkGlError("glUniform1i vTex");

    return OK;

}

AndroidOpenglRender::~AndroidOpenglRender() {
}

LIYL_NAMESPACE_END

