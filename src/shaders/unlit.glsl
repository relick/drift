@module unlit

@ctype mat4 hmm_mat4
@ctype vec3 hmm_vec3

@vs vs
@include unlit.vert
@end

@fs fs
@include unlit.frag
@end

@program sg vs fs