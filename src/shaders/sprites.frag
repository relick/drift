//#version 330

flat in uint SpriteFlags;
in vec2 UV;

uniform sampler2D textureAtlas;

out vec4 FragColour;

void main()
{
	FragColour = texture(textureAtlas, UV);
}