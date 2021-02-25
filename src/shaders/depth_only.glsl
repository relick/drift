@module depth_only

@ctype mat4 fMat4
@ctype vec3 fVec3
@ctype vec2 fVec2

@vs vs
@include depth_only.vert
@end

@fs fs
@include depth_only.frag
@end

@program sg vs fs