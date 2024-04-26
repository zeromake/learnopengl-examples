//------------------------------------------------------------------------------
//  Anti Aliasing (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "HandmadeMath.h"
#include "2-offscreen-msaa.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

#define OFFSCREEN_SAMPLE_COUNT (1)
#define DISPLAY_SAMPLE_COUNT (4)

/* application state */
static struct {
    struct {
        sg_attachments attachment;
        sg_attachments_desc attachment_desc;
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
    } offscreen;
    struct { 
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
    } display;
} state;

/* called initially and when window size changes */
void create_offscreen_pass(int width, int height) {
    /* destroy previous resource (can be called for invalid id) */
    sg_destroy_attachments(state.offscreen.attachment);
    sg_destroy_image(state.offscreen.attachment_desc.colors[0].image);
    sg_destroy_image(state.offscreen.attachment_desc.depth_stencil.image);

    /* create offscreen rendertarget images and pass */
    sg_sampler_desc color_smp_desc = {
        /* Webgl 1.0 does not support repeat for textures that are not a power of two in size */
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
    };
    sg_image_desc color_img_desc = {
        .render_target = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        /* we need to set the sample count in both the rendertarget and the pipeline */
        .sample_count = OFFSCREEN_SAMPLE_COUNT,
        .label = "color-image"
    };
    sg_image color_img = sg_make_image(&color_img_desc);
    sg_sampler color_smp = sg_make_sampler(&color_smp_desc);

    sg_image_desc depth_img_desc = color_img_desc;
    depth_img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    depth_img_desc.label = "depth-image";
    depth_img_desc.type = SG_IMAGETYPE_2D;
    sg_image depth_img = sg_make_image(&depth_img_desc);

    state.offscreen.attachment_desc = (sg_attachments_desc){
        .colors[0].image = color_img,
        .depth_stencil.image = depth_img,
        .label = "offscreen-pass"
    };
    state.offscreen.attachment = sg_make_attachments(&state.offscreen.attachment_desc);

    /* also need to update the fullscreen-quad texture bindings */
    state.display.bind.fs.images[SLOT__diffuse_texture] = color_img;
    state.display.bind.fs.samplers[SLOT__diffuse_texture] = color_smp;
}

static void init(void) {
    lopgl_setup();

    /* a render pass with one color- and one depth-attachment image */
    create_offscreen_pass(sapp_width(), sapp_height());

    /* a pass action to clear offscreen framebuffer */
    state.offscreen.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    /* a pass action for rendering the fullscreen-quad */
    state.display.pass_action = (sg_pass_action) {
        .colors[0].load_action=SG_LOADACTION_DONTCARE,
        .depth.load_action=SG_LOADACTION_DONTCARE,
        .stencil.load_action=SG_LOADACTION_DONTCARE
    };
    
    float cube_vertices[] = {
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

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .data = SG_RANGE(cube_vertices),
        .label = "cube-vertices"
    });

    float quad_vertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    sg_buffer quad_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .data = SG_RANGE(quad_vertices),
        .label = "quad-vertices"
    });
    
    state.offscreen.bind.vertex_buffers[0] = cube_buffer;

    /* resource bindings to render an fullscreen-quad */
    state.display.bind.vertex_buffers[0] = quad_buffer;

    /* create a pipeline object for offscreen pass */
    state.offscreen.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(offscreen_shader_desc(sg_query_backend())),
        .layout = {
            .attrs = {
                [ATTR_vs_offscreen_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
        },
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
        },
        .color_count = 1,
        /* we need to set the sample count in both the rendertarget and the pipeline */
        .sample_count = OFFSCREEN_SAMPLE_COUNT,
        .label = "offscreen-pipeline"
    });

    /* and another pipeline-state-object for the display pass */
    state.display.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs = {
                [ATTR_vs_display_a_pos].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_display_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .shader = sg_make_shader(display_shader_desc(sg_query_backend())),
        .sample_count = DISPLAY_SAMPLE_COUNT,
        .label = "display-pipeline"
    });
}

void frame(void) {
    lopgl_update();

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    /* the offscreen pass, rendering an rotating, untextured cube into a render target image */
    sg_begin_pass(&(sg_pass){ .action = state.offscreen.pass_action, .attachments = state.offscreen.attachment });
    sg_apply_pipeline(state.offscreen.pip);
    sg_apply_bindings(&state.offscreen.bind);

    vs_params.model = HMM_Translate(HMM_V3(0.f, 0.f, 0.f));
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    sg_end_pass();

    /* and the display-pass, rendering a quad, using the previously rendered 
       offscreen render-target as texture */
    sg_begin_pass(&(sg_pass){ .action = state.display.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.display.pip);
    sg_apply_bindings(&state.display.bind);
    sg_draw(0, 6, 1);

    lopgl_render_help();

    sg_end_pass();
    sg_commit();
}



void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_RESIZED) {
        create_offscreen_pass(e->framebuffer_width, e->framebuffer_height);
    }

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
        .sample_count = DISPLAY_SAMPLE_COUNT,
        .window_title = "Offscreen MSAA (LearnOpenGL)",
    };
}
