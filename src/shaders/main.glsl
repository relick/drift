@module main

@ctype mat4 fMat4
@ctype vec4 fVec4
@ctype vec3 fVec3
@ctype vec2 fVec2

@vs vs
@include main.vert
@end

@fs fs
@include main.frag
@end

@program sg vs fs