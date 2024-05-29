//------------------------------------------------------------------------------
//  1-7-2-rotate-translate
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
#include "transformations.glsl.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint8_t file_buffer[256 * 1024];
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

    float vertices[] = {
        // positions         // texture coords
        0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,   // bottom left
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f    // top left 
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .data = SG_RANGE(vertices),
        .label = "quad-vertices"
    });

    /* an index buffer with 2 triangles */
    uint16_t indices[] = {
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
    };
    state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(indices),
        .data = SG_RANGE(indices),
        .label = "quad-indices"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

    /* create a pipeline object (default render states are fine for triangle) */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2
            }
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
            .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 1.0f, 0.0f, 0.0f, 1.0f } }
        };
    }
}

void frame(void) {
    sfetch_dowork();

    HMM_Mat4 translate = HMM_Translate(HMM_V3(0.5f, -0.5f, 0.0f));
    // 强制声明为 rad 类型的角度
    HMM_Mat4 rotate = HMM_Rotate_RH(HMM_AngleRad((float)stm_sec(stm_now())), HMM_V3(0.0f, 0.0f, 1.0f));
    HMM_Mat4 trans = HMM_MulM4(translate, rotate);

    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    vs_params_t vs_params = {
        .transform = trans
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));

    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
    sfetch_shutdown();
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
        .window_title = "Rotate Translate - LearnOpenGL",
    };
}
