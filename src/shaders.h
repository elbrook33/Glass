#ifndef SHADERS_H
#define SHADERS_H
/*
 * ============
 * Vert shaders
 * ============
 */
const char* posVert = R"GLSL(
varying vec3        direction;
void main() {
    gl_Position     = ftransform();
    gl_TexCoord[0]  = gl_MultiTexCoord0;
    mat4 flipXY     = mat4(-1,0,0,0, 0,-1,0,0, 0,0,1,0, 0,0,0,1);
    direction       = (gl_ModelViewMatrix*gl_Vertex).xyz;
}
)GLSL";
const char* fixedVert = R"GLSL(
varying vec3        direction;
void main() {
    gl_Position     = gl_ProjectionMatrix*gl_Vertex;
    gl_TexCoord[0]  = gl_MultiTexCoord0;
    mat4 flipXY     = mat4(-1,0,0,0, 0,-1,0,0, 0,0,1,0, 0,0,0,1);
    direction       = (flipXY*gl_ModelViewMatrix*flipXY*vec4(gl_Vertex.xyz,0.0)).xyz;
}
)GLSL";
const char* sphereVert = R"GLSL(
#define M_PI        3.141592654
uniform float       shiftX;
uniform float       shiftY;
vec2 toA(vec3 pos) {
    return atan(pos.xy/pos.z);
}
void main() {
    float w         = ftransform().w;
    gl_Position     = vec4(toA(gl_Vertex.xyz)/M_PI*w, 0.5, w);
    gl_TexCoord[0]  = gl_MultiTexCoord0 + vec4(shiftX, shiftY, 0.0, 0.0);
}
)GLSL";
const char* spherizeVert = R"GLSL(
#define M_PI        3.141592654
varying vec2        angles;
varying float       w;
uniform float       z;
uniform float       scale;
vec2 toA(vec3 pos) {
    return atan(pos.xy/pos.z);
}
void main() {
    w               = ftransform().w;
    gl_Position     = vec4(toA(gl_Vertex.xyz)/M_PI*w, 0.5, w);
    gl_TexCoord[0]  = gl_MultiTexCoord0;
    mat4 flipXY     = mat4(-1,0,0,0, 0,-1,0,0, 0,0,1,0, 0,0,0,1);
    angles          = toA((flipXY*gl_ModelViewMatrix*flipXY*vec4(scale*gl_Vertex.xy, z, 1.0)).xyz);
}
)GLSL";
/*
 * ============
 * Frag shaders
 * ============
 */
const char* flatUV = R"GLSL(
uniform sampler2D   tex;
void main() {
    vec4 color      = texture2D(tex, gl_TexCoord[0].st);
    gl_FragColor    = color;
}
)GLSL";
const char* clearUV = R"GLSL(
uniform sampler2D   tex;
void main() {
    vec4 color      = texture2D(tex, gl_TexCoord[0].st);
    gl_FragColor    = vec4(color.rgb, 0.0);
}
)GLSL";
const char* clearDir = R"GLSL(
#define M_PI        3.141592654
uniform sampler2D   tex;
varying vec3        direction;
void main() {
    vec4 color      = texture2D(tex, atan(direction.xy/direction.z)/(2.0*M_PI) + 0.5);
    gl_FragColor    = vec4(color.rgb, 0.0);
}
)GLSL";
const char* dimFrag = R"GLSL(
uniform sampler2D   tex;
uniform vec4        diffuse;
uniform float       dim;
void main() {
    gl_FragColor    = vec4(mix(diffuse, texture2D(tex, gl_TexCoord[0].st), dim).rgb, 1.0);

}
)GLSL";
const char* flatten = R"GLSL(
#define M_PI        3.141592654
uniform sampler2D   tex;
varying vec3        direction;
void main() {
    gl_FragColor    = texture2D(tex, atan(direction.xy/direction.z)/(2.0*M_PI) + 0.5);
}
)GLSL";
const char* spherize = R"GLSL(
uniform sampler2D   tex;
varying vec2        angles;
varying float       w;
uniform float       z;
uniform float       viewRatio;
uniform float       scale;
void main() {
    vec2 uv         = z*tan(angles)/(2.0*scale*vec2(w,w/viewRatio)) + 0.5;
    gl_FragColor    = texture2D(tex, uv);
}
)GLSL";
const char* glassFrag = R"GLSL(
#define M_PI        3.141592654
#define dim         0.95
#define p           1.4
#define IOR         0.995
#define stretch     0.625
#define blurEdge    8.0
#define blurR       2.5
varying vec3        direction;
uniform sampler2D   tex;
uniform float       resolution;
vec2 sphereMap(vec3 dir) {
    return vec2( atan(dir.x/dir.z),
                 atan(dir.y/dir.z) )/(2.0*M_PI)+0.5;
}
vec4 curves(vec4 color) {
    return dim*pow(color,vec4(p));
}
void main() {
    float x = gl_TexCoord[0].x*2.0-1.0, y = gl_TexCoord[0].y*2.0-1.0;
    vec3 refracted  = refract(normalize(direction), vec3(0.0,0.0,1.0), IOR);
    vec2 uv         = sphereMap(refracted);
    vec4 color      = curves(texture2D(tex, uv));
    // Blur near edges
    float r = max(abs(x), abs(y));
    if(resolution-r*resolution<=blurEdge) {
        float delta = (blurEdge-(resolution-r*resolution))/blurEdge;
        float blur  = blurR/resolution*delta;
        refracted   = refract(direction, normalize(vec3(x*delta, y*delta, stretch)), IOR);
        uv          = sphereMap(refracted);
        color      += curves(texture2D(tex, uv + vec2( blur,-blur)));
        color      += curves(texture2D(tex, uv + vec2(-blur, blur)));
        color      /= 3.0;
    }
    gl_FragColor = vec4(color.rgb, 1.0);
}
)GLSL";
/*
 * ============
 * Line shaders
 * ============
 */
const char* lineVert = R"GLSL(
uniform float       thickness;
varying float       offset;
varying vec3        direction;
void main() {
    gl_Position     = vec4(ftransform().xy + 0.5*thickness*normalize(gl_Normal).xy, ftransform().zw);
    gl_TexCoord[0]  = gl_MultiTexCoord0;
    offset          = gl_Normal.z;
    direction       = (gl_ModelViewMatrix*gl_Vertex).xyz;
}
)GLSL";
const char* lineFragUV = R"GLSL(
uniform float dim;
uniform sampler2D texture;
varying float offset;
void main() {
    gl_FragColor = vec4(dim*texture2D(texture, gl_TexCoord[0].st).rgb, 1.0-abs(offset));
}
)GLSL";
const char* lineFragDir = R"GLSL(
#define M_PI        3.141592654
#define dim         0.9
#define p           2.4
uniform sampler2D   tex;
varying float       offset;
varying vec3        direction;
vec4 curves(vec4 color) {
    return dim*pow(color,vec4(p));
}
void main() {
    vec4 color      = texture2D(tex, vec2(  atan(direction.x/direction.z),
                                            atan(direction.y/direction.z) )/(2.0*M_PI)+0.5);
    float factor    = 1.0-abs(offset);
    gl_FragColor    = vec4(curves(color).rgb, factor);
}
)GLSL";

#endif // SHADERS_H
