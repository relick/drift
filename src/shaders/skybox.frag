//#version 330

in vec3 TexCoords;

uniform samplerCube skybox;

uniform fs_params {
	vec3 sunDir;
};

out vec4 FragColour;

vec3 MixInSun(in vec3 skyCol, in vec3 skyboxSample, in float starLevel, in vec3 nSunDir, in vec3 nVertexDir)
{
	float theta = dot(nSunDir,nVertexDir);
	float innerCutoff = 0.998;
	float outerCutoff = 0.95;
	float starOuterCutoff = 0.9;

	float outerMixLevel = 1.0 - (max(theta - starOuterCutoff, 0.0) / (1.0 - starOuterCutoff));
	outerMixLevel *= outerMixLevel;
	vec3 result = mix(skyCol, skyboxSample + skyCol, starLevel * outerMixLevel);

	if(theta > innerCutoff)
	{
		result = vec3(1.0);
	}
	else if (theta > outerCutoff)
	{
		float mixAmount = (outerCutoff - theta) / (outerCutoff - innerCutoff);
		mixAmount *= mixAmount;
		result = mix(result, vec3(1.0), mixAmount);
	}

	return result;
}

void main()
{
	vec3 blueSkyTop = vec3(0.0, 0.541, 1.0);
	vec3 blueSkyMid = vec3(0.0, 0.706, 1.0);
	vec3 darkNightTop = vec3(0.0);
	vec3 darkNightMid = vec3(0.0, 0.117, 0.380);
	
	vec3 nVertexDir = normalize(TexCoords);
	vec3 nSunDir = normalize(-sunDir);
	
	float sunLevel = 1.0 - max(nSunDir.y, 0.0);
	sunLevel *= sunLevel;
	sunLevel = 1.0 - sunLevel;
	vec3 top = mix(darkNightTop, blueSkyTop, sunLevel);
	vec3 mid = mix(darkNightMid, blueSkyMid, sunLevel);

	float skyLevel = abs(nVertexDir.y);
	vec3 col = mix(mid, top, skyLevel);

	float starLevel = max(skyLevel - sunLevel * 0.5, 0.0);

	vec3 skyboxSample = texture(skybox, TexCoords).rgb;
	vec3 result = MixInSun(col, skyboxSample, starLevel, nSunDir, nVertexDir);

	FragColour = vec4(result, 1.0);
}