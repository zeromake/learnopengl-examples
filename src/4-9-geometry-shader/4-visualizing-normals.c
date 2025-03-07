//------------------------------------------------------------------------------
//  Geometry Shader (4)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#include "4-visualizing-normals.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "../../libs/fast_obj/lopgl_fast_obj.h"

static const char* filename = "backpack.obj";

typedef struct mesh_t {
    sg_pipeline pip_diffuse;
    sg_pipeline pip_normals;
    sg_bindings bind_diffuse;
    sg_bindings bind_normals;
    unsigned int face_count;
} mesh_t;

/* application state */
static struct {
    mesh_t mesh; 
    sg_pass_action pass_action;
    uint8_t file_buffer[16 * 1024 * 1024];
    float vertex_buffer[70000 * 3 * 5];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void load_obj_callback(lopgl_obj_response_t* response) {
    fastObjMesh* mesh = response->mesh;
    state.mesh.face_count = mesh->face_count;

    for (unsigned int i = 0; i < mesh->face_count * 3; ++i) {
        fastObjIndex vertex = mesh->indices[i];

        unsigned int pos = i * 5;
        unsigned int v_pos = vertex.p * 3;
        unsigned int t_pos = vertex.t * 2;

        memcpy(state.vertex_buffer + pos, mesh->positions + v_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 3, mesh->texcoords + t_pos, 2 * sizeof(float));
    }
    sg_sampler_desc smp_desc = {
        /* set filter to nearest, webgl2 does not support filtering for float textures */
        .mag_filter = SG_FILTER_NEAREST,
        .min_filter = SG_FILTER_NEAREST,
    };
    int width = 1024;
    int height = mesh->face_count*3*5/width + 1;
    int size = width * height * 4;
    sg_image vertex_image_id = sg_make_image(&(sg_image_desc){
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_R32F,
        .data.subimage[0][0] = {
            .ptr = state.vertex_buffer,
            .size = size
        },
        .label = "color-texture"
    });
    sg_sampler vertex_smp_id = sg_make_sampler(&smp_desc);

    state.mesh.bind_diffuse.images[IMG__vertex_texture] = vertex_image_id;
    state.mesh.bind_normals.images[IMG__vertex_texture] = vertex_image_id;
    state.mesh.bind_diffuse.samplers[SMP_vertex_texture_smp] = vertex_smp_id;
    state.mesh.bind_normals.samplers[SMP_vertex_texture_smp] = vertex_smp_id;

    sg_alloc_image_smp(state.mesh.bind_diffuse, IMG__diffuse_texture, SMP_diffuse_texture_smp);
    sg_image diffuse_img_id = state.mesh.bind_diffuse.images[IMG__diffuse_texture];

    lopgl_load_image(&(lopgl_image_request_t){
        .path = mesh->materials[0].map_Kd.name,
        .img_id = diffuse_img_id,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .fail_callback = fail_callback
    });
}

static void init(void) {
    lopgl_setup();

    /* create shader from code-generated sg_shader_desc */
    sg_shader simple_shd = sg_make_shader(simple_shader_desc(sg_query_backend()));
    
    float vertices[] = {
        0.0f,
    };
    state.mesh.bind_diffuse.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
    });
    state.mesh.bind_normals.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
    });

    /* create a pipeline object for the diffuse shading */
    state.mesh.pip_diffuse = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = simple_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_simple_a_dummy].format = SG_VERTEXFORMAT_FLOAT,
            },
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled =true,
        },
        .label = "diffuse-pipeline"
    });

    sg_shader normals_shd = sg_make_shader(normals_shader_desc(sg_query_backend()));

    /* create a pipeline object for the normals */
    state.mesh.pip_normals = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = normals_shd,
        .primitive_type = SG_PRIMITIVETYPE_LINES,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_normals_a_dummy].format = SG_VERTEXFORMAT_FLOAT,
            },
        },
        .depth = {
            .compare =SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled =true,
        },
        .label = "normals-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    lopgl_load_obj(&(lopgl_obj_request_t){
        .path = filename,
        .callback = load_obj_callback,
        .fail_callback = fail_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
    });
}

void frame(void) {
    lopgl_update();

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });

    if (state.mesh.face_count > 0) {
        HMM_Mat4 view = lopgl_view_matrix();
        HMM_Mat4 projection = HMM_Perspective_RH_NO(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

        vs_params_t vs_params = {
            .model = HMM_M4D(1.f),
            .view = view,
            .projection = projection,
        };

        sg_apply_pipeline(state.mesh.pip_diffuse);
        sg_apply_bindings(&state.mesh.bind_diffuse);

        sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));

        sg_draw(0, state.mesh.face_count * 3, 1);

        sg_apply_pipeline(state.mesh.pip_normals);
        sg_apply_bindings(&state.mesh.bind_normals);

        sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));

        sg_draw(0, state.mesh.face_count * 2, 1);
    }

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
        .window_title = "Visualizing Normals (LearnOpenGL)",
    };
}
