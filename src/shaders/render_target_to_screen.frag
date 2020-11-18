//#version 330
in vec2 TexCoord;

uniform sampler2D tex;

out vec4 FragColor;

void main()
{
#if SOKOL_GLSL
    FragColor = texture(tex, vec2(TexCoord.x, -TexCoord.y));
#else
    FragColor = texture(tex, TexCoord);
#endif
}