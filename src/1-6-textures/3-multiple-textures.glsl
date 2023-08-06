@vs vs
in vec3 position;
in vec3 aColor;
in vec2 aTexCoord;
  
out vec3 ourColor;
out vec2 TexCoord;

void main() {
    gl_Position = vec4(position, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
@end

@fs fs
out vec4 FragColor;

in vec3 ourColor;
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
