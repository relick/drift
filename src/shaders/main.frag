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
    vec3 diffuseSample = vec3(texture(mat_diffuseTex, TexCoord));
    vec3 matAmbient = diffuseSample * Material.ambientColour;
    vec3 matDiffuse = diffuseSample * Material.diffuseColour;

    float ambientStrength = 0.1;
    vec3 ambient = lightColor * ambientStrength * matAmbient;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * matDiffuse;
    
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
    specularIntensity = pow(max(specularIntensity, 0.0), Material.shininess);
    vec3 specular = lightColor * matSpecular * specularIntensity;

    vec4 result = vec4(ambient + diffuse + specular, 1.0);
    FragColor = result;
}