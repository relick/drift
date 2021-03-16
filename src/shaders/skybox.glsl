@module skybox

@ctype mat4 fMat4
@ctype vec3 fVec3
@ctype vec2 fVec2

@vs vs
@include skybox.vert
@end

@fs fs
@include skybox.frag
@end

@program sg vs fs