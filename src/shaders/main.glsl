@module main

@ctype mat4 Mat4
@ctype vec4 Vec4
@ctype vec3 Vec3
@ctype vec2 Vec2

@vs vs
@include main.vert
@end

@fs fs
@include main.frag
@end

@program sg vs fs