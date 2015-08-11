#ifndef BLUR_H
#define BLUR_H

const char* blurVert = R"GLSL(
uniform float   radius;
uniform int     direction;
uniform float   resolution;
varying vec2 uv, blur1, blur2, blur3, blur4, blur5, blur6;
void main(void) {
    gl_Position = ftransform();
    uv          = gl_MultiTexCoord0.st;
//    float off1 = 1.33 * radius / resolution;
    float off1 = 1.411764705882353  * radius / resolution;
    float off2 = 3.2941176470588234 * radius / resolution;
    float off3 = 5.176470588235294  * radius / resolution;
    if(direction==1) {
        blur1 = uv - vec2(off1, 0.0);
        blur2 = uv + vec2(off1, 0.0);
        blur3 = uv - vec2(off2, 0.0);
        blur4 = uv + vec2(off2, 0.0);
        blur5 = uv - vec2(off3, 0.0);
        blur6 = uv + vec2(off3, 0.0);
    } else {
        blur1 = uv - vec2(0.0, off1);
        blur2 = uv + vec2(0.0, off1);
        blur3 = uv - vec2(0.0, off2);
        blur4 = uv + vec2(0.0, off2);
        blur5 = uv - vec2(0.0, off3);
        blur6 = uv + vec2(0.0, off3);
    }
}
)GLSL";
const char* blurFrag = R"GLSL(
varying vec2 uv, blur1, blur2, blur3, blur4, blur5, blur6;
uniform sampler2D tex;
void main(void) {
    vec4 color  = vec4(0.0);
//    color += texture2D(tex, uv   ) * 0.29411764705882354;
//    color += texture2D(tex, blur1) * 0.35294117647058826;
//    color += texture2D(tex, blur2) * 0.35294117647058826;
//    gl_FragColor = color; 
    color += texture2D(tex, uv   ) * 0.1964825501511404;
    color += texture2D(tex, blur1) * 0.2969069646728344;
    color += texture2D(tex, blur2) * 0.2969069646728344;
    color += texture2D(tex, blur3) * 0.09447039785044732;
    color += texture2D(tex, blur4) * 0.09447039785044732;
    color += texture2D(tex, blur5) * 0.010381362401148057;
    color += texture2D(tex, blur6) * 0.010381362401148057;
    gl_FragColor = color;
}
)GLSL";

#endif // BLUR_H
