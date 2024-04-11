//------------------------------------------------------------------------------
//  Advanced GLSL (3)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "3-front-facing.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint8_t file_buffer[32 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    float vertices[] = {
        // positions         // texture coord
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,        
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,            

        0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,     
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 
        
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,       
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 
                  
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,              
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f        
    };

    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
        .label = "vertices-cube"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

    /* create a pipeline object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS,
            .write_enabled =true,
        },
        .cull_mode = SG_CULLMODE_NONE,
        .face_winding = SG_FACEWINDING_CCW,
        .label = "cube-pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_alloc_image_smp(state.bind.fs, SLOT__front_texture, SLOT_front_texture_smp);
    sg_image img_id_front = state.bind.fs.images[SLOT__front_texture];

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "uv_grid.png",
            .img_id = img_id_front,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });

    /* checkerboard texture, needs to be power of two for webgl 1 */
    uint32_t pixels[16*16];

    for (size_t i = 0; i < 16; ++i) {
        uint32_t c0 = i % 2 == 0 ? 0xFFD9D9D9 : 0xFFA8A8A8;
        uint32_t c1 = i % 2 == 0 ? 0xFFA8A8A8 : 0xFFD9D9D9; 
        for (size_t j = 0; j < 16; ++j) { 
            pixels[i*16 + j] = j % 2 == 0 ? c0 : c1;
        }
    }

    state.bind.fs.images[SLOT__back_texture] = sg_make_image(&(sg_image_desc){
        .width = 16,
        .height = 16,
        .data.subimage[0][0] = {
            .ptr = pixels,
            .size = sizeof(pixels)
        },
        .label = "back-texture"
    });
    state.bind.fs.samplers[SLOT__back_texture] = sg_make_sampler(&global_sampler_desc);
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
    sg_apply_bindings(&state.bind);

    // TODO: adjust camera instead of scaling model
    //vs_params.model = HMM_M4D(1.f);
    vs_params.model = HMM_Scale(HMM_V3(2.f, 2.f, 2.f));
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));

    sg_draw(0, 36, 1);

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
        .window_title = "Front Facing (LearnOpenGL)",
    };
}
