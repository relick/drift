//#version 330
in vec3 aPos;

uniform vs_params {
    mat4 projViewModel;
};

void main()
{
    gl_Position = projViewModel * vec4(aPos, 1.0);
} 