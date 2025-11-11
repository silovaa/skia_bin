#ifndef SKIA_CONTEXT_H
#define SKIA_CONTEXT_H

#include <memory>

class wl_surface;

namespace skwindow {

class WindowContext;
class DisplayParams;

std::unique_ptr<WindowContext> MakeEGLForWayland(void *dpy, wl_surface *surf, int w, int h,
                                                 std::unique_ptr<const DisplayParams>);
}  // namespace skwindow

#endif //  SKIA_CONTEXT_H
