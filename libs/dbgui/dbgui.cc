//------------------------------------------------------------------------------
//  dbgui.cc
//  Implementation file for the generic debug UI overlay, using
//  the sokol_imgui.h utility header which implements the Dear ImGui
//  glue code.
//------------------------------------------------------------------------------
#include <sokol_gfx.h>
#include <sokol_app.h>
#include <sokol_log.h>
#include <math.h>
#include <imgui/imgui.h>
#include <imgui/misc/freetype/imgui_freetype.h>
#include <imgui/misc/fonts/DroidSans.h>
#define SOKOL_IMGUI_IMPL
#include "util/sokol_imgui.h"
#define SOKOL_GFX_IMGUI_IMPL
#include "util/sokol_gfx_imgui.h"

extern "C" {

static sgimgui_t sg_imgui;

static void setupScale(float scale)
{
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar rangesBasic[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x03BC, 0x03BC, // micro
        0x03C3, 0x03C3, // small sigma
        0x2013, 0x2013, // en dash
        0x2264, 0x2264, // less-than or equal to
        0,
    };
    ImFontConfig configBasic;
    configBasic.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryCompressedTTF(
        DroidSans_compressed_data,
        DroidSans_compressed_size,
        round( 15.0f * scale ),
        &configBasic,
        rangesBasic
    );
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
    // No need to upscale the style on macOS, but we need to downscale the fonts.
    io.FontGlobalScale = 1.0f / scale;
    scale = 1.0f;
#endif
    auto& style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark();
    style.WindowBorderSize = 1.f * scale;
    style.FrameBorderSize = 1.f * scale;
    style.FrameRounding = 5.f;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(1, 1, 1, 0.03f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.45f);
    style.ScaleAllSizes(scale);
}

void __dbgui_setup(int sample_count) {
    // setup debug inspection header(s)
    const sgimgui_desc_t desc = { };
    sgimgui_init(&sg_imgui, &desc);

    // setup the sokol-imgui utility header
    simgui_desc_t simgui_desc = { };
    simgui_desc.sample_count = sample_count;
    simgui_desc.logger.func = slog_func;
    simgui_setup(&simgui_desc);
    if (sapp_dpi_scale() != 1.0f)
        setupScale(sapp_dpi_scale());
}

void __dbgui_shutdown(void) {
    sgimgui_discard(&sg_imgui);
    simgui_shutdown();
}

void __dbgui_draw(void) {
    simgui_new_frame({ sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("sokol-gfx")) {
            ImGui::MenuItem("Capabilities", 0, &sg_imgui.caps_window.open);
            ImGui::MenuItem("Buffers", 0, &sg_imgui.buffer_window.open);
            ImGui::MenuItem("Images", 0, &sg_imgui.image_window.open);
            ImGui::MenuItem("Samplers", 0, &sg_imgui.sampler_window.open);
            ImGui::MenuItem("Shaders", 0, &sg_imgui.shader_window.open);
            ImGui::MenuItem("Pipelines", 0, &sg_imgui.pipeline_window.open);
            // NOTE(pabdulin): see https://github.com/floooh/sokol/blob/7f7cd64c6d9d1d4ed08d88a3879b1d69841bf0a4/CHANGELOG.md?plain=1#L69
            // ImGui::MenuItem("Passes", 0, &sg_imgui.passes.open);
            ImGui::MenuItem("Calls", 0, &sg_imgui.capture_window.open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    sgimgui_draw(&sg_imgui);
    simgui_render();
}

void __dbgui_event(const sapp_event* e) {
    simgui_handle_event(e);
}

bool __dbgui_event_with_retval(const sapp_event* e) {
    return simgui_handle_event(e);
}

} // extern "C"