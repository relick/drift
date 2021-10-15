@module sprites

@ctype mat4 Mat4
@ctype vec3 Vec3
@ctype vec2 Vec2

@vs vs
@include sprites.vert
@end

@fs fs
@include sprites.frag
@end

@program sg vs fs