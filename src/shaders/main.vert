//#version 330
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoord;
in vec3 aTangent;

uniform vs_params {
    mat4 projection;
};

uniform model_params {
    mat4 viewModel;
    mat4 normal;
    mat4 lightSpace;
};

out vec3 FragPos;
out vec2 TexCoord;
out vec4 FragPosLightSpace;
out mat3 TBN;

void main()
{
    FragPos = vec3(viewModel * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpace * vec4(aPos, 1.0);
    vec3 Normal = normalize(vec3(normal * vec4(aNormal, 0.0)));
    vec3 Tangent = normalize(vec3(normal * vec4(aTangent, 0.0)));
    vec3 Bitangent = normalize(cross(Normal, Tangent));
    TBN = mat3(Tangent, Bitangent, Normal);

    gl_Position = projection * viewModel * vec4(aPos, 1.0);
} 