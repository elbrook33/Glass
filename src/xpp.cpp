#include "xpp.h"
#include <X11/extensions/shape.h>       // For ShapeInput
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/Xatom.h>                  // For WINDOW_TYPE
#include <X11/Xutil.h>                  // For titles and app names
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
using namespace std;

/*
 * ===========
 * Constructor
 * ===========
 */
Xpp::Xpp(const char* displayName) {
    display                         = XOpenDisplay(displayName);
    root                            = XDefaultRootWindow(display);
    XWindowAttributes a;            XGetWindowAttributes(display, root, &a);
    width                           = a.width;
    height                          = a.height;
    depth                           = a.depth;
    overlay                         = XCompositeGetOverlayWindow(display, root);
    XserverRegion region            = XFixesCreateRegion(display, NULL, 0);
    XFixesSetWindowShapeRegion      (display, overlay, ShapeBounding, 0, 0, 0);
    XFixesSetWindowShapeRegion      (display, overlay, ShapeInput, 0, 0, region);
    XFixesDestroyRegion             (display, region);
    XCompositeRedirectSubwindows    (display, root, CompositeRedirectAutomatic);
    XSetErrorHandler                (errorHandler);
    int err; XDamageQueryExtension  (display, &damageEvent, &err);
}
Xpp::~Xpp() {
    for(auto i:images) {removeWindow(i.first);}
    XCloseDisplay(display);
}
/*
 * ============
 * Window lists
 * ============
 */
tuple<Window*,int> Xpp::getWindows() {
    Window *windows, rootReturn, parentReturn; unsigned int N;
    XQueryTree(display, root, &rootReturn, &parentReturn, &windows, &N);
    return tuple<Window*,int>(windows, N);
}
void Xpp::addWindow(Window id) {
    XWindowAttributes a;
    if( images.count(id)                        ||
       !XGetWindowAttributes(display, id, &a)   ||
        a.map_state != IsViewable)
        return;
    images[id] = setupShm(id);
    XDamageCreate(display, id, XDamageReportNonEmpty);
}
void Xpp::removeWindow(Window id) {
    if(!images.count(id)) return;
    removeShm(images[id]);
    images.erase(id);
}
void Xpp::updateWindow(Window id) {
    if(!images.count(id)) return;
    XWindowAttributes a;
    if( XGetWindowAttributes(display, id, &a) &&
       (a.width  == images[id]->imageBlock->width &&
        a.height == images[id]->imageBlock->height)) {
        cout << "Image sizes already the same" << endl;
        return;
    }
    removeShm(images[id]);
    images[id] = setupShm(id);
}
/*
 * =================
 * Window attributes
 * =================
 */
Atom Xpp::getWindowType(Window id) {
    Atom windowTypeFlag = XInternAtom( display, "_NET_WM_WINDOW_TYPE", false );
    Atom returnType; int returnFormat; unsigned long numItems, itemsLeft; unsigned char *data;
    XGetWindowProperty  (display, id, windowTypeFlag, 0L, 1L, false, XA_ATOM, &returnType, &returnFormat, &numItems, &itemsLeft, &data);
    Atom* atoms         = (Atom*)data;
    Atom  type          = numItems > 0? atoms[0] : 0;
    XFree(atoms);
    return type;
}
bool Xpp::isAWindow(Window id) {
    XWindowAttributes a;
    if(!XGetWindowAttributes(display, id, &a)) return false;
    if(a.map_state != IsViewable || a.override_redirect != 0) return false;
    Atom windowType     = getWindowType(id);
    Atom normalTypeFlag = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", false);
    return windowType == normalTypeFlag
        || windowType == 0;
}
bool Xpp::isADialog(Window id) {
    XWindowAttributes a;
    if(!XGetWindowAttributes(display, id, &a)) return false;
    if(a.map_state != IsViewable || a.override_redirect != 0) return false;
    Atom windowType     = getWindowType(id);
    Atom dialogTypeFlag = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", false);
    return windowType == dialogTypeFlag;
}
bool Xpp::isFullscreen(Window id) {
    XWindowAttributes a;
    return XGetWindowAttributes(display, id, &a)
        && a.x == 0 && a.y == 0 && a.width == width && a.height == height;
}
/*
 * =============
 * Window images
 * =============
 */
bool Xpp::hasImage(Window id) {
    XWindowAttributes a;
    return images.count(id) && XGetWindowAttributes(display, id, &a)
        && a.width  == images[id]->imageBlock->width
        && a.height == images[id]->imageBlock->height;
}
XImage* Xpp::getImage(Window id) {
    if(!hasImage(id)) {cout<<"Invalid shmImage (doesn't exist or wrong size)"<<endl; return NULL;}
    XShmGetImage(display, id, images[id]->imageBlock, 0, 0, AllPlanes);
    return images[id]->imageBlock;
}
/*
 * ===================
 * Configuring windows
 * ===================
 */
void Xpp::center(Window id) {
    XWindowAttributes a;
    if(XGetWindowAttributes(display, id, &a)) XMoveWindow(display, id, (width-a.width)/2, (height-a.height)/2);
}
void Xpp::resizeAndCenter(Window id, int w, int h) {
    XMoveResizeWindow(display, id, (width-w)/2, (height-h)/2, w, h);
}
void Xpp::focus(Window id) {
    XSetInputFocus(display, id, RevertToPointerRoot, CurrentTime);
}
void Xpp::raise(Window id) {
    XRaiseWindow(display, id);
}
/*
 * =================
 * Pointers and keys
 * =================
 */
tuple<int,int> Xpp::mouseXY() {
    Window rootReturn, childReturn; int rootX, rootY, winX, winY; unsigned int maskReturn;
    if(XQueryPointer(display, root, &rootReturn, &childReturn, &rootX, &rootY, &winX, &winY, &maskReturn)) return tuple<int,int>(rootX,rootY);
    return tuple<int,int>(-1,-1);
}
/*
 * ====
 * Loop
 * ====
 */
void Xpp::start(void (*userInitStep)(Xpp *), void (*userEventStep)(Xpp*, XEvent)) {
    XSelectInput            (display, root, SubstructureNotifyMask|PointerMotionMask|ButtonPressMask);
    initializeCurrentWindows();
    userInitStep            (this);
    eventsLoop              (userEventStep);
}
void Xpp::initializeCurrentWindows() {
    Window*         windows;
    unsigned int    N;
    tie(windows,N)  = getWindows();
    for(unsigned int i=0; i<N; i++) { addWindow(windows[i]); }
    XFree(windows);
}
void Xpp::eventsLoop(void (*userEventStep)(Xpp*, XEvent)) {
    XEvent event;
    while(true) {
        XNextEvent(display, &event);
        switch(event.type) {
        case MapNotify:
            cout << "MapNotify" << endl;
            addWindow(event.xmap.window);
            break;
        case UnmapNotify:
            cout << "UnmapNotify" << endl;
            removeWindow(event.xunmap.window);
            break;
        case ConfigureNotify:
            cout << "ConfigureNotify" << endl;
            updateWindow(event.xconfigure.window);
            break;
        case CreateNotify:
            cout << "CreateNotify" << endl;
            break;
        case DestroyNotify:
            cout << "DestroyNotify" << endl;
            break;
        case EnterNotify:
            cout << "EnterNotify" << endl;
            break;
        case LeaveNotify:
            cout << "LeaveNotify" << endl;
            break;
        case ClientMessage:
            cout << "ClientMessage" << endl;
            break;
        case ButtonPress:
            cout << "ButtonPress" << endl;
            break;
        case KeyPress:
            cout << "KeyPress: " << event.xkey.keycode << endl;
            break;
        case MotionNotify:
            break;
        default:
            if(event.type==damageEvent+XDamageNotify) {
                XDamageNotifyEvent* damage = (XDamageNotifyEvent*)&event;
                XDamageSubtract(display, damage->damage, None, None);
            } else {
                cout << "Other: " << event.type << endl;
            }
            break;
        }
        userEventStep(this, event);
    }
}
/*
 * =============================
 * Shared memory (window images)
 * =============================
 */
XppShm* Xpp::setupShm(Window id) {
    XShmSegmentInfo*   shmInfo = new XShmSegmentInfo();
    XWindowAttributes  a; if(!XGetWindowAttributes(display, id, &a)) return NULL;
    XImage* image      = XShmCreateImage(display, CopyFromParent, depth, ZPixmap, NULL, shmInfo, a.width, a.height);
    shmInfo->shmid     = shmget(IPC_PRIVATE, image->bytes_per_line*image->height, IPC_CREAT|0777);
    shmInfo->shmaddr   = image->data = (char*)shmat(shmInfo->shmid, 0, 0);
    shmInfo->readOnly  = false;
    cout << "shm ready. XShmAttach" << endl;
    XShmAttach         (display, shmInfo);
    cout << "shm attached" << endl;
    XppShm* xppShm     = new XppShm();
    xppShm->shmInfo    = shmInfo;
    xppShm->imageBlock = image;
    return xppShm;
}
void Xpp::removeShm(XppShm* xppShm) {
    XShmDetach   (display, xppShm->shmInfo);
    XDestroyImage(xppShm->imageBlock);
    cout << "shm detached" << endl;
    shmdt        (xppShm->shmInfo->shmaddr);
    shmctl       (xppShm->shmInfo->shmid, IPC_RMID, 0);
    delete        xppShm->shmInfo;
    delete        xppShm;
    cout << "shm deleted" << endl;
}
/*
 * ==============
 * Error handling
 * ==============
 */
int errorHandler(Display* d, XErrorEvent* e) {
    int length = 100;
    char* text = (char*)malloc(sizeof(char)*length);
    cout<<"X11 error caught. Request: "<<(int)e->request_code<<" ("<<(int)e->error_code<<":"<<(int)e->minor_code<<")"<<endl;
    XGetErrorText(d, e->error_code,   text, length);    cout<<"Error code: "<<text<<endl;
    XGetErrorText(d, e->minor_code,   text, length);    cout<<"Minor code: "<<text<<endl;
    free(text);
    return e->error_code;
}
