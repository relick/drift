@module sprites

@ctype mat4 fMat4
@ctype vec3 fVec3
@ctype vec2 fVec2

@vs vs
@include sprites.vert
@end

@fs fs
@include sprites.frag
@end

@program sg vs fs