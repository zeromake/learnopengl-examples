//------------------------------------------------------------------------------
//  Depth Testing (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "2-depth-less.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind_cube;
    sg_bindings bind_plane;
    sg_pass_action pass_action;
    uint8_t file_buffer[2 * 1024 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    float cube_vertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .data = SG_RANGE(cube_vertices),
        .label = "cube-vertices"
    });
    
    state.bind_cube.vertex_buffers[0] = cube_buffer;

    float plane_vertices[] = {
        // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f								
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(plane_vertices),
        .data = SG_RANGE(plane_vertices),
        .label = "plane-vertices"
    });
    
    state.bind_plane.vertex_buffers[0] = plane_buffer;

    /* create shader from code-generated sg_shader_desc */
    sg_shader phong_shd = sg_make_shader(phong_shader_desc(sg_query_backend()));

    /* create a pipeline object for object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = phong_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_phong_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_phong_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS,
            .write_enabled =true,
        },
        .label = "pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_alloc_image_smp(state.bind_cube, IMG__diffuse_texture, SMP_diffuse_texture_smp);
    sg_alloc_image_smp(state.bind_plane, IMG__diffuse_texture, SMP_diffuse_texture_smp);
    sg_image marble_img_id = state.bind_cube.images[IMG__diffuse_texture];
    sg_image metal_img_id = state.bind_plane.images[IMG__diffuse_texture];

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "metal.png",
            .img_id = metal_img_id,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "marble.jpg",
            .img_id = marble_img_id,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void frame(void) {
    lopgl_update();

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };
    
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind_cube);

    vs_params.model = HMM_Translate(HMM_V3(-1.0f, 0.0f, -1.0f));
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    vs_params.model = HMM_Translate(HMM_V3(2.0f, 0.0f, 0.0f));
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    sg_apply_bindings(&state.bind_plane);

    vs_params.model = HMM_M4D(1.0f);
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
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
        .window_title = "Depth Test Less (LearnOpenGL)",
    };
}
