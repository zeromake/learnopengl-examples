//------------------------------------------------------------------------------
//  Advanced GLSL (4)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "HandmadeMath.h"
#include "4-uniform-buffers.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip_red;
    sg_pipeline pip_green;
    sg_pipeline pip_blue;
    sg_pipeline pip_yellow;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void) {
    lopgl_setup();

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };
    
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
        .label = "vertices-cube"
    });

    /* create shaders from code-generated sg_shader_desc */
    sg_shader shd_red = sg_make_shader(red_shader_desc(sg_query_backend()));
    sg_shader shd_green = sg_make_shader(green_shader_desc(sg_query_backend()));
    sg_shader shd_blue = sg_make_shader(blue_shader_desc(sg_query_backend()));
    sg_shader shd_yellow = sg_make_shader(yellow_shader_desc(sg_query_backend()));

    sg_pipeline_desc pip_desc = {
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3
            }
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS,
            .write_enabled =true,
        },
        .label = "cube-pipeline"
    };

    /* create a pipeline objects */
    pip_desc.shader = shd_red;
    state.pip_red = sg_make_pipeline(&pip_desc);
    pip_desc.shader = shd_green;
    state.pip_green = sg_make_pipeline(&pip_desc);
    pip_desc.shader = shd_blue;
    state.pip_blue = sg_make_pipeline(&pip_desc);
    pip_desc.shader = shd_yellow;
    state.pip_yellow = sg_make_pipeline(&pip_desc);

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void frame(void) {
    lopgl_update();

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });

    sg_apply_pipeline(state.pip_red);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_view_projection_t vs_vp = {
        .view = view,
        .projection = projection
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &SG_RANGE(vs_vp));

    vs_model_t vs_m = {
        .model = HMM_Translate(HMM_V3(-0.75f, 0.75f, 0.0f))       // move top-left
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &SG_RANGE(vs_m));
    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_green);
    sg_apply_bindings(&state.bind);
    // we need to re-apply the uniforms after applying a new pipeline, sort of defeats the purpose of this example...
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &SG_RANGE(vs_vp));
    vs_m.model = HMM_Translate(HMM_V3(0.75f, 0.75f, 0.0f));       // move top-right
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &SG_RANGE(vs_m));
    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_yellow);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &SG_RANGE(vs_vp));
    vs_m.model = HMM_Translate(HMM_V3(-0.75f, -0.75f, 0.0f));     // move bottom-left
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &SG_RANGE(vs_m));
    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_blue);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &SG_RANGE(vs_vp));
    vs_m.model = HMM_Translate(HMM_V3(0.75f, -0.75f, 0.0f));      // move bottom-right
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &SG_RANGE(vs_m));
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
        .window_title = "Uniform Buffers (LearnOpenGL)",
    };
}
