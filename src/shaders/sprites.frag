//#version 330

in vec2 UV;

uniform sampler2D textureAtlas;

out vec4 FragColour;

void main()
{
	FragColour = texture(textureAtlas, UV);
}