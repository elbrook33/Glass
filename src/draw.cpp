#include "draw.h"
#include "UVRect.h"
#include "shaders.h"
#include "blur.h"
#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <time.h>
using namespace std;

/*
 * ===========
 * Constructor
 * ===========
 */
Draw::Draw(Display* d, Window o, int w, int h, float winw, float winh, float b)
  : display(d), overlay(o), width(w), height(h), winW(winw), winH(winh), B(b), zScale(cameraScale*h), focusZ(-zScale) {
    screenSetup         ();
    bgSphere            = newTexture(GL_TEXTURE_2D, true);
    sphereHeight        = loadTexture(bgSphere, "/opt/Glass/bg.png");
    initializeShaders   ();
    createFramebuffers  ();
    loadTexture         (below.texture, "/opt/Glass/bg.png");
    bg                  ();
}
/*
 * ======
 * Events
 * ======
 */
void Draw::add(Window id, char* data, int x, int y, int w, int h, int pos) {
    if(pos==0)  moveRest(id);
    Object      o;
    o.texture   = newTexture();
    loadTexture (o.texture, data, w, h);
    o.R         = UVRect(x, y,-zScale, w, h, this);
    o.popup     = pos==-1;
    objects[id] = o;
    windows     ();
    screen      ();
}
void Draw::remove(Window id) {
    glDeleteTextures(1, &objects[id].texture);
    objects.erase(id);
    windows     ();
    screen      ();
}
Window Draw::idAt(int i) {
    for(auto o:objects) {if(o.second.position==i) return o.first;}
    return 0;
}
void Draw::move(Window id, int i) {
    if(i==0)   {objects[id].R = UVRect(winW, winH, this); objects[id].above=true; moveRest(id);}
    else       {objects[id].R = UVRect(i, this); objects[id].above=false;}
    objects[id].position = i;
}
void Draw::moveRest(Window id) {
    int n = 1;
    for(auto i:objects) {
        if(i.first != id && !i.second.popup) {
            move(i.first, n);
            n++;
        }
    }
}
void Draw::update(Window id, char *data, int x, int y, int w, int h, int pos) {
    if(!objects.count(id)) {add(id, data, x, y, w, h, pos); return;}
    loadTexture(objects[id].texture, data, w, h);
    bool resized=false, moved=false;
    if(objects[id].above) {
        UVRect oldR     = objects[id].R;
        objects[id].R   = UVRect(x, y, -zScale, w, h, this);
        resized         = oldR.w != objects[id].R.w || oldR.h != objects[id].R.h;
        moved           = oldR.x != objects[id].R.x || oldR.y != objects[id].R.y;
    }
    if(objects[id].R.z == focusZ && !moved && !resized) {
        content(objects[id]);
        if(!objects[id].popup) popups();
    } else windows();
}
void Draw::update() {
    windows     ();
    screen      ();
}
void Draw::resize(Window id, int w, int h) {
    objects[id].R.w = w;
    objects[id].R.h = h;
}
#define bgDistance 2.6
void Draw::setBlur(float z) {
    focusZ      = -(bgDistance*z+1)*zScale;
}
#define rotationScale 2
void Draw::setAngle(float angleX, float angleY) {
    cameraX     = angleX*rotationScale;
    cameraY     = angleY*rotationScale;
    update      ();
}
/*
 * ======
 * Window
 * ======
 */
void Draw::windows() {
    windows     (false);
    blitBelow   ();
    windows     (true);
    popups      ();
}
void Draw::windows(bool above) {
    clearTemp   (above);
    float z     = 0;
    for(auto i:objects) {
        if(i.second.above==above && !i.second.popup) {window(i.second); z=i.second.R.z;}
    }
    if(z != focusZ) {
        blur        (z, above);
        loadTexture (temp.texture);
        loadFb      (above? 0 : below.fb);
        above?      fixedView() : sphereView();
        loadShader  (above? texUV : sphereMap);
        sphereMap   ->set("z", z);
        sphereMap   ->set("scale", abs(z/zScale));
        face        (UVRect(this));
    }
    defaultView ();
}
void Draw::popups() {
    for(auto i:objects) {
        if(i.second.popup && i.second.R.z==focusZ) popup(i.second);
    }
}
void Draw::window(Object o) {
    if(o.R.z!=focusZ) {blurredWindow(o); return;}
    loadFb      (0);
    frame       (o);
    frameBorder (o);
    content     (o);
    contentBorder(o);
}
void Draw::blurredWindow(Object o) {
    loadFb      (temp.fb);
    frame       (o);
    frameBorder (o);
    content     (o);
}
void Draw::popup(Object o) {
    content     (o);
    contentBorder(o);
}
void Draw::frame(Object o) {
    loadTexture (o.above? below.texture:bgSphere);
    loadShader  (glass);
    glass->set  ("resolution", o.R.expand(B).w);
    face        (o.R.expand(B));
}
void Draw::frameBorder(Object o) {
    loadTexture (o.above? below.texture:bgSphere);
    loadShader  (lineDir);
    rect        (o.R.expand(B));
}
void Draw::content(Object o) {
    loadTexture (o.texture);
    loadShader  (dim);
    dim->set    ("dim", o.above? 0.92:0.65);
    face        (o.R.flipV());
}
void Draw::contentBorder(Object o) {
    loadTexture (o.texture);
    loadShader  (lineUV);
    rect        (o.R.flipV());
}
void Draw::bg(GLuint fb) {
    defaultView ();
    loadTexture (bgSphere);
    loadFb      (fb);
    loadShader  (texDir);
    face        (UVRect(this));
}
void Draw::blitBelow() {
    fixedView   ();
    loadTexture (below.texture);
    loadFb      (0);
    loadShader  (texDir);
    face        (UVRect(this));
    defaultView ();
}
void Draw::clearTemp(bool above) {
    fixedView   ();
    glDisable   (GL_BLEND);
    loadShader  (above? UVClear:texDir);
    loadTexture (above? below.texture:bgSphere);
    loadFb      (temp.fb);
    face        (UVRect(this));
    glEnable    (GL_BLEND);
    defaultView ();
}
void Draw::screen() {
    loadFb(0);
    glFlush();
}
/*
 * ==========
 * Primitives
 * ==========
 */
#define dofFactor   0.0025
#define rFast       1.0
#define cut1        0.5
#define cut2        1.5
#define cut3        3.5
void Draw::blur(float z, bool above) {
    float r         = abs(z-focusZ)*dofFactor;
    float rDone     = 0;
    loadShader      (texUV);
    if(r>cut1) {
        reducedView (2);
        loadTexture (temp.texture);
        loadFb      (blur1[2].fb);
        if(above) {
        glDisable   (GL_BLEND);
        loadShader  (UVClear);
        face        (UVRect(this));
        glEnable    (GL_BLEND);
        }
        loadShader  (texUV);
        face        (UVRect(this));
        rDone       = cut1;
    }
    if(r>cut2) {
        reducedView (4);
        loadTexture (blur1[2].texture);
        loadFb      (blur1[4].fb);
        if(above) {
        glDisable   (GL_BLEND);
        loadShader  (UVClear);
        face        (UVRect(this));
        glEnable    (GL_BLEND);
        }
        loadShader  (texUV);
        face        (UVRect(this));
        rDone       = cut2;
    }
    if(r>cut3) {
        reducedView (8);
        loadTexture (blur1[4].texture);
        loadFb      (blur1[8].fb);
        if(above) {
        glDisable   (GL_BLEND);
        loadShader  (UVClear);
        face        (UVRect(this));
        glEnable    (GL_BLEND);
        }
        loadShader  (texUV);
        face        (UVRect(this));
        reducedView (4);
        loadTexture (blur1[8].texture);
        loadFb      (blur1[4].fb);
        face        (UVRect(this));
        rDone       = cut3;
    }
    if(r>cut2) {
        reducedView (2);
        loadTexture (blur1[4].texture);
        loadFb      (blur1[2].fb);
        face        (UVRect(this));
    }
    if(r>cut1) {
        reducedView (1);
        loadTexture (blur1[2].texture);
        loadFb      (temp.fb);
        face        (UVRect(this));
    }
    fixedView   ();
    loadTexture (temp.texture);
    loadFb      (blur1[1].fb);
    if(above) {
    glDisable   (GL_BLEND);
    loadShader  (UVClear);
    face        (UVRect(this));
    glEnable    (GL_BLEND);
    }
    loadShader  (fastBlur);
    fastBlur    ->set("radius", (r-rDone)*rFast);
    fastBlur    ->seti("direction", 1);
    fastBlur    ->set("resolution", width);
    face        (UVRect (this));
    loadTexture (blur1[1].texture);
    loadFb      (temp.fb);
    fastBlur    ->seti("direction", 2);
    fastBlur    ->set("resolution", height);
    face        (UVRect(this));
    defaultView ();
}
void Draw::face(UVRect R) {
    glBegin(GL_QUADS);
    glTexCoord2f(R.minU, R.minV); glVertex3f(R.x-R.w/2, R.y-R.h/2, R.z);
    glTexCoord2f(R.maxU, R.minV); glVertex3f(R.x+R.w/2, R.y-R.h/2, R.z);
    glTexCoord2f(R.maxU, R.maxV); glVertex3f(R.x+R.w/2, R.y+R.h/2, R.z);
    glTexCoord2f(R.minU, R.maxV); glVertex3f(R.x-R.w/2, R.y+R.h/2, R.z);
    glEnd();
}
void Draw::rect(UVRect R, float r) {
    line(R.x-R.w/2-r, R.y-R.h/2,   R.z, R.x+R.w/2+r, R.y-R.h/2,   R.z, R.minU, R.maxU, R.minV, R.minV);
    line(R.x+R.w/2,   R.y-R.h/2-r, R.z, R.x+R.w/2,   R.y+R.h/2+r, R.z, R.maxU, R.maxU, R.minV, R.maxV);
    line(R.x+R.w/2+r, R.y+R.h/2,   R.z, R.x-R.w/2-r, R.y+R.h/2,   R.z, R.maxU, R.minU, R.maxV, R.maxV);
    line(R.x-R.w/2,   R.y+R.h/2+r, R.z, R.x-R.w/2,   R.y-R.h/2-r, R.z, R.minU, R.minU, R.maxV, R.minV);
}
void Draw::line(float x1, float y1, float z1, float x2, float y2, float z2, float minU, float maxU, float minV, float maxV) {
    glBegin(GL_QUADS);
    glTexCoord2f(minU, minV); glNormal3f(y2-y1, x2-x1, 1); glVertex3f(x1, y1, z1);
    glTexCoord2f(minU, minV); glNormal3f(y1-y2, x1-x2,-1); glVertex3f(x1, y1, z1);
    glTexCoord2f(maxU, maxV); glNormal3f(y1-y2, x1-x2,-1); glVertex3f(x2, y2, z2);
    glTexCoord2f(maxU, maxV); glNormal3f(y2-y1, x2-x1, 1); glVertex3f(x2, y2, z2);
    glEnd();
}
/*
 * =======
 * Helpers
 * =======
 */
void Draw::loadFb(GLuint fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
}
void Draw::loadTexture(GLuint texture, GLenum target) {
    glBindTexture(target, texture);
}
void Draw::loadShader(ShaderProgram* shader) {
    shader->use();
}
/*
 * =====
 * Views
 * =====
 */
void Draw::defaultView() {
    setView(width, height, cameraAngle);
}
void Draw::sphereView() {
    setView(2*sphereHeight, sphereHeight, 60, false, false);
}
void Draw::fixedView() {
    setView(width, height, cameraAngle, true);
}
void Draw::reducedView(int factor) {
    setView(width/factor, height/factor, cameraAngle, true);
}
#define moveRatio   75
void Draw::setView(float w, float h, float angle, bool fixed, bool ortho) {
    glViewport          (0, 0, w, h);
    glMatrixMode        (GL_PROJECTION);
    glLoadIdentity      ();
    ortho==true?        glOrtho(-w/2,w/2, -h/2,h/2, 10,10000)
                       :gluPerspective(angle, w/h, 10, 10000);
    glMatrixMode        (GL_MODELVIEW);
    glLoadIdentity      ();
    gluLookAt           (0, 0, 0,
                         0, 0,-1,
                         0, 1, 0);
    if(!fixed) {
    glRotatef           (-cameraX, 0, 1, 0);
    glRotatef           (-cameraY, 1, 0, 0);
    glTranslatef        (-cameraX*moveRatio, cameraY*moveRatio, 0);
    }
}
/*
 * =====
 * Setup
 * =====
 */
void Draw::screenSetup() {
    int numConfigs;
    int fbAttributes[]      ={GLX_RENDER_TYPE,GLX_RGBA_BIT, GLX_DOUBLEBUFFER,false, None};
    GLXFBConfig* fbConfigs  = glXChooseFBConfig(display, DefaultScreen(display), fbAttributes, &numConfigs);
    glxOverlay              = glXCreateWindow(display, fbConfigs[0], overlay, NULL);
    GLXContext context      = glXCreateNewContext(display, fbConfigs[0], GLX_RGBA_TYPE, NULL, true);
    glXMakeCurrent          (display, glxOverlay, context);
    glewInit                ();
    glBlendFunc             (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable                (GL_BLEND);
    glClearColor            (0.317647059, 0.274509804, 0.243137255, 0.0);
}
void Draw::initializeShaders() {
    texUV       = new ShaderProgram(posVert, flatUV);
    UVClear     = new ShaderProgram(posVert, clearUV);
    dirClear    = new ShaderProgram(posVert, clearDir);
    texDir      = new ShaderProgram(fixedVert, flatten);
    glass       = new ShaderProgram(posVert, glassFrag);
    glass       ->addUniform("resolution");
    fastBlur    = new ShaderProgram(blurVert, blurFrag);
    fastBlur    ->addUniform("direction");
    fastBlur    ->addUniform("resolution");
    fastBlur    ->addUniform("radius");
    lineUV      = new ShaderProgram(lineVert, lineFragUV);
    lineUV      ->addUniform("thickness", 2);
    lineUV      ->addUniform("dim", 1);
    lineDir     = new ShaderProgram(lineVert, lineFragDir);
    lineDir     ->addUniform("thickness", 4);
    sphereMap   = new ShaderProgram(spherizeVert, spherize);
    sphereMap   ->addUniform("viewRatio", width/height);
    sphereMap   ->addUniform("z");
    sphereMap   ->addUniform("scale");
    sphereUV    = new ShaderProgram(sphereVert, flatUV);
    sphereUV    ->addUniform("shiftX");
    sphereUV    ->addUniform("shiftY");
    float* c    = (float*)malloc(4*sizeof(float));
    c[0]        = 0.317647059;
    c[1]        = 0.274509804;
    c[2]        = 0.243137255;
    c[3]        = 1.0;
    dim         = new ShaderProgram(posVert, dimFrag);
    dim         ->addUniform("dim");
    dim         ->addUniformfv("diffuse", 4, c);
    free        (c);
}
void Draw::createFramebuffers() {
    below       = newFbo();
    temp        = newFbo();
    blur1[1]    = newFbo(1);
    blur1[2]    = newFbo(2);
    blur1[4]    = newFbo(4);
    blur1[8]    = newFbo(8);
}
FBO Draw::newFbo(int factor) {
    FBO fbo;
    glGenFramebuffers       (1, &fbo.fb);
    fbo.texture             = newTexture();
    loadTexture             (fbo.texture, NULL, width/factor, height/factor);
    glBindFramebuffer       (GL_FRAMEBUFFER, fbo.fb);
    glFramebufferTexture2D  (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.texture, 0);
    bg                      (fbo.fb);
    return fbo;
}
GLuint Draw::newTexture(GLenum target, bool repeat) {
    GLuint id;
    glGenTextures   (1, &id);
    glBindTexture   (target, id);
    if(!repeat) {
    glTexParameterf (target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameterf (target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    }
    glTexParameterf (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return id;
}
int Draw::loadTexture(GLuint texture, const char *fileName) {
    int w, h, channels;
    char* data      = (char*)SOIL_load_image(fileName, &w, &h, &channels, false);
    loadTexture     (texture, data, w, h, GL_TEXTURE_2D, GL_RGBA);
    free            (data);
    return h;
}
void Draw::loadTexture(GLuint texture, char* data, int w, int h, GLenum target, GLenum format) {
    glBindTexture   (target, texture);
    glTexImage2D    (target, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, data);
}
