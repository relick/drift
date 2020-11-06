//#version 330
in vec3 aPos;
in vec3 aNormal;

uniform vs_params {
    mat4 view_model;
    mat4 normal;
    mat4 projection;
};

out vec3 FragPos;
out vec3 Normal;

void main()
{
    gl_Position = projection * view_model * vec4(aPos, 1.0);
    FragPos = vec3(view_model * vec4(aPos, 1.0));
    Normal = vec3(normal * vec4(aNormal, 0.0));
} 