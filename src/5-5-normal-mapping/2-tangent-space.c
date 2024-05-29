//------------------------------------------------------------------------------
//  Normal Mapping (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "2-tangent-space.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    HMM_Vec3 light_pos;
    uint8_t file_buffer[1024 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

HMM_Vec3 computeTangent(HMM_Vec3 pos0, HMM_Vec3 pos1, HMM_Vec3 pos2, HMM_Vec2 uv0, HMM_Vec2 uv1, HMM_Vec2 uv2) {
    HMM_Vec3 edge0 = HMM_SubV3(pos1, pos0);
    HMM_Vec3 edge1 = HMM_SubV3(pos2, pos0);
    HMM_Vec2 delta_uv0 = HMM_SubV2(uv1, uv0);
    HMM_Vec2 delta_uv1 = HMM_SubV2(uv2, uv0);

    float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);

    float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_V3(x, y, z);
}

static void init(void) {
    lopgl_setup();

    state.light_pos = HMM_V3(.5f, 1.f, .3f);

    /* positions */
    HMM_Vec3 pos0 = HMM_V3(-1.f,  1.f, 0.f);
    HMM_Vec3 pos1 = HMM_V3(-1.f, -1.f, 0.f);
    HMM_Vec3 pos2 = HMM_V3( 1.f, -1.f, 0.f);
    HMM_Vec3 pos3 = HMM_V3( 1.f,  1.f, 0.f);
    /* texture coordinates */
    HMM_Vec2 uv0 = HMM_V2(0.f, 1.f);
    HMM_Vec2 uv1 = HMM_V2(0.f, 0.f);
    HMM_Vec2 uv2 = HMM_V2(1.f, 0.f);  
    HMM_Vec2 uv3 = HMM_V2(1.f, 1.f);
    /* normal vector */
    HMM_Vec3 nm = HMM_V3(0.f, 0.f, 1.f);
    /* tangents */
    HMM_Vec3 ta0 = computeTangent(pos0, pos1, pos2, uv0, uv1, uv2);
    HMM_Vec3 ta1 = computeTangent(pos0, pos2, pos3, uv0, uv2, uv3);

    /* we can compute the bitangent in the vertex shader by taking 
       the cross product of the normal and tangent */
    float quad_vertices[] = {
        // positions            // normal         // texcoords  // tangent           
        pos0.X, pos0.Y, pos0.Z, nm.X, nm.Y, nm.Z, uv0.X, uv0.Y, ta0.X, ta0.Y, ta0.Z,
        pos1.X, pos1.Y, pos1.Z, nm.X, nm.Y, nm.Z, uv1.X, uv1.Y, ta0.X, ta0.Y, ta0.Z,
        pos2.X, pos2.Y, pos2.Z, nm.X, nm.Y, nm.Z, uv2.X, uv2.Y, ta0.X, ta0.Y, ta0.Z,

        pos0.X, pos0.Y, pos0.Z, nm.X, nm.Y, nm.Z, uv0.X, uv0.Y, ta1.X, ta1.Y, ta1.Z,
        pos2.X, pos2.Y, pos2.Z, nm.X, nm.Y, nm.Z, uv2.X, uv2.Y, ta1.X, ta1.Y, ta1.Z,
        pos3.X, pos3.Y, pos3.Z, nm.X, nm.Y, nm.Z, uv3.X, uv3.Y, ta1.X, ta1.Y, ta1.Z
    };

    sg_buffer quad_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .data = SG_RANGE(quad_vertices),
        .label = "quad-vertices"
    });
    
    state.bind.vertex_buffers[0] = quad_buffer;

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(blinn_phong_shader_desc(sg_query_backend()));

    /* create a pipeline object for object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_a_tangent].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .label = "object-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_alloc_image_smp(state.bind.fs, SLOT__diffuse_map, SLOT_diffuse_map_smp);
    sg_image img_id_diffuse = state.bind.fs.images[SLOT__diffuse_map];

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "brickwall.jpg",
            .img_id = img_id_diffuse,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });

    sg_alloc_image_smp(state.bind.fs, SLOT__normal_map, SLOT_normal_map_smp);
    sg_image img_id_normal = state.bind.fs.images[SLOT__normal_map];

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "brickwall_normal.jpg",
            .img_id = img_id_normal,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void frame(void) {
    lopgl_update();

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);
    /* rotate the quad to show normal mapping from multiple directions */
    HMM_Mat4 model = HMM_Rotate_RH(HMM_AngleRad((float)stm_sec(stm_now()) * -10.f * 0.1f), HMM_NormV3(HMM_V3(1.f, 0.f, 1.f)));

    vs_params_t vs_params = {
        .view = view,
        .projection = projection,
        .model = model,
        .view_pos = lopgl_camera_position(),
        .light_pos = state.light_pos
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));

    sg_draw(0, 6, 1);

    lopgl_render_help();

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    lopgl_handle_input(e);
}

void cleanup(void) {
    lopgl_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .high_dpi = true,
        .window_title = "Tangent Space (LearnOpenGL)",
    };
}
