//#version 330
in vec3 aPos;

uniform vs_params {
    mat4 view_model;
    mat4 projection;
};

void main()
{
    gl_Position = projection * view_model * vec4(aPos, 1.0);
} 