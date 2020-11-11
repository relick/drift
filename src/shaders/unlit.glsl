@module unlit

@ctype mat4 fMat4
@ctype vec3 fVec3

@vs vs
@include unlit.vert
@end

@fs fs
@include unlit.frag
@end

@program sg vs fs