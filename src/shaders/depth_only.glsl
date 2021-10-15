@module depth_only

@ctype mat4 Mat4
@ctype vec3 Vec3
@ctype vec2 Vec2

@vs vs
@include depth_only.vert
@end

@fs fs
@include depth_only.frag
@end

@program sg vs fs