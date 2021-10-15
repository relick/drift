@module phong

@ctype mat4 Mat4
@ctype vec3 Vec3

@vs vs
@include phong.vert
@end

@fs fs
@include phong.frag
@end

@program sg vs fs