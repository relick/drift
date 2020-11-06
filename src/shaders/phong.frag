//#version 330
in vec3 FragPos;
in vec3 Normal;

uniform fs_params {
    vec3 objectColor;
    vec3 lightColor;
    vec3 lightPos;
};

out vec4 FragColor;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 reflectDir = reflect(-lightDir, norm);

    float fresnelPower = 5.0; // schlick's approx val, could get from spec map?

    float fresnelFactor = dot(halfwayDir, viewDir); // or dot(norm, viewDir);
    fresnelFactor = pow(1.0 - max(fresnelFactor, 0.0), fresnelPower);

    float specularStrength = mix(0.0, 1.0, fresnelFactor);
    
    float specularIntensity =
        //dot(viewDir, reflectDir) // phong
        dot(norm, halfwayDir) // blinn
    ;
    specularIntensity = pow(max(specularIntensity, 0.0), 128.0);
    vec3 specular = specularStrength * specularIntensity * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}