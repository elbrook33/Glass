#ifndef XPP_H
#define XPP_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <unordered_map>
#include <deque>
#include <tuple>

typedef struct XppShm {
    XShmSegmentInfo* shmInfo;
    XImage*          imageBlock;
} XppShm;

class Xpp
{
public:
    Xpp                     (const char* displayName = ":0");
    ~Xpp                    ();
    Display*                display;
    int                     width, height, depth, damageEvent;
    Window                  root, overlay;
    std::unordered_map<Window,XppShm*>
                            images;
    std::deque<Window>      windows;
    // Loop
    void start              (void (*userInitStep)(Xpp*), void (*userEventStep)(Xpp*, XEvent));
    void eventsLoop         (void (*userEventStep)(Xpp*, XEvent));
    // Window lists
    void                    initializeCurrentWindows();
    std::tuple<Window*,int> getWindows          ();
    void                    addWindow           (Window id);
    void                    removeWindow        (Window id);
    void                    updateWindow        (Window id);
    // Window attributes
    Atom                    getWindowType       (Window id);
    bool                    isAWindow           (Window id);
    bool                    isADialog           (Window id);
    bool                    isFullscreen        (Window id);
    // Window images
    bool                    hasImage            (Window id);
    XImage*                 getImage            (Window id);
    XppShm*                 setupShm            (Window id);
    void                    removeShm           (XppShm* xppShm);
    // Manipulating windows
    void                    center              (Window id);
    void                    resizeAndCenter     (Window id, int w, int h);
    void                    focus               (Window id);
    void                    raise               (Window id);
    // Pointers and keys
    std::tuple<int,int>     mouseXY             ();
};

int errorHandler(Display* d, XErrorEvent* e);

#endif // XPP_H
