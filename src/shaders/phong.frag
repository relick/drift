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
    
    float specularStrength = 0.5;
    
    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 reflectDir = reflect(-lightDir, norm);

    float specularIntensity =
        //dot(viewDir, reflectDir) // phong
        dot(norm, halfwayDir) // blinn
    ;
    specularIntensity = pow(clamp(specularIntensity, 0.0, 1.0), 128.0);
    vec3 specular = specularStrength * specularIntensity * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}