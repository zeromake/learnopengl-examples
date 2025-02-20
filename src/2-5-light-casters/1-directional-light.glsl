@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4

@vs vs
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

layout(binding = 0) uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));
    // inverse tranpose is left out because:
    // (a) glsl es 1.0 (webgl 1.0) doesn't have inverse and transpose functions
    // (b) we're not performing non-uniform scale
    Normal = mat3(model) * aNormal;
    TexCoords = aTexCoords;
}
@end

@fs fs
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

layout(binding = 1) uniform fs_params {
    vec3 viewPos;
};

layout(binding = 2) uniform fs_material {
    float shininess;
} material;

layout(binding = 0) uniform fs_light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

layout(binding = 0) uniform texture2D _diffuse_texture;
layout(binding = 0) uniform sampler diffuse_texture_smp;
#define diffuse_texture sampler2D(_diffuse_texture, diffuse_texture_smp)
layout(binding = 1) uniform texture2D _specular_texture;
layout(binding = 1) uniform sampler specular_texture_smp;
#define specular_texture sampler2D(_specular_texture, specular_texture_smp)

void main() {
    // ambient
    vec3 ambient = light.ambient * vec3(texture(diffuse_texture, TexCoords));
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_texture, TexCoords));
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(specular_texture, TexCoords));  
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
@end

@program phong vs fs

