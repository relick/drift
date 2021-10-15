@module render_target_to_screen

@ctype mat4 Mat4
@ctype vec3 Vec3
@ctype vec2 Vec2

@vs vs
@include render_target_to_screen.vert
@end

@fs fs
@include render_target_to_screen.frag
@end

@program sg vs fs