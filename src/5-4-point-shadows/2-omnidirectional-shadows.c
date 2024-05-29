//------------------------------------------------------------------------------
//  Point Shadows (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "2-omnidirectional-shadows.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

static const int SHADOW_WIDTH = 1024;
static const int SHADOW_HEIGHT = 1024;

/* application state */
static struct {
    struct {
        sg_pass_action pass_action;
        sg_attachments attachment[6];
        sg_pipeline pip;
        sg_bindings bind;
    } depth;
    struct {
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
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

    state.light_pos = HMM_V3(0.f, 0.f, 0.f);

    /* create depth cubemap */
    sg_image_desc img_desc = {
        .type = SG_IMAGETYPE_CUBE,
        .render_target = true,
        .width = SHADOW_WIDTH,
        .height = SHADOW_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 1,
        .label = "shadow-map-color-image"
    };
    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_w = SG_WRAP_CLAMP_TO_EDGE,
    };
    sg_sampler color_smp = sg_make_sampler(&smp_desc);
    sg_image color_img = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    img_desc.label = "shadow-map-depth-image";
    sg_image depth_img = sg_make_image(&img_desc);

    /* one pass for each cubemap face */
    for (size_t i = 0; i < 6; ++i) {
        state.depth.attachment[i] = sg_make_attachments(&(sg_attachments_desc){
            .colors[0] = { .image = color_img, .slice = i },
            .depth_stencil = {  .image = depth_img, .slice = i },
            .label = "shadow-map-pass"
        });
    }

    // sokol and webgl 1 do not support using the depth map as texture map
    // so instead we write the depth value to the color map
    state.shadows.bind.fs.images[SLOT__depth_map] = color_img;
    state.shadows.bind.fs.samplers[SLOT_depth_map_smp] = color_smp;

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
    
    state.depth.bind.vertex_buffers[0] = cube_buffer;
    state.shadows.bind.vertex_buffers[0] = cube_buffer;

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
            .write_enabled =true,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
        },
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
        },
        .color_count = 1,
        /* cull front faces for depth map */
        .cull_mode = SG_CULLMODE_FRONT,
        .face_winding = SG_FACEWINDING_CCW,
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
        .cull_mode = SG_CULLMODE_BACK,
        .face_winding = SG_FACEWINDING_CCW,
        .label = "shadows-pipeline"
    });

    state.depth.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={1.f, 1.f, 1.f, 1.0f} }
    };

    state.shadows.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_alloc_image_smp(state.shadows.bind.fs, SLOT__diffuse_texture, SLOT_diffuse_texture_smp);
    sg_image img_id_diffuse = state.shadows.bind.fs.images[SLOT__diffuse_texture];

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "wood.png",
            .img_id = img_id_diffuse,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void draw_room_cube() {
    vs_params_t vs_params = {
        /* invert the outer cube so just the inner faces are shown with back culling */
        .model = HMM_Scale(HMM_V3(-5.f, -5.f, -5.f))
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
}

void draw_cubes() {
    vs_params_t vs_params;
    HMM_Mat4 translate = HMM_Translate(HMM_V3(4.f, -3.5f, 0.f));
    HMM_Mat4 scale = HMM_Scale(HMM_V3(.5f, .5f, .5f));
    vs_params.model = HMM_MulM4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_V3(2.f, 3.f, 1.f));
    scale = HMM_Scale(HMM_V3(.75f, .75f, .75f));
    vs_params.model = HMM_MulM4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_V3(-3.f, -1.f, 0.f));
    scale = HMM_Scale(HMM_V3(.5f, .5f, .5f));
    vs_params.model = HMM_MulM4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_V3(-1.5f, 1.f, 1.5f));
    scale = HMM_Scale(HMM_V3(.5f, .5f, .5f));
    vs_params.model = HMM_MulM4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_V3(-1.5f, 2.f, -3.f));
    HMM_Mat4 rotate = HMM_Rotate_RH(HMM_AngleDeg(60.f), HMM_NormV3(HMM_V3(1.f, 0.f, 1.f)));
    scale = HMM_Scale(HMM_V3(.75f, .75f, .75f));
    vs_params.model = HMM_MulM4(HMM_MulM4(translate, rotate), scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
}

void frame(void) {
    lopgl_update();

    /* move light position over time */
    state.light_pos.Z = HMM_SinF((float)stm_sec(stm_now()) * .5f) * 3.f;

    /* create light space transform matrices */
    HMM_Mat4 light_space_transforms[6];
    float near_plane = 1.0f;
    float far_plane  = 25.0f;
    HMM_Mat4 shadow_proj = HMM_Perspective_RH_NO(90.0f, (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
    HMM_Vec3 center = HMM_AddV3(state.light_pos, HMM_V3(1.f, 0.f, 0.f));
    HMM_Mat4 lookat = HMM_LookAt_RH(state.light_pos, center, HMM_V3(0.f, -1.f, 0.f));
    light_space_transforms[SG_CUBEFACE_POS_X] = HMM_MulM4(shadow_proj, lookat);
    center = HMM_AddV3(state.light_pos, HMM_V3(-1.f, 0.f, 0.f));
    lookat = HMM_LookAt_RH(state.light_pos, center, HMM_V3(0.f, -1.f, 0.f));
    light_space_transforms[SG_CUBEFACE_NEG_X] = HMM_MulM4(shadow_proj, lookat);
    center = HMM_AddV3(state.light_pos, HMM_V3(0.f, 1.f, 0.f));
    lookat = HMM_LookAt_RH(state.light_pos, center, HMM_V3(0.f, 0.f, 1.f));
    light_space_transforms[SG_CUBEFACE_POS_Y] = HMM_MulM4(shadow_proj, lookat);
    center = HMM_AddV3(state.light_pos, HMM_V3(0.f, -1.f, 0.f));
    lookat = HMM_LookAt_RH(state.light_pos, center, HMM_V3(0.f, 0.f, -1.f));
    light_space_transforms[SG_CUBEFACE_NEG_Y] = HMM_MulM4(shadow_proj, lookat);
    center = HMM_AddV3(state.light_pos, HMM_V3(0.f, 0.f, 1.f));
    lookat = HMM_LookAt_RH(state.light_pos, center, HMM_V3(0.f, -1.f,  0.f));
    light_space_transforms[SG_CUBEFACE_POS_Z] = HMM_MulM4(shadow_proj, lookat);
    center = HMM_AddV3(state.light_pos, HMM_V3(0.f, 0.f, -1.f));
    lookat = HMM_LookAt_RH(state.light_pos, center, HMM_V3(0.f, -1.f,  0.f));
    light_space_transforms[SG_CUBEFACE_NEG_Z] = HMM_MulM4(shadow_proj, lookat);

    /* render depth of scene to cubemap (from light's perspective) */
    for (size_t i = 0; i < 6; ++i) {
        sg_begin_pass(&(sg_pass){
            .action = state.depth.pass_action,
            .attachments = state.depth.attachment[i],
        });
        sg_apply_pipeline(state.depth.pip);
        sg_apply_bindings(&state.depth.bind);

        vs_params_depth_t vs_params_depth = {
            .light_space_matrix = light_space_transforms[i]
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_depth, &SG_RANGE(vs_params_depth));

        fs_params_depth_t fs_params_depth = {
            .light_pos = state.light_pos,
            .far_plane = far_plane
        };

        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params_depth, &SG_RANGE(fs_params_depth));

        draw_cubes();
        sg_end_pass();
    }

    /* render scene as normal using the generated depth/shadow map */
    sg_begin_pass(&(sg_pass){
        .action = state.shadows.pass_action,
        .swapchain = sglue_swapchain(),
    });
    sg_apply_pipeline(state.shadows.pip);
    sg_apply_bindings(&state.shadows.bind);

    HMM_Mat4 view = lopgl_view_matrix();
    HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_shadows_t vs_params_shadows = {
        .projection = projection,
        .view = view,
        .normal_multiplier = 1.f
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_shadows, &SG_RANGE(vs_params_shadows));

    fs_params_shadows_t fs_params_shadows = {
        .light_pos = state.light_pos,
        .view_pos = lopgl_camera_position(),
        .far_plane = far_plane
    };

    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params_shadows, &SG_RANGE(fs_params_shadows));

    draw_cubes();

    /* invert the normals for the outer cube */
    vs_params_shadows.normal_multiplier = -1.f;
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_shadows, &SG_RANGE(vs_params_shadows));

    draw_room_cube();

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
        .window_title = "Omnidirectional Shadows (LearnOpenGL)",
    };
}
