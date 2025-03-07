@ctype mat4 HMM_Mat4

@vs vs
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains
out vec2 tex_coords;

layout(binding = 0) uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
    float time;
};

layout(binding = 0) uniform texture2D _vertex_texture;
layout(binding = 0) uniform sampler vertex_texture_smp;
#define vertex_texture sampler2D(_vertex_texture, vertex_texture_smp)

float getVal(uint index) {
    float x = index % 1024;
    float y = index / 1024;
    return texelFetch(vertex_texture, ivec2(x, y), 0).r;
}

vec2 getVec2(uint index) {
    float x = getVal(index);
    float y = getVal(index + 1);
    return vec2(x, y);
}

vec3 getVec3(uint index) {
    float x = getVal(index);
    float y = getVal(index + 1);
    float z = getVal(index + 2);
    return vec3(x, y, z);
}

vec3 getNormal(uint index) {
    vec3 p0 = getVec3(index);
    vec3 p1 = getVec3(index + 5);
    vec3 p2 = getVec3(index + 10);
    vec3 a = p0 - p1;
    vec3 b = p1 - p2;
    return normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 normal) {
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; 
    return position + vec4(direction, 0.0);
} 

void main() {
    uint index = gl_VertexIndex * 5;
    vec4 pos = vec4(getVec3(index), 1.0);
    tex_coords = getVec2(index + 3);

    uint f_index = index - index%15;
    vec3 normal = getNormal(f_index);
    pos = explode(pos, normal);
    gl_Position = projection * view * model * pos;
    
}
@end

@fs fs
in vec2 tex_coords;
out vec4 frag_color;

layout(binding = 1) uniform texture2D _diffuse_texture;
layout(binding = 1) uniform sampler diffuse_texture_smp;
#define diffuse_texture sampler2D(_diffuse_texture, diffuse_texture_smp)

void main() {
    frag_color = texture(diffuse_texture, tex_coords);
}
@end

@program phong vs fs

