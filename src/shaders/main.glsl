@module main

@ctype mat4 hmm_mat4
@ctype vec3 fVec3Data
@ctype vec2 fVec2Data

@vs vs
@include main.vert
@end

@fs fs
@include main.frag
@end

@program sg vs fs