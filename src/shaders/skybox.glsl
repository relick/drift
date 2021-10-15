@module skybox

@ctype mat4 Mat4
@ctype vec3 Vec3
@ctype vec2 Vec2

@vs vs
@include skybox.vert
@end

@fs fs
@include skybox.frag
@end

@program sg vs fs