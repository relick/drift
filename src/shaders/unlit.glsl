@module unlit

@ctype mat4 Mat4
@ctype vec3 Vec3

@vs vs
@include unlit.vert
@end

@fs fs
@include unlit.frag
@end

@program sg vs fs