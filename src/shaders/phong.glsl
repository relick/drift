@module phong

@ctype mat4 hmm_mat4
@ctype vec3 hmm_vec3

@vs vs
@include phong.vert
@end

@fs fs
@include phong.frag
@end

@program sg vs fs