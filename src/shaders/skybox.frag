//#version 330

in vec3 TexCoords;

uniform samplerCube skybox;

out vec4 FragColour;

void main()
{
	FragColour = texture(skybox, TexCoords);
}