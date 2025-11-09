#include "skia_context.h"

//for NativeWindowType
#define WL_EGL_PLATFORM
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "tools/window/GLWindowContext.h"
#include "include/gpu/ganesh/gl/GrGLAssembleInterface.h"
#include "src/gpu/ganesh/gl/GrGLDefines.h"

#include "wayland-egl-core.h"

using skwindow::DisplayParams;
using skwindow::internal::GLWindowContext;

namespace {

class EGLWindowContext_wayland : public GLWindowContext {
public:
    EGLWindowContext_wayland(void *dpy, wl_egl_window *native_win, std::unique_ptr<const DisplayParams>);
    ~EGLWindowContext_wayland();

    void resize(int w, int h) override;

protected:
    void onSwapBuffers() override;
    sk_sp<const GrGLInterface> onInitializeContext() override;
    void onDestroyContext() override;

private:
    wl_egl_window *m_native = nullptr;
    // union{
    //     void* m_dpy;
        EGLDisplay m_Display;
    //};
    EGLContext m_EGLContext = EGL_NO_CONTEXT;
    EGLSurface m_EGLSurface = EGL_NO_SURFACE;
};

EGLWindowContext_wayland::EGLWindowContext_wayland(void* dpy, wl_egl_window *native_win,
                                               std::unique_ptr<const DisplayParams> params)
    : GLWindowContext(std::move(params)), m_native(native_win), m_Display(dpy)
{
    this->initializeContext();
}

EGLWindowContext_wayland::~EGLWindowContext_wayland()
{
    this->destroyContext();
    if (m_native) wl_egl_window_destroy(m_native);
}

void EGLWindowContext_wayland::resize(int w, int h)
{
    wl_egl_window_resize(m_native, w, h, 0, 0);
    fSurface.reset(nullptr);
}

sk_sp<const GrGLInterface> EGLWindowContext_wayland::onInitializeContext() {
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

    // We expect ANGLE to support this extension
    if (!eglGetPlatformDisplayEXT) {
        return nullptr;
    }

    m_Display = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT, m_Display, nullptr);
    if (EGL_NO_DISPLAY == m_Display) {
        return nullptr;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    if (!eglInitialize(m_Display, &majorVersion, &minorVersion)) {
        SkDebugf("Could not initialize display!\n");
        return nullptr;
    }
    EGLint numConfigs;
    fSampleCount = this->getDisplayParams()->msaaSampleCount();
    const int sampleBuffers = fSampleCount > 1 ? 1 : 0;
    const int eglSampleCnt = fSampleCount > 1 ? fSampleCount : 0;
    const EGLint configAttribs[] = {EGL_RENDERABLE_TYPE,
                                    // We currently only support ES3.
                                    EGL_OPENGL_ES3_BIT,
                                    EGL_RED_SIZE,   8,
                                    EGL_GREEN_SIZE, 8,
                                    EGL_BLUE_SIZE,  8,
                                    EGL_ALPHA_SIZE, 8,
                                    EGL_STENCIL_SIZE, 8,
                                    EGL_SAMPLE_BUFFERS, sampleBuffers,
                                    EGL_SAMPLES, eglSampleCnt,
                                    EGL_NONE};

    EGLConfig surfaceConfig;
    if (!eglChooseConfig(m_Display, configAttribs, &surfaceConfig, 1, &numConfigs)) {
        SkDebugf("Could not create choose config!\n");
        return nullptr;
    }
    // We currently only support ES3.
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    m_EGLContext = eglCreateContext(m_Display, surfaceConfig, nullptr, contextAttribs);
    if (EGL_NO_CONTEXT == m_EGLContext) {
        SkDebugf("Could not create context!\n");
        return nullptr;
    }
    m_EGLSurface = eglCreateWindowSurface(m_Display, surfaceConfig, m_native, nullptr);
    if (EGL_NO_SURFACE == m_EGLSurface) {
        SkDebugf("Could not create surface!\n");
        return nullptr;
    }
    if (!eglMakeCurrent(m_Display, m_EGLSurface, m_EGLSurface, m_EGLContext)) {
        SkDebugf("Could not make context current!\n");
        return nullptr;
    }

    sk_sp<const GrGLInterface> interface(GrGLMakeAssembledInterface(
            nullptr,
            [](void* ctx, const char name[]) -> GrGLFuncPtr { return eglGetProcAddress(name); }));
    if (interface) {
        interface->fFunctions.fClearStencil(0);
        interface->fFunctions.fClearColor(0, 0, 0, 0);
        interface->fFunctions.fStencilMask(0xffffffff);
        interface->fFunctions.fClear(GR_GL_STENCIL_BUFFER_BIT | GR_GL_COLOR_BUFFER_BIT);

        eglGetConfigAttrib(m_Display, surfaceConfig, EGL_STENCIL_SIZE, &fStencilBits);

        wl_egl_window_get_attached_size(m_native, &fWidth, &fHeight);

        interface->fFunctions.fViewport(0, 0, fWidth, fHeight);
    }
    return interface;
}

void EGLWindowContext_wayland::onSwapBuffers()
{
    if (!eglSwapBuffers(m_Display, m_EGLSurface)) {
        SkDebugf("Could not complete eglSwapBuffers.\n");
    }
}

void EGLWindowContext_wayland::onDestroyContext() {
    eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (EGL_NO_CONTEXT != m_EGLContext) {
        eglDestroyContext(m_Display, m_EGLContext);
    }
    if (EGL_NO_SURFACE != m_EGLSurface) {
        eglDestroySurface(m_Display, m_EGLSurface);
    }
    if (EGL_NO_DISPLAY != m_Display) {
        eglTerminate(m_Display);
    }
}

}  // anonymous namespace

namespace skwindow {

std::unique_ptr<WindowContext> MakeEGLForWayland(void *dpy, wl_egl_window *window,
                                                 std::unique_ptr<const DisplayParams> params)
{
    std::unique_ptr<WindowContext> ctx(new EGLWindowContext_wayland(
                                            dpy,
                                            window,
                                            std::move(params)));
    if (!ctx->isValid()) {
        return nullptr;
    }
    return ctx;
}

}  // namespace skwindow
