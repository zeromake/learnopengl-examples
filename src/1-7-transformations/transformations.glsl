@ctype mat4 HMM_Mat4

@vs vs
in vec3 position;
in vec2 aTexCoord;
  
out vec2 TexCoord;

uniform vs_params {
    mat4 transform;
};

void main() {
    gl_Position = transform * vec4(position, 1.0);
    TexCoord = aTexCoord;
}
@end

@fs fs
out vec4 FragColor;

in vec2 TexCoord;

uniform texture2D _texture1;
uniform sampler texture1_smp;
#define texture1 sampler2D(_texture1, texture1_smp)
uniform texture2D _texture2;
uniform sampler texture2_smp;
#define texture2 sampler2D(_texture2, texture2_smp)

void main() {
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}
@end

@program simple vs fs
