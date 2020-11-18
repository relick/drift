@module render_target_to_screen

@ctype mat4 fMat4
@ctype vec3 fVec3
@ctype vec2 fVec2

@vs vs
@include render_target_to_screen.vert
@end

@fs fs
@include render_target_to_screen.frag
@end

@program sg vs fs