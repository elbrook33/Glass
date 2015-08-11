#ifndef DRAW_H
#define DRAW_H

#include "UVRect.h"
#include "shaderProgram.h"
#include <X11/Xlib.h>
#include <map>
#include <math.h>
#define cameraAngle 60
#define cameraScale 0.866025404
#define SQRT3_2 0.866025404

class FBO {
public:
    GLuint texture, fb;
};

class Object {
public:
    GLuint  texture;
    UVRect  R;
    bool    above=true;
    int     position=0;
    bool    popup=false;
};

class Draw {
public:
    // Member variables
    Display*        display;
    Window          overlay, glxOverlay;
    float           width, height, sphereHeight,
                    winW, winH, B, rotation=0,
                    zScale, cameraX=0, cameraY=0, focusZ;
    ShaderProgram  *texUV, *UVClear,
                   *fastBlur, *lineUV, *lineDir, *glass, *dim,
                   *texDir, *dirClear, *sphereMap, *sphereUV;
    GLuint          bgSphere;
    FBO             below, temp;
    std::map<int,FBO> blur1;
    std::map<Window,Object> objects;
    // Constructor
    Draw(Display* display, Window overlay, int w, int h, float winw, float winh, float b);
    // Events
    void    add                 (Window id, char* data, int w, int h, int x, int y, int pos=0);
    void    remove              (Window id);
    Window  idAt                (int i);
    void    move                (Window id, int i);
    void    moveRest            (Window id);
    void    update              (Window id, char* data, int w, int h, int x, int y, int pos);
    void    update              ();
    void    resize              (Window id, int w, int h);
    void    setBlur             (float z);
    void    setAngle            (float angleX, float angleY);
    // Window
    void    windows             ();
    void    windows             (bool above);
    void    popups              ();
    void    window              (Object o);
    void    blurredWindow       (Object o);
    void    popup               (Object o);
    void    frame               (Object o);
    void    frameBorder         (Object o);
    void    content             (Object o);
    void    contentBorder       (Object o);
    void    bg                  (GLuint fb=0);
    void    blitBelow           ();
    void    clearBelow          ();
    void    clearTemp           (bool above);
    void    screen              ();
    // Primitives
    void    blur                (float z, bool above);
    void    face                (UVRect R);
    void    rect                (UVRect R, float r=0);
    void    line                (float x1, float y1, float z1, float x2, float y2, float z2, float minU=0.0, float maxU=1.0, float minV=0.0, float maxV=1.0);
    // Helpers
    void    loadFb              (GLuint fb);
    void    loadTexture         (GLuint texture, GLenum target=GL_TEXTURE_2D);
    void    loadShader          (ShaderProgram* shader);
    // Views
    void    defaultView         ();
    void    sphereView          ();
    void    fixedView           ();
    void    reducedView         (int factor);
    void    setView             (float w, float h, float angle, bool fixed=false, bool ortho=false);
    // Setup
    void    screenSetup         ();
    void    initializeShaders   ();
    void    createFramebuffers  ();
    FBO     newFbo              (int factor=1);
    GLuint  newTexture          (GLenum target=GL_TEXTURE_2D, bool repeat=false);
    int     loadTexture         (GLuint texture, const char* fileName);
    void    loadTexture         (GLuint texture, char* data, int w, int h, GLenum target=GL_TEXTURE_2D, GLenum format=GL_BGRA);
};

#endif // DRAW_H
