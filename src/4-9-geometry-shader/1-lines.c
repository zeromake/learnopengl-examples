//------------------------------------------------------------------------------
//  Geometry Shader (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "1-lines.glsl.h"
#include "string.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void) {
    lopgl_setup();

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

    float vertices[] = {
        0.0f,
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
    });
    /* a pipeline state object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                /* dummy vertex attribute, otherwise sokol complains */
                [ATTR_vs_a_dummy].format = SG_VERTEXFORMAT_FLOAT,
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_LINES,
        .label = "vertices-pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    float positions[] = {
        -0.5f, 0.5f, // top-left
        0.5f,  0.5f, // top-right
        0.5f,  -0.5f, // bottom-right
        -0.5f, -0.5f,  // bottom-left
    };
    sg_sampler_desc smp_desc = {
        /* set filter to nearest, webgl2 does not support filtering for float textures */
        .mag_filter = SG_FILTER_NEAREST,
        .min_filter = SG_FILTER_NEAREST,
    };
    state.bind.vs.images[SLOT__positions_texture] = sg_make_image(&(sg_image_desc){
        .width = 4,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_RG32F,
        .data.subimage[0][0] = {
            .ptr = positions,
            .size = sizeof(positions)
        },
        .label = "positions-texture"
    });
    state.bind.vs.samplers[SLOT_positions_texture_smp] = sg_make_sampler(&smp_desc);
}

void frame(void) {
    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 4*2, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }
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
        .window_title = "Lines (LearnOpenGL)",
    };
}
