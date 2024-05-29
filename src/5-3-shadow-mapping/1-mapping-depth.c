//------------------------------------------------------------------------------
//  Shadow Mapping (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "HandmadeMath.h"
#include "1-mapping-depth.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    struct {
        sg_pass_action pass_action;
        sg_attachments attachment;
        sg_pipeline pip;
        sg_bindings bind_cube;
        sg_bindings bind_plane;
    } depth;
    struct {
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
    } quad;
    HMM_Mat4 light_space_matrix;
} state;

static void init(void) {
    lopgl_setup();

    // compute light space matrix
    HMM_Vec3 light_pos = HMM_V3(-2.f, 4.65f, -1.f);
    float near_plane = 1.f;
    float far_plane = 7.5f;
    HMM_Mat4 light_projection = HMM_Orthographic_RH_NO(-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);
    HMM_Mat4 light_view = HMM_LookAt_RH(light_pos, HMM_V3(0.f, 0.f, 0.f), HMM_V3(0.f, 1.f, 0.f));
    state.light_space_matrix = HMM_MulM4(light_projection, light_view);

     /* a render pass with one color- and one depth-attachment image */
    sg_image_desc img_desc = {
        .render_target = true,
        .width = 1024,
        .height = 1024,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 1,
        .label = "shadow-map-color-image"
    };
    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
    };
    sg_image color_img = sg_make_image(&img_desc);
    sg_sampler color_smp = sg_make_sampler(&smp_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    img_desc.label = "shadow-map-depth-image";
    sg_image depth_img = sg_make_image(&img_desc);
    state.depth.attachment = sg_make_attachments(&(sg_attachments_desc){
        .colors[0].image = color_img,
        .depth_stencil.image = depth_img,
        .label = "shadow-map-pass"
    });

    // sokol and webgl 1 do not support using the depth map as texture map
    // so instead we write the depth value to the color map
    state.quad.bind.fs.images[SLOT__depth_map] = color_img;
    state.quad.bind.fs.samplers[SLOT_depth_map_smp] = color_smp;

    float cube_vertices[] = {
        // back face
        -1.f, -1.f, -1.f,
        1.f,  1.f, -1.f,
        1.f, -1.f, -1.f,          
        1.f,  1.f, -1.f, 
        -1.f, -1.f, -1.f,
        -1.f,  1.f, -1.f,
        // front face
        -1.f, -1.f,  1.f,
        1.f, -1.f,  1.f, 
        1.f,  1.f,  1.f, 
        1.f,  1.f,  1.f, 
        -1.f,  1.f,  1.f,
        -1.f, -1.f,  1.f,
        // left face
        -1.f,  1.f,  1.f,
        -1.f,  1.f, -1.f,
        -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f,
        -1.f, -1.f,  1.f,
        -1.f,  1.f,  1.f,
        // right face
        1.f,  1.f,  1.f, 
        1.f, -1.f, -1.f, 
        1.f,  1.f, -1.f,          
        1.f, -1.f, -1.f, 
        1.f,  1.f,  1.f, 
        1.f, -1.f,  1.f,      
        // bottom face
        -1.f, -1.f, -1.f,
        1.f, -1.f, -1.f, 
        1.f, -1.f,  1.f, 
        1.f, -1.f,  1.f, 
        -1.f, -1.f,  1.f,
        -1.f, -1.f, -1.f,
        // top face
        -1.f,  1.f, -1.f,
        1.f,  1.f , 1.f, 
        1.f,  1.f, -1.f,      
        1.f,  1.f,  1.f, 
        -1.f,  1.f, -1.f,
        -1.f,  1.f,  1.f        
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .data = SG_RANGE(cube_vertices),
        .label = "cube-vertices"
    });
    
    state.depth.bind_cube.vertex_buffers[0] = cube_buffer;

    float plane_vertices[] = {
        // positions       
         25.f, -.5f,  25.f,
        -25.f, -.5f,  25.f,
        -25.f, -.5f, -25.f,

         25.f, -.5f,  25.f,
        -25.f, -.5f, -25.f,
         25.f, -.5f, -25.f,
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(plane_vertices),
        .data = SG_RANGE(plane_vertices),
        .label = "plane-vertices"
    });
    
    state.depth.bind_plane.vertex_buffers[0] = plane_buffer;

    float quad_vertices[] = {
        // positions     // texture Coords
        -1.f,  1.f, 0.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 0.f, 0.f,
         1.f,  1.f, 0.f, 1.f, 1.f,
         1.f, -1.f, 0.f, 1.f, 0.f,
    };

    sg_buffer quad_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .data = SG_RANGE(quad_vertices),
        .label = "quad-vertices"
    });
    
    state.quad.bind.vertex_buffers[0] = quad_buffer;

    sg_shader shd_depth = sg_make_shader(depth_shader_desc(sg_query_backend()));

    state.depth.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_depth,
        .layout = {
            .attrs = {
                [ATTR_vs_depth_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS_EQUAL,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .write_enabled =true,
        },
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
        },
        .color_count = 1,
        .label = "depth-pipeline"
    });

    sg_shader shd_quad = sg_make_shader(quad_shader_desc(sg_query_backend()));

    state.quad.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_quad,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .layout = {
            .attrs = {
                [ATTR_vs_quad_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_quad_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .label = "quad-pipeline"
    });
    
    state.depth.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={1.f, 1.f, 1.f, 1.0f} }
    };

    state.quad.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void frame(void) {
    lopgl_update();

    sg_begin_pass(&(sg_pass){
        .action = state.depth.pass_action,
        .attachments = state.depth.attachment,
    });

    sg_apply_pipeline(state.depth.pip);
    sg_apply_bindings(&state.depth.bind_plane);

    //floor
    vs_params_t vs_params = {
        .light_space_matrix = state.light_space_matrix,
        .model = HMM_M4D(1.f)
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 6, 1);

    // cubes
    sg_apply_bindings(&state.depth.bind_cube);
    HMM_Mat4 translate = HMM_Translate(HMM_V3(0.f, 1.5f, 0.f));
    HMM_Mat4 scale = HMM_Scale(HMM_V3(.5f, .5f, .5f));
    vs_params.model = HMM_MulM4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_V3(2.f, 0.f, 1.f));
    scale = HMM_Scale(HMM_V3(.5f, .5f, .5f));
    vs_params.model = HMM_MulM4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_V3(-1.f, 0.f, 2.f));
    HMM_Mat4 rotate = HMM_Rotate_RH(HMM_AngleDeg(60.f), HMM_NormV3(HMM_V3(1.f, 0.f, 1.f)));
    scale = HMM_Scale(HMM_V3(.25f, .25f, .25f));
    vs_params.model = HMM_MulM4(HMM_MulM4(translate, rotate), scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    sg_end_pass();

    sg_begin_pass(&(sg_pass){
        .action = state.quad.pass_action,
        .swapchain = sglue_swapchain(),
    });
    sg_apply_pipeline(state.quad.pip);
    sg_apply_bindings(&state.quad.bind);

    sg_draw(0, 4, 1);

    lopgl_render_help();

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
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
        .window_title = "Mapping Depth (LearnOpenGL)",
    };
}
