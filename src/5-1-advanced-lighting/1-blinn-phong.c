//------------------------------------------------------------------------------
//  Advanced Lighting (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "1-blinn-phong.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    HMM_Vec3 light_pos;
    bool blinn;
    uint8_t file_buffer[2 * 1024 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    // set light position
    state.light_pos = HMM_V3(0.f, 0.f, 0.f);

    // default is blinn phong shading
    state.blinn = true;

    float vertices[] = {
        // positions            // normals         // texcoords
         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
        .label = "plane-vertices"
    });
    
    state.bind.vertex_buffers[0] = plane_buffer;

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(blinn_phong_shader_desc(sg_query_backend()));

    /* create a pipeline object for object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_blinn_phong_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_blinn_phong_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_blinn_phong_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .label = "object-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_alloc_image_smp(state.bind, IMG__floor_texture, SMP_floor_texture_smp);
    sg_image img_id_floor = state.bind.images[IMG__floor_texture];

    lopgl_load_image(&(lopgl_image_request_t){
        .path = "wood.png",
        .img_id = img_id_floor,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .fail_callback = fail_callback
    });
}

static void render_ui() {
    sdtx_canvas(sapp_width()*0.5f, sapp_height()*0.5f);
    sdtx_origin(sapp_width()*0.5f/8.f - 18.f, 0.25f);       // each character occupies a grid fo 8x8
    sdtx_home();

    sdtx_color4b(0xff, 0x00, 0x00, 0xaf);
    sdtx_puts(state.blinn ? "Blinn-Phong\n\n" : "Phong\n\n");
    sdtx_puts("Switch:\t\t'B'");
    sdtx_draw();
}

void frame(void) {
    lopgl_update();

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));

    fs_params_t fs_params = {
        .view_pos = lopgl_camera_position(),
        .light_pos = state.light_pos,
        .blinn = state.blinn ? 1.f : 0.f 
    };

    sg_apply_uniforms(UB_fs_params, &SG_RANGE(fs_params));

    sg_draw(0, 6, 1);

    lopgl_render_help();

    if (lopgl_ui_visible()) {
        render_ui();
    }

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    lopgl_handle_input(e);

    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_B) {
            state.blinn = !state.blinn;
        }
    }
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
        .window_title = "Blinn-Phong (LearnOpenGL)",
    };
}
