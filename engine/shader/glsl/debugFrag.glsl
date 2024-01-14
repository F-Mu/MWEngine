#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

void fragPrint(vec3 vec){
    if (gl_FragCoord.x<1&&gl_FragCoord.y<1)
    print(vec);
}