//#version 330
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform material {
    vec3 diffuseColour;
    vec3 specularColour;
    vec3 ambientColour;
    float shininess;
} Material;

uniform sampler2D mat_diffuseTex;
uniform sampler2D mat_specularTex;

uniform fs_params {
    vec3 lightColor;
    vec3 lightPos;
};

out vec4 FragColor;

void main()
{
    vec4 diffuseSample = texture(mat_diffuseTex, TexCoord);
    if (diffuseSample.a < 0.01)
    {
        discard;
    }
    vec4 matAmbient = diffuseSample * vec4(Material.ambientColour, 1.0);
    vec4 matDiffuse = diffuseSample * vec4(Material.diffuseColour, 1.0);

    float ambientStrength = 0.1;
    vec4 ambient = vec4(lightColor, 1.0) * ambientStrength * matAmbient;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = vec4(lightColor, 1.0) * diff * matDiffuse;
    
    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 reflectDir = reflect(-lightDir, norm);

    float fresnelPower = 5.0; // schlick's approx val, could get from spec map?
    
    float fresnelFactor = dot(halfwayDir, viewDir); // or dot(norm, viewDir);
    fresnelFactor = pow(1.0 - max(fresnelFactor, 0.0), fresnelPower);

    vec3 matSpecular = mix(vec3(texture(mat_specularTex, TexCoord)) * Material.specularColour, vec3(1.0), fresnelFactor);
    
    float specularIntensity =
        //dot(viewDir, reflectDir) // phong
        dot(norm, halfwayDir) // blinn
    ;
    if (Material.shininess > 0.0)
    {
        specularIntensity = pow(max(specularIntensity, 0.0), Material.shininess);
    }
    else
    {
        specularIntensity = 0.0;
    }
    vec3 specular = lightColor * matSpecular * specularIntensity;

    vec4 result = ambient + diffuse + vec4(specular, 1.0);
    FragColor = result;
}