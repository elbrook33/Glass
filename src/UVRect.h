#ifndef UVRECT_H
#define UVRECT_H

class Draw;

class UVRect {
public:
    float x, y, z, w, h, minU, maxU, minV, maxV;
    UVRect() {}
    UVRect(float X, float Y, float Z, float W, float H, float MinU=0, float MaxU=1, float MinV=0, float MaxV=1);
    UVRect(float X, float Y, float Z, float W, float H, Draw* draw);
    UVRect(float W, float H, Draw* draw);
    UVRect(Draw* draw);
    UVRect(int i, Draw* draw);
    UVRect expand(float dr);
    UVRect fullUV();
    UVRect flipXY();
    UVRect flipV();
};

#endif // UVRECT_H
