#include "UVRect.h"
#include "draw.h"

UVRect::UVRect(float X, float Y, float Z, float W, float H, float MinU, float MaxU, float MinV, float MaxV)
    : x(X), y(Y), z(Z), w(W), h(H), minU(MinU), maxU(MaxU), minV(MinV), maxV(MaxV) {
}
UVRect::UVRect(float scrX, float scrY, float Z, float W, float H, Draw* draw)
    : x(scrX+W/2-draw->width/2), y(draw->height/2-(scrY+H/2)), z(Z), w(W), h(H), minU(0), maxU(1), minV(0), maxV(1) {
}
UVRect::UVRect(float W, float H, Draw* draw)
    : x(0), y(0), z(-draw->zScale), w(W), h(H), minU(0), maxU(1), minV(0), maxV(1) {
}
#define xScale  0.95
#define yScale  0.95
#define nMax    5
UVRect::UVRect(int i, Draw* draw)
    : minU(0), maxU(1), minV(0), maxV(1) {
    if(i==0) {
        //
    } else {
        i--;
        float   totalW  = draw->width*xScale,
                startX  = -totalW/2,
                gap     = draw->B,
                rawW    = (totalW-(nMax-1)*gap)/nMax,
                scale   = draw->winW/rawW;
        w               = scale*rawW;
        h               = w*(draw->winH/draw->winW);
        x               = scale*(startX+i*(rawW+gap))+w/2;
        y               = scale*(draw->height/2*yScale)-h/2;
        z               =-scale*draw->zScale;
    }
}
UVRect::UVRect(Draw* draw)
    : x(0), y(0), z(-draw->zScale), w(draw->width), h(draw->height), minU(0), maxU(1), minV(0), maxV(1) {
}
UVRect UVRect::expand(float dr) {
    return UVRect(x, y, z, w+2*dr, h+2*dr, minU, maxU, minV, maxV);
}
UVRect UVRect::fullUV() {
    return UVRect(x, y, z, w, h, 0, 1, 0, 1);
}
UVRect UVRect::flipXY() {
    return UVRect(-x,-y, z,-w,-h, minU, maxU, minV, maxV);
}
UVRect UVRect::flipV() {
    return UVRect(x, y, z, w, h, minU, maxV, maxV, minV);
}
