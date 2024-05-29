//------------------------------------------------------------------------------
//  1-9-3-look
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_fetch.h"
#include "sokol_time.h"
#include "sokol_helper.h"
#include "HandmadeMath.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "shaders.glsl.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint8_t file_buffer[256 * 1024];
    HMM_Vec3 cube_positions[10];
    HMM_Vec3 camera_pos;
    HMM_Vec3 camera_front;
    HMM_Vec3 camera_up;
    uint64_t last_time;
    uint64_t delta_time;
    bool mouse_btn;
    bool first_mouse;
    float last_x;
    float last_y;
    float yaw;
    float pitch;
    float fov;
} state;

static void fetch_callback(const sfetch_response_t*);

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment()
    });

     /* setup sokol-fetch
        The 1 channel and 1 lane configuration essentially serializes
        IO requests. Which is just fine for this example. */
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 2,
        .num_channels = 1,
        .num_lanes = 1
    });

    /* initialize sokol_time */
    stm_setup();

    /* Allocate an image handle, but don't actually initialize the image yet,
       this happens later when the asynchronous file load has finished.
       Any draw calls containing such an "incomplete" image handle
       will be silently dropped.
    */
    sg_alloc_image_smp(state.bind.fs, SLOT__texture1, SLOT_texture1_smp);
    sg_alloc_image_smp(state.bind.fs, SLOT__texture2, SLOT_texture2_smp);

    /* flip images vertically after loading */
    stbi_set_flip_vertically_on_load(true);  

    // hide mouse cursor
    sapp_show_mouse(false);

    // set default camera configuration
    state.camera_pos = HMM_V3(0.0f, 0.0f,  3.0f);
    state.camera_front = HMM_V3(0.0f, 0.0f, -1.0f);
    state.camera_up = HMM_V3(0.0f, 1.0f,  0.0f);
    state.first_mouse = true;
    state.fov = 45.0f;
    state.yaw = -90.0f;

    state.cube_positions[0] = HMM_V3( 0.0f,  0.0f,  0.0f);
    state.cube_positions[1] = HMM_V3( 2.0f,  5.0f, -15.0f);
    state.cube_positions[2] = HMM_V3(-1.5f, -2.2f, -2.5f);
    state.cube_positions[3] = HMM_V3(-3.8f, -2.0f, -12.3f);
    state.cube_positions[4] = HMM_V3( 2.4f, -0.4f, -3.5f);
    state.cube_positions[5] = HMM_V3(-1.7f,  3.0f, -7.5f);
    state.cube_positions[6] = HMM_V3( 1.3f, -2.0f, -2.5f);
    state.cube_positions[7] = HMM_V3( 1.5f,  2.0f, -2.5f);
    state.cube_positions[8] = HMM_V3( 1.5f,  0.2f, -1.5f);
    state.cube_positions[9] = HMM_V3(-1.3f,  1.0f, -1.5f);

    float vertices[] = {
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
    
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
        .label = "cube-vertices"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

    /* create a pipeline object (default render states are fine for triangle) */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
        },
        .label = "triangle-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.2f, 0.3f, 0.3f, 1.0f} }
    };

    sg_image image1 = state.bind.fs.images[SLOT__texture1];
    sg_image image2 = state.bind.fs.images[SLOT__texture2];

    /* start loading the JPG file */
    sfetch_send(&(sfetch_request_t){
        .path = "container.jpg",
        .callback = fetch_callback,
        .buffer = SFETCH_RANGE(state.file_buffer),
        .user_data = SFETCH_RANGE(image1),
    });

    /* start loading the PNG file
       we can use the same buffer because we are serializing the request (see sfetch_setup) */
    sfetch_send(&(sfetch_request_t){
        .path = "awesomeface.png",
        .callback = fetch_callback,
        .buffer = SFETCH_RANGE(state.file_buffer),
        .user_data = SFETCH_RANGE(image2),
    });
}

/* The fetch-callback is called by sokol_fetch.h when the data is loaded,
   or when an error has occurred.
*/
static void fetch_callback(const sfetch_response_t* response) {
    if (response->fetched) {
        /* the file data has been fetched, since we provided a big-enough
           buffer we can be sure that all data has been loaded here
        */
        int img_width, img_height, num_channels;
        const int desired_channels = 4;
        stbi_uc* pixels = stbi_load_from_memory(
            response->data.ptr,
            (int)response->data.size,
            &img_width, &img_height,
            &num_channels, desired_channels);
        if (pixels) {
            sg_image image = *(sg_image*)response->user_data;
            sg_init_image(image, &(sg_image_desc) {
                .width = img_width,
                .height = img_height,
                /* set pixel_format to RGBA8 for WebGL */
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .data.subimage[0][0] = {
                    .ptr = pixels,
                    .size = img_width * img_height * 4,
                }
            });
            stbi_image_free(pixels);
        }
    }
    else if (response->failed) {
        // if loading the file failed, set clear color to red
        state.pass_action = (sg_pass_action) {
            .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
        };
    }
}

void frame(void) {
    state.delta_time = stm_laptime(&state.last_time);
    sfetch_dowork();

    HMM_Mat4 view = HMM_LookAt_RH(state.camera_pos, HMM_AddV3(state.camera_pos, state.camera_front), state.camera_up);
    HMM_Mat4 projection = HMM_Perspective_RH_NO(state.fov, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    for(size_t i = 0; i < 10; i++) {
        HMM_Mat4 model = HMM_Translate(state.cube_positions[i]);
        float angle = 20.0f * i; 
        model = HMM_MulM4(model, HMM_Rotate_RH(HMM_AngleDeg(angle), HMM_V3(1.0f, 0.3f, 0.5f)));
        vs_params.model = model;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));

        sg_draw(0, 36, 1);
    }

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
        state.mouse_btn = true;
    } else if (e->type == SAPP_EVENTTYPE_MOUSE_UP) {
        state.mouse_btn = false;
    } else if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }

        if (e->key_code == SAPP_KEYCODE_SPACE) {
            bool mouse_shown = sapp_mouse_shown();
            sapp_show_mouse(!mouse_shown);
        }

        float camera_speed = 5.f * (float) stm_sec(state.delta_time);

        if (e->key_code == SAPP_KEYCODE_W) {
            HMM_Vec3 offset = HMM_MulV3F(state.camera_front, camera_speed);
            state.camera_pos = HMM_AddV3(state.camera_pos, offset);
        }
        if (e->key_code == SAPP_KEYCODE_S) {
            HMM_Vec3 offset = HMM_MulV3F(state.camera_front, camera_speed);
            state.camera_pos = HMM_SubV3(state.camera_pos, offset);
        }
        if (e->key_code == SAPP_KEYCODE_A) {
            HMM_Vec3 offset = HMM_MulV3F(HMM_NormV3(HMM_Cross(state.camera_front, state.camera_up)), camera_speed);
            state.camera_pos = HMM_SubV3(state.camera_pos, offset);
        }
        if (e->key_code == SAPP_KEYCODE_D) {
            HMM_Vec3 offset = HMM_MulV3F(HMM_NormV3(HMM_Cross(state.camera_front, state.camera_up)), camera_speed);
            state.camera_pos = HMM_AddV3(state.camera_pos, offset);
        }
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.mouse_btn) {
        if(state.first_mouse) {
            state.last_x = e->mouse_x;
            state.last_y = e->mouse_y;
            state.first_mouse = false;
        }
    
        float xoffset = e->mouse_x - state.last_x;
        float yoffset = state.last_y - e->mouse_y; 
        state.last_x = e->mouse_x;
        state.last_y = e->mouse_y;

        float sensitivity = 0.001f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        state.yaw   += xoffset;
        state.pitch += yoffset;

        if(state.pitch > 89.0f) {
            state.pitch = 89.0f;
        }
        else if(state.pitch < -89.0f) {
            state.pitch = -89.0f;
        }

        HMM_Vec3 direction;
        direction.X = cosf(HMM_ToRad(state.yaw)) * cosf(HMM_ToRad(state.pitch));
        direction.Y = sinf(HMM_ToRad(state.pitch));
        direction.Z = sinf(HMM_ToRad(state.yaw)) * cosf(HMM_ToRad(state.pitch));
        state.camera_front = HMM_NormV3(direction);
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        if (state.fov >= 1.0f && state.fov <= 45.0f) {
            state.fov -= e->scroll_y;
        }
        if (state.fov <= 1.0f)
            state.fov = 1.0f;
        else if (state.fov >= 45.0f)
            state.fov = 45.0f;
    }
}


void cleanup(void) {
    sg_shutdown();
    sfetch_shutdown();
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
        .window_title = "Look - LearnOpenGL",
    };
}
