@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4

@vs vs
in vec3 aPos;
in vec2 aTexCoords;

out vec2 TexCoords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}
@end

@fs fs
in vec2 TexCoords;

out vec4 FragColor;

uniform texture2D _front_texture;
uniform sampler front_texture_smp;
#define front_texture sampler2D(_front_texture, front_texture_smp)
uniform texture2D _back_texture;
uniform sampler back_texture_smp;
#define back_texture sampler2D(_back_texture, back_texture_smp)

void main() {
    if(gl_FrontFacing)
        FragColor = texture(front_texture, TexCoords);
    else
        FragColor = texture(back_texture, TexCoords);
}
@end

@program simple vs fs
