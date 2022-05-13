//#version 330

// quad
in vec2 aPos;
in vec2 aUV;

// sprite data
in vec3 aSpritePos;
in vec2 aSpriteScale;
in float aSpriteRot;
in vec2 aTopLeftUV;
in vec2 aUVDims;
in vec2 aSpriteDims;
in uint aSpriteFlags;

uniform vs_params {
    mat4 projection;
};

out uint SpriteFlags;
out vec2 UV;

void main()
{
    if (aSpriteFlags != 0)
    {
       gl_Position = vec4(2.0, 0.0, 0.0, 1.0);
       return;
    }

    SpriteFlags = aSpriteFlags;
    UV = aTopLeftUV + (aUV * aUVDims);

    vec2 scaledPos = aSpriteScale * aSpriteDims * aPos;

    float cosRot = cos(aSpriteRot);
    float sinRot = sin(aSpriteRot);
    mat2 rotation = mat2(cosRot, -sinRot, sinRot, cosRot);
    vec2 rotatedPos = rotation * scaledPos;

    vec3 translatedPos = aSpritePos + vec3(rotatedPos, 0.0);

    gl_Position = projection * vec4(translatedPos, 1.0);
} 