//#version 330
in vec3 aPos;
in vec2 aTexCoord;

uniform vs_params {
    float aspectMult;
};

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos.x * aspectMult, aPos.y, aPos.z, 1.0);
    TexCoord = aTexCoord;
} 