//#version 330

in vec3 TexCoords;

uniform samplerCube skybox;

uniform fs_params {
	vec3 sunDir;
};

out vec4 FragColour;

void main()
{
	float theta = dot(normalize(-sunDir),normalize(TexCoords));
	float innerCutoff = 0.998;
	float outerCutoff = 0.95;

	if(theta > innerCutoff)
	{
		FragColour = vec4(1.0);
	}
	else if (theta > outerCutoff)
	{
		float mixAmount = (outerCutoff - theta) / (outerCutoff - innerCutoff);
		mixAmount *= mixAmount;
		FragColour = mix(texture(skybox, TexCoords), vec4(1.0), mixAmount);
	}
	else
	{
		FragColour = texture(skybox, TexCoords);
	}
}