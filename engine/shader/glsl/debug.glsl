#extension GL_EXT_debug_printf : enable
void print(float x){
    debugPrintfEXT("float:%f\n",x);
}
void print(vec3 vec){
    debugPrintfEXT("vec3:%f,%f,%f\n", vec.x, vec.y, vec.z);
}
void print(vec4 vec){
    debugPrintfEXT("vec4:%f,%f,%f,%f\n", vec.x, vec.y, vec.z, vec.w);
}
void print(mat4 mat){
    debugPrintfEXT("mat4:%f,%f,%f,%f\n    %f,%f,%f,%f\n    %f,%f,%f,%f\n    %f,%f,%f,%f\n",
    mat[0][0], mat[0][1], mat[0][2], mat[0][3],
    mat[1][0], mat[1][1], mat[1][2], mat[1][3],
    mat[2][0], mat[2][1], mat[2][2], mat[2][3],
    mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}
void printPos(vec4 vec){
    debugPrintfEXT("vec4:%f,%f,%f,%f\n", vec.x/vec.w, vec.y/vec.w, vec.z/vec.w, vec.w);
}