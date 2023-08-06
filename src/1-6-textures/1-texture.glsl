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

uniform texture2D _ourTexture;
uniform sampler ourTexture_smp;
#define ourTexture sampler2D(_ourTexture, ourTexture_smp)

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
@end

@program simple vs fs
