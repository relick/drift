//#version 330
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoord;

uniform vs_params {
    mat4 viewModel;
    mat4 normal;
    mat4 projection;
    mat4 lightSpace;
};

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

void main()
{
    FragPos = vec3(viewModel * vec4(aPos, 1.0));
    Normal = vec3(normal * vec4(aNormal, 0.0));
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpace * vec4(aPos, 1.0);

    gl_Position = projection * viewModel * vec4(aPos, 1.0);
} 