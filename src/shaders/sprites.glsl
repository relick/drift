@module sprites

@ctype mat4 Mat4
@ctype vec3 Vec3
@ctype vec2 Vec2

@block shared_block
@include common.glslh
@include sprites_constants.glslh
@end

@vs vs
@include_block shared_block
@include sprites.vert
@end

@fs fs
@include_block shared_block
@include sprites.frag
@end

@program sg vs fs