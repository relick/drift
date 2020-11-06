//#version 330
in vec3 aPos;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
} 