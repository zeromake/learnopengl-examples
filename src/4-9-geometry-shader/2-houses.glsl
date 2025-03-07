@ctype vec4 HMM_Vec4

@vs vs
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains
out vec4 color;

layout(binding = 0) uniform texture2D _position_texture;
layout(binding = 0) uniform sampler position_texture_smp;
#define position_texture sampler2D(_position_texture, position_texture_smp)
layout(binding = 1) uniform texture2D _color_texture;
layout(binding = 1) uniform sampler color_texture_smp;
#define color_texture sampler2D(_color_texture, color_texture_smp)

void main() {
    uint vertexIndex = uint(gl_VertexIndex);
    uint pos_index = vertexIndex / 9;
    vec4 pos = texelFetch(position_texture, ivec2(pos_index, 0), 0);

    uint index = vertexIndex % 9;
    vec2 offset = index == 0 ? vec2(-0.2, 0.2) : vec2(0.0, 0.0);
    offset = index == 1 ? vec2(0.2, -0.2) : offset;
    offset = index == 2 ? vec2(-0.2, -0.2) : offset;
    offset = index == 3 ? vec2(-0.2, 0.2) : offset;
    offset = index == 4 ? vec2(0.2, 0.2) : offset;
    offset = index == 5 ? vec2(0.2, -0.2) : offset;
    offset = index == 6 ? vec2(-0.2, 0.2) : offset;
    offset = index == 7 ? vec2(0.0, 0.4) : offset;
    offset = index == 8 ? vec2(0.2, 0.2) : offset;
    gl_Position = vec4(pos.x + offset.x, pos.y + offset.y, 0.0, 1.0);

    color = texelFetch(color_texture, ivec2(pos_index, 0), 0);
    color = index == 7 ? vec4(1.0, 1.0, 1.0, 1.0) : color;
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program simple vs fs

