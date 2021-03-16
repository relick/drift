//#version 330
in vec3 FragPos;
in vec2 TexCoord;
in vec4 FragPosLightSpace;
in mat3 TBN;

// material assumed to be layout(location=0)
uniform material {
    vec3 diffuseColour;
    vec3 specularColour;
    vec3 ambientColour;
    float shininess;
} Material;

uniform sampler2D mat_diffuseTex;
uniform sampler2D mat_specularTex;
uniform sampler2D mat_normalTex;

const int MAX_LIGHTS = 16;

uniform lights {
    vec3 ambient;
    float numLights;

    vec4 Col[MAX_LIGHTS];
    vec4 Pos[MAX_LIGHTS]; // if w is 0, then pos.xyz is -direction for a directional light
    vec4 Att[MAX_LIGHTS]; // constant, linear, quadratic

    // spotlight only, if point light then filled in to not matter
    vec4 Dir[MAX_LIGHTS]; // already negative
    vec4 Cut[MAX_LIGHTS]; // x is innerCutoff, y is outerCutoff
} Lights;

uniform sampler2DShadow directionalShadowMap;

out vec4 FragColour;


float CalcShadow(in vec4 fragPosLightSpace, in vec3 lightDir, in vec3 viewNormal)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
#if SOKOL_GLSL
    projCoords = projCoords * 0.5 + 0.5;
#else
    projCoords.x = 0.5 + (0.5 * projCoords.x);
    projCoords.y = 0.5 - (0.5 * projCoords.y);
#endif
    float bias = max(0.0005 * (1.0 - max(dot(viewNormal, lightDir), 0.0)), 0.00025);
    projCoords.z -= bias;
    
    // basic box style pcf
    float shadow = 0.0;
    if(clamp(projCoords.x, 0.0, 1.0) == projCoords.x &&
       clamp(projCoords.y, 0.0, 1.0) == projCoords.y &&
       projCoords.z > 0.0 &&
       projCoords.z <= 1.0
       )
    {
        vec2 texelSize = 1.0 / textureSize(directionalShadowMap, 0);
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                shadow += texture(directionalShadowMap, projCoords + vec3(vec2(x, y) * texelSize, 0)); 
            }    
        }
        shadow /= 9.0;
    }

    return shadow;
}

bool IsDirectionalLight(in int i)
{
    return Lights.Pos[i].w == 0.0;
}

// point, directional, spotlight, done in this. ambient done in main.
vec4 CalcLight(in int i, in vec4 matSpecular, inout float shadow, in vec3 viewNormal)
{
    vec4 diffuse = vec4(0.0);
    vec3 specular = vec3(0.0);

    float atten = 1.0;
    vec3 lightDir;

    float theta;

    // light type
    if(IsDirectionalLight(i))
    {
        // directional
        lightDir = Lights.Pos[i].xyz;
        theta = 1.1; // greater than all cutoffs
        shadow = CalcShadow(FragPosLightSpace, lightDir, viewNormal);
    }
    else
    {
        // point/spotlight
        lightDir = normalize(Lights.Pos[i].xyz - FragPos);
        float dist = length(Lights.Pos[i].xyz - FragPos);
        atten = 1.0 / (Lights.Att[i].x + Lights.Att[i].y * dist + Lights.Att[i].z * (dist * dist));
            
        theta = dot(lightDir, Lights.Dir[i].xyz);
        float epsilon = Lights.Cut[i].x - Lights.Cut[i].y;
        atten *= clamp((theta - Lights.Cut[i].y) / epsilon, 0.0, 1.0);
    }

    if(theta > Lights.Cut[i].y)
    {
        float diff = max(dot(viewNormal, lightDir), 0.0);
        diffuse = (vec4(Lights.Col[i].rgb, 1.0) * Lights.Col[i].a * diff) * atten;
        
        float specularIntensity = 0.0;
        if (Material.shininess > 0.0)
        {
            vec3 viewDir = normalize(-FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            vec3 reflectDir = reflect(-lightDir, viewNormal);

            float fresnelPower = 5.0; // schlick's approx val, could get from spec map?
    
            float fresnelFactor = dot(halfwayDir, viewDir); // or dot(norm, viewDir);
            fresnelFactor = pow(1.0 - max(fresnelFactor, 0.0), fresnelPower);

            vec3 matSpecular = mix(vec3(matSpecular) * Material.specularColour, vec3(1.0), fresnelFactor);
    
            float specularIntensity =
                //dot(viewDir, reflectDir) // phong
                dot(viewNormal, halfwayDir) // blinn
            ;
            specularIntensity = pow(max(specularIntensity, 0.0), Material.shininess);
                
            specular = (Lights.Col[i].rgb * Lights.Col[i].a * matSpecular * specularIntensity) * atten;
        }
    }

    return diffuse + vec4(specular, 1.0);
}

void main()
{
    vec3 viewNormal = texture(mat_normalTex, TexCoord).rgb;
    viewNormal = viewNormal * 2.0 - 1.0;
    viewNormal = normalize(TBN * viewNormal);
    
    vec4 diffuseSample = texture(mat_diffuseTex, TexCoord);
    if (diffuseSample.a < 0.01)
    {
        discard;
    }
    float gamma = 2.2;
    diffuseSample.rgb = pow(diffuseSample.rgb, vec3(gamma));

    vec4 matAmbient = vec4(Material.ambientColour, 1.0);
    vec4 matDiffuse = diffuseSample * vec4(Material.diffuseColour, 1.0);
    vec4 matSpecular = texture(mat_specularTex, TexCoord);

    float shadow = 0.0;
    vec4 result = vec4(0.0);

    for(int i = 0; i < int(Lights.numLights); ++i)
    {
        result += CalcLight(i, matSpecular, shadow, viewNormal);
    }

    vec4 ambient = vec4(Lights.ambient, 1.0) * matAmbient;
    
    result *= (1.0 - shadow);
    result += ambient;
    
    // finally gamma correct
    result *= matDiffuse;
    result.rgb = pow(result.rgb, vec3(1.0/gamma));

    FragColour = result;
}