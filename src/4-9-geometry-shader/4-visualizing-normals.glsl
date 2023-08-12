@ctype mat4 HMM_Mat4

@block data
float getVal(uint index, texture2D tex, sampler smp) {
    float x = index % 1024;
    float y = index / 1024;
    return texelFetch(sampler2D(tex, smp), ivec2(x, y), 0).r;
}

vec2 getVec2(uint index, texture2D tex, sampler smp) {
    float x = getVal(index, tex, smp);
    float y = getVal(index + 1, tex, smp);
    return vec2(x, y);
}

vec3 getVec3(uint index, texture2D tex, sampler smp) {
    float x = getVal(index, tex, smp);
    float y = getVal(index + 1, tex, smp);
    float z = getVal(index + 2, tex, smp);
    return vec3(x, y, z);
}
@end

@vs vs_simple
@include_block data
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains
out vec2 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

uniform texture2D _vertex_texture;
uniform sampler vertex_texture_smp;
#define vertex_texture sampler2D(_vertex_texture, vertex_texture_smp)

void main() {
    uint index = gl_VertexIndex * 5;
    vec4 pos = vec4(getVec3(index, _vertex_texture, vertex_texture_smp), 1.0);
    tex_coords = getVec2(index + 3, _vertex_texture, vertex_texture_smp);
    gl_Position = projection * view * model * pos;
    
}
@end

@fs fs_simple
in vec2 tex_coords;
out vec4 frag_color;

uniform texture2D _diffuse_texture;
uniform sampler diffuse_texture_smp;
#define diffuse_texture sampler2D(_diffuse_texture, diffuse_texture_smp)

void main() {
    frag_color = texture(diffuse_texture, tex_coords);
}
@end

@vs vs_normals
@include_block data
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

uniform texture2D _vertex_texture;
uniform sampler vertex_texture_smp;
#define vertex_texture sampler2D(_vertex_texture, vertex_texture_smp)

const float MAGNITUDE = 0.2;

vec3 getNormal(vec3 p0, vec3 p1, vec3 p2) {
    vec3 a = p0 - p1;
    vec3 b = p1 - p2;
    return normalize(cross(a, b));
}

void main() {
    uint vertexIndex = uint(gl_VertexIndex);
    uint index = vertexIndex >> 1;      // divide by 2
    index = index * 15;

    vec3 p0 = getVec3(index, _vertex_texture, vertex_texture_smp);
    vec3 p1 = getVec3(index + 5, _vertex_texture, vertex_texture_smp);
    vec3 p2 = getVec3(index + 10, _vertex_texture, vertex_texture_smp);

    vec3 mid = (p0 + p1 + p2) / 3.0;
    vec3 normal = getNormal(p0, p1, p2);
    vec3 direction = (vertexIndex % 2) * normal; 

    vec4 pos = vec4(mid + direction * MAGNITUDE, 1.0);
    gl_Position = projection * view * model * pos;
}
@end

@fs fs_normals
out vec4 frag_color;

void main() {
    frag_color = vec4(1.0, 1.0, 0.0, 1.0);
}
@end

@program simple vs_simple fs_simple
@program normals vs_normals fs_normals
