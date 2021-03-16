//#version 330
in vec3 aPos;

uniform vs_params {
    mat4 untranslated_projView;
};

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;
    vec4 pos = untranslated_projView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
} 