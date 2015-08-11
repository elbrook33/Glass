#include "draw.h"
#include "xpp.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#include <deque>
#include <tuple>
#include <iostream>
#include <thread>
#include <chrono>
#include <time.h>
#include <cmath>
using namespace std;
/*
 * =====================
 * Functions and globals
 * =====================
 */
void    initStep        (Xpp *xpp);
void    eventStep       (Xpp* xpp, XEvent event);
Draw*   draw;
bool    isBig           = false,
        wasOutside      = false;
int                     minW, minH, maxW, maxH, B;
time_t  t               = time(NULL);
deque<Window>           windows;
tuple<float,float>calculateSwivelAngles(float x, float y, float w, float h);
bool    onBorder        (int x, int y, int w, int h);
bool    inBg            (int x, int y, int w, int h);
void    moveToFront     (int i, Xpp *xpp, Draw *draw);
void    blurAnimation   (float start, float end, Xpp* xpp, Draw* draw);
void    setDimensions   (int w, int h);
int     indexOf         (Window id);
float   mix             (float a, float b, float d);
/*
 * ===============
 * Initializations
 * ===============
 */
int main( int args, char *argList[] ) {
    const char* display = (args==2? argList[1]:":0");
    Xpp                 xpp(display);
    setDimensions       (xpp.width, xpp.height);
    draw                = new Draw(xpp.display, xpp.overlay, xpp.width, xpp.height, minW, minH, B);
    xpp.start           (initStep, eventStep);
    delete              draw;
    return              0;
}
void initStep(Xpp *xpp) {
    int N; Window* win; tie(win,N) = xpp->getWindows();
    for(int i=0; i<N; i++) {
        if(xpp->isAWindow(win[i])) {
            windows.push_front  (win[i]);
            xpp->resizeAndCenter(win[i], minW, minH);
            XImage* image       = xpp->getImage(windows.front());
            if(image)           draw->add(windows.front(), image->data, (xpp->width-minW)/2, (xpp->height-minH)/2, image->width, image->height);
        }
    }
    XFree(win);
}
/*
 * ========================
 * Responding to X11 events
 * ========================
 */
void eventStep(Xpp *xpp, XEvent event) {
    switch(event.type) {
    case MapNotify:
    {
        if(xpp->isAWindow(event.xmap.window) || xpp->isADialog(event.xmap.window)) {
            windows.push_front  (event.xmap.window);
            if(!xpp->isFullscreen(event.xmap.window))
                xpp->resizeAndCenter(windows.front(), minW, minH);
            xpp->focus          (windows.front());
            // Should be resizing formerly frontmost window.
        }
    }
        break;
    case UnmapNotify:
    {
        int i = indexOf(event.xunmap.window);
        if (i!=-1) {
            windows.erase(windows.begin()+i);
        }
        if (i==0) {
            isBig = false;
            draw->move(windows.front(), 0);
        }
        draw->remove(event.xunmap.window);
        draw->update();
    }
        break;
    case ConfigureNotify:
    {
        int i = indexOf(event.xconfigure.window);
        switch(i) {
        case -1:
            break;
        case 0:
            if(!xpp->isFullscreen(event.xconfigure.window) &&
              (event.xconfigure.width > isBig?maxW:minW || event.xconfigure.height > isBig?maxH:minH)) {
                xpp->resizeAndCenter(windows[i], isBig?maxW:minW, isBig?maxH:minH);
            }
            break;
        default:
            if(event.xconfigure.width > minW || event.xconfigure.height > minH) {
                xpp->resizeAndCenter(windows[i], minW, minH);
            }
//            if(event.xconfigure.above == windows.front()) {
//                // This doesn't work.
//                moveToFront(i, xpp, draw);
//            }
        }
    }
        break;
    case ButtonPress:
    {
        if(onBorder(event.xbutton.x_root, event.xbutton.y_root, xpp->width, xpp->height)) {
            isBig                   = !isBig;
            xpp->resizeAndCenter    (windows.front(), isBig?maxW:minW, isBig?maxH:minH);
            // XWarpPointer
            // Needs full draw update once new image made.
        } else
        if(inBg(event.xbutton.x_root, event.xbutton.y_root, xpp->width, xpp->height)) {
            if(event.xbutton.y_root < xpp->height/4) {
                int i = event.xbutton.x_root/(xpp->width/5)+1;
                Window id = draw->idAt(i); i = indexOf(id);
                moveToFront(i, xpp, draw);
            }
        }
    }
        break;
    case MotionNotify:
        while(XCheckTypedEvent(xpp->display, MotionNotify, &event));
    {
        bool isOutside = inBg(event.xmotion.x_root, event.xmotion.y_root, xpp->width, xpp->height);
        if(isOutside) {
            if(!wasOutside) {
                blurAnimation(0, 1, xpp, draw);
                draw->setBlur(1);
                wasOutside = true;
            } else {
                float angleX, angleY;
                tie(angleX, angleY) = calculateSwivelAngles(event.xmotion.x_root, event.xmotion.y_root, xpp->width, xpp->height);
                draw->setAngle   (angleX, angleY);
            }
        } else {
            if(wasOutside) {
                blurAnimation(1, 0, xpp, draw);
                draw->setBlur(0);
                draw->setAngle(0,0);
                wasOutside = false;
            }
        }
    }
        break;
    default:
        if(event.type == xpp->damageEvent+XDamageNotify) {
            XDamageNotifyEvent* d   = (XDamageNotifyEvent*)&event;
            XImage* image           = xpp->getImage(d->drawable);
            if(!image) break;
            draw                    ->update(d->drawable, image->data, d->geometry.x, d->geometry.y, image->width, image->height, indexOf(d->drawable));
            if(!d->more) draw       ->screen();
        }
    }
}

/*
 * ================
 * Helper functions
 * ================
 */
bool onBorder(int x, int y, int w, int h) {
    int winW=isBig?maxW:minW, winH=isBig?maxH:minH,
        left=(w-winW)/2-B, right=w-left, top=(h-winH)/2-B, bottom=h-top;
    return x>=left && x<=right && y>=top && y<=bottom;
}
bool inBg(int x, int y, int w, int h) {
    int winW=isBig?maxW:minW, winH=isBig?maxH:minH,
        left=(w-winW)/2-B, right=w-left, top=(h-winH)/2-B, bottom=h-top;
    return x<left || x>right || y<top || y>bottom;
}
void moveToFront(int i, Xpp* xpp, Draw* draw) {
    xpp->resizeAndCenter(windows.front(), minW, minH);
    windows.push_front  (windows[i]);
    windows.erase       (windows.begin()+i+1);
    xpp->raise          (windows.front());
    xpp->focus          (windows.front());
    isBig               = false;
    draw->move          (windows.front(), 0);
    draw->update        ();
}
void blurAnimation(float start, float end, Xpp* xpp, Draw* draw) {
    float x, y;
    clock_t t = clock(), target = CLOCKS_PER_SEC/50; // 0.2 seconds in 10 frames
    for(float b=0; b<1; b+=0.1) {
        tie(x,y) = xpp->mouseXY();
        tie(x,y) = calculateSwivelAngles(x, y, xpp->width, xpp->height);
        draw->setBlur(b*(end-start)+start);
        draw->setAngle(x, y);
        t = clock()-t;
        this_thread::sleep_for(chrono::milliseconds(1000*(target-t)/CLOCKS_PER_SEC));
        t = clock();
    }
}
tuple<float,float> calculateSwivelAngles(float x, float y, float w, float h) {
    float winW=isBig?maxW:minW, winH=isBig?maxH:minH,
          left=(w-winW)/2-B, right=w-left, top=(h-winH)/2-B, bottom=h-top,
          dx=0, dy=0, rx=(x-w/2)/(w/2), ry=(y-h/2)/(h/2);
    if(x<left)   dx = (x-left)  /left;
    if(x>right)  dx = (x-right) /left;
    if(y<top)    dy = (y-top)   /top;
    if(y>bottom) dy = (y-bottom)/top;
    float angleX = mix(dx, rx/2, mix(dy*dy,ry*ry,dx*dx)),
          angleY = mix(dy, ry/2, mix(dx*dx,rx*rx,dy*dy));
    return tuple<float,float>(angleX, angleY);
}
#define maxWPercent     0.86
#define maxHPercent     0.96
#define minHPercent     0.8
#define minWPercentOfH  1.25
#define borderPercent   0.05
void setDimensions(int w, int h) {
    minH = h*minHPercent;
    minW = minH*minWPercentOfH;
    maxH = h*maxHPercent;
    maxW = w*maxWPercent;
    B    = h*borderPercent;
}
int indexOf(Window id) {
    int i=0;
    for(auto win : windows) {
        if(win==id) return i;
        i++;
    }
    return -1;
}
float mix(float a, float b, float d) {
    return (1-d)*a + d*b;
}
