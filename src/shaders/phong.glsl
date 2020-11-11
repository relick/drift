@module phong

@ctype mat4 fMat4
@ctype vec3 fVec3

@vs vs
@include phong.vert
@end

@fs fs
@include phong.frag
@end

@program sg vs fs