@ctype vec2 HMM_Vec2
@ctype vec3 HMM_Vec3
@ctype mat4 HMM_Mat4

@vs vs_offscreen
in vec3 a_pos;
in vec2 a_tex_coords;
out vec2 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
    tex_coords = a_tex_coords;
}
@end

@vs vs_display
in vec2 a_pos;
in vec2 a_tex_coords;
out vec2 tex_coords;

void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
    tex_coords = a_tex_coords;
}
@end

@fs fs_offscreen
in vec2 tex_coords;
out vec4 frag_color;

uniform texture2D _diffuse_texture;
uniform sampler diffuse_texture_smp;
#define diffuse_texture sampler2D(_diffuse_texture, diffuse_texture_smp)

void main() {
    frag_color = texture(diffuse_texture, tex_coords);
}
@end

@fs fs_display
in vec2 tex_coords;
out vec4 frag_color;

uniform texture2D _diffuse_texture;
uniform sampler diffuse_texture_smp;
#define diffuse_texture sampler2D(_diffuse_texture, diffuse_texture_smp)

void main() {
    frag_color = vec4(vec3(1.0 - texture(diffuse_texture, tex_coords)), 1.0);
}
@end

@program offscreen vs_offscreen fs_offscreen
@program display vs_display fs_display
