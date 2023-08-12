@ctype vec4 HMM_Vec4

@vs vs
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains

uniform texture2D _positions_texture;
uniform sampler positions_texture_smp;
#define positions_texture sampler2D(_positions_texture, positions_texture_smp)

void main() {
    uint vertexIndex = uint(gl_VertexIndex);
    uint index = vertexIndex >> 1;      // divide by 2
    vec4 pos = texelFetch(positions_texture, ivec2(index, 0), 0);

    pos.x = vertexIndex % 2 == 0 ? pos.x - 0.1 : pos.x + 0.1;
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}
@end

@fs fs
out vec4 frag_color;

void main() {
    frag_color = vec4(0.0, 1.0, 0.0, 1.0);
}
@end

@program simple vs fs
