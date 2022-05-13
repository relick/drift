//#version 330

flat in uint SpriteFlags;
in vec2 UV;

uniform sampler2D textureAtlas;

out vec4 FragColour;

void main()
{
    if (SpriteFlags != 0)
    {
       discard;
    }

	FragColour = texture(textureAtlas, UV);
}