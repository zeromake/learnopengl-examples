//------------------------------------------------------------------------------
//  Shadow Mapping (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "2-rendering-shadows.glsl.h"
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
        sg_bindings bind_cube;
        sg_bindings bind_plane;
    } shadows;
    HMM_Vec3 light_pos;
    HMM_Mat4 light_space_matrix;
    uint8_t file_buffer[2 * 1024 * 1024];
} state;

static void fail_callback() {
    state.shadows.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    // compute light space matrix
    state.light_pos = HMM_V3(-2.f, 4.f, -1.f);
    float near_plane = 1.f;
    float far_plane = 7.5f;
    HMM_Mat4 light_projection = HMM_Orthographic_RH_NO(-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);
    HMM_Mat4 light_view = HMM_LookAt_RH(state.light_pos, HMM_V3(0.f, 0.f, 0.f), HMM_V3(0.f, 1.f, 0.f));
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
    sg_sampler color_smp = sg_make_sampler(&smp_desc);
    sg_image color_img = sg_make_image(&img_desc);
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
    state.shadows.bind_cube.fs.images[SLOT__shadow_map] = color_img;
    state.shadows.bind_plane.fs.images[SLOT__shadow_map] = color_img;
    state.shadows.bind_cube.fs.samplers[SLOT_shadow_map_smp] = color_smp;
    state.shadows.bind_plane.fs.samplers[SLOT_shadow_map_smp] = color_smp;

    float cube_vertices[] = {
        // back face
        -1.f, -1.f, -1.f,  0.f,  0.f, -1.f, 0.f, 0.f, // bottom-left
         1.f,  1.f, -1.f,  0.f,  0.f, -1.f, 1.f, 1.f, // top-right
         1.f, -1.f, -1.f,  0.f,  0.f, -1.f, 1.f, 0.f, // bottom-right         
         1.f,  1.f, -1.f,  0.f,  0.f, -1.f, 1.f, 1.f, // top-right
        -1.f, -1.f, -1.f,  0.f,  0.f, -1.f, 0.f, 0.f, // bottom-left
        -1.f,  1.f, -1.f,  0.f,  0.f, -1.f, 0.f, 1.f, // top-left
        // front face
        -1.f, -1.f,  1.f,  0.f,  0.f,  1.f, 0.f, 0.f, // bottom-left
         1.f, -1.f,  1.f,  0.f,  0.f,  1.f, 1.f, 0.f, // bottom-right
         1.f,  1.f,  1.f,  0.f,  0.f,  1.f, 1.f, 1.f, // top-right
         1.f,  1.f,  1.f,  0.f,  0.f,  1.f, 1.f, 1.f, // top-right
        -1.f,  1.f,  1.f,  0.f,  0.f,  1.f, 0.f, 1.f, // top-left
        -1.f, -1.f,  1.f,  0.f,  0.f,  1.f, 0.f, 0.f, // bottom-left
        // left face
        -1.f,  1.f,  1.f, -1.f,  0.f,  0.f, 1.f, 0.f, // top-right
        -1.f,  1.f, -1.f, -1.f,  0.f,  0.f, 1.f, 1.f, // top-left
        -1.f, -1.f, -1.f, -1.f,  0.f,  0.f, 0.f, 1.f, // bottom-left
        -1.f, -1.f, -1.f, -1.f,  0.f,  0.f, 0.f, 1.f, // bottom-left
        -1.f, -1.f,  1.f, -1.f,  0.f,  0.f, 0.f, 0.f, // bottom-right
        -1.f,  1.f,  1.f, -1.f,  0.f,  0.f, 1.f, 0.f, // top-right
        // right face
         1.f,  1.f,  1.f,  1.f,  0.f,  0.f, 1.f, 0.f, // top-left
         1.f, -1.f, -1.f,  1.f,  0.f,  0.f, 0.f, 1.f, // bottom-right
         1.f,  1.f, -1.f,  1.f,  0.f,  0.f, 1.f, 1.f, // top-right         
         1.f, -1.f, -1.f,  1.f,  0.f,  0.f, 0.f, 1.f, // bottom-right
         1.f,  1.f,  1.f,  1.f,  0.f,  0.f, 1.f, 0.f, // top-left
         1.f, -1.f,  1.f,  1.f,  0.f,  0.f, 0.f, 0.f, // bottom-left     
        // bottom face
        -1.f, -1.f, -1.f,  0.f, -1.f,  0.f, 0.f, 1.f, // top-right
         1.f, -1.f, -1.f,  0.f, -1.f,  0.f, 1.f, 1.f, // top-left
         1.f, -1.f,  1.f,  0.f, -1.f,  0.f, 1.f, 0.f, // bottom-left
         1.f, -1.f,  1.f,  0.f, -1.f,  0.f, 1.f, 0.f, // bottom-left
        -1.f, -1.f,  1.f,  0.f, -1.f,  0.f, 0.f, 0.f, // bottom-right
        -1.f, -1.f, -1.f,  0.f, -1.f,  0.f, 0.f, 1.f, // top-right
        // top face
        -1.f,  1.f, -1.f,  0.f,  1.f,  0.f, 0.f, 1.f, // top-left
         1.f,  1.f , 1.f,  0.f,  1.f,  0.f, 1.f, 0.f, // bottom-right
         1.f,  1.f, -1.f,  0.f,  1.f,  0.f, 1.f, 1.f, // top-right     
         1.f,  1.f,  1.f,  0.f,  1.f,  0.f, 1.f, 0.f, // bottom-right
        -1.f,  1.f, -1.f,  0.f,  1.f,  0.f, 0.f, 1.f, // top-left
        -1.f,  1.f,  1.f,  0.f,  1.f,  0.f, 0.f, 0.f  // bottom-left        
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .data = SG_RANGE(cube_vertices),
        .label = "cube-vertices"
    });
    
    state.depth.bind_cube.vertex_buffers[0] = cube_buffer;
    state.shadows.bind_cube.vertex_buffers[0] = cube_buffer;

    float plane_vertices[] = {
        // positions         // normals      // texcoords
         25.f, -.5f,  25.f,  0.f, 1.f, 0.f,  25.f,  0.f,
        -25.f, -.5f,  25.f,  0.f, 1.f, 0.f,   0.f,  0.f,
        -25.f, -.5f, -25.f,  0.f, 1.f, 0.f,   0.f, 25.f,

         25.f, -.5f,  25.f,  0.f, 1.f, 0.f,  25.f,  0.f,
        -25.f, -.5f, -25.f,  0.f, 1.f, 0.f,   0.f, 25.f,
         25.f, -.5f, -25.f,  0.f, 1.f, 0.f,  25.f, 25.f
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(plane_vertices),
        .data = SG_RANGE(plane_vertices),
        .label = "plane-vertices"
    });
    
    state.depth.bind_plane.vertex_buffers[0] = plane_buffer;
    state.shadows.bind_plane.vertex_buffers[0] = plane_buffer;

    sg_shader shd_depth = sg_make_shader(depth_shader_desc(sg_query_backend()));

    state.depth.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_depth,
        .layout = {
            /* Buffer's normal and texture coords are skipped */
            .buffers[0].stride = 8 * sizeof(float),
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

    sg_shader shd_shadows = sg_make_shader(shadows_shader_desc(sg_query_backend()));

    state.shadows.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_shadows,
        .layout = {
            .attrs = {
                [ATTR_vs_shadows_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_shadows_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_shadows_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled =true,
        },
        .label = "shadows-pipeline"
    });

    state.depth.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={1.f, 1.f, 1.f, 1.0f} }
    };

    state.shadows.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_alloc_image_smp(state.shadows.bind_cube.fs, SLOT__diffuse_texture, SLOT_diffuse_texture_smp);
    sg_image img_id_diffuse = state.shadows.bind_cube.fs.images[SLOT__diffuse_texture];
    state.shadows.bind_plane.fs.images[SLOT__diffuse_texture] = img_id_diffuse;
    state.shadows.bind_plane.fs.samplers[SLOT_diffuse_texture_smp] = state.shadows.bind_cube.fs.samplers[SLOT_diffuse_texture_smp];

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "wood.png",
            .img_id = img_id_diffuse,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void draw_cubes() {
    vs_params_t vs_params = {
        .light_space_matrix = state.light_space_matrix,
        .model = HMM_M4D(1.f)
    };

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
}

void frame(void) {
    lopgl_update();

    /* 1. render depth of scene to texture (from light's perspective) */
    sg_begin_pass(&(sg_pass){
        .action = state.depth.pass_action,
        .attachments = state.depth.attachment,
    });
    sg_apply_pipeline(state.depth.pip);

    /* plane */
    sg_apply_bindings(&state.depth.bind_plane);

    vs_params_t vs_params = {
        .light_space_matrix = state.light_space_matrix,
        .model = HMM_M4D(1.f)
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 6, 1);

    /* cubes */
    sg_apply_bindings(&state.depth.bind_cube);
    draw_cubes();
    sg_end_pass();

    /* 2. render scene as normal using the generated depth/shadow map */
    sg_begin_pass(&(sg_pass){
        .action = state.shadows.pass_action,
        .swapchain = sglue_swapchain(),
    });
    sg_apply_pipeline(state.shadows.pip);

    /* plane */
    sg_apply_bindings(&state.shadows.bind_plane);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_shadows_t vs_params_shadows = {
        .projection = projection,
        .view = view
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_shadows, &SG_RANGE(vs_params_shadows));

    fs_params_shadows_t fs_params_shadows = {
        .light_pos = state.light_pos,
        .view_pos = lopgl_camera_position(),
    };

    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params_shadows, &SG_RANGE(fs_params_shadows));

    sg_draw(0, 6, 1);

    /* cubes */
    sg_apply_bindings(&state.shadows.bind_cube);
    draw_cubes();

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
        .window_title = "Rendering Shadows (LearnOpenGL)",
    };
}
