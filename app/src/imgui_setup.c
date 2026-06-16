#include "imgui_setup.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS

#include "cimgui/cimgui.h"
#include "cimgui/cimgui_impl.h"

#include "user.h"

void imgui_init(tenv* env) {
  tuser_data* usr = env->usr;

  igCreateContext(NULL);
  igImplVulkan_Init(&(ImGui_ImplVulkan_InitInfo){
      .ApiVersion = VK_API_VERSION_1_0,
      .Instance = env->ctx->instance,
      .PhysicalDevice = env->ctx->ph_device,
      .Device = env->ctx->device,
      .QueueFamily = env->ctx->queue_family,
      .Queue = env->ctx->queue,
      .DescriptorPool = env->ctx->descriptor_pool,
      .DescriptorPoolSize = 0,
      .MinImageCount = env->ctx->min_image_count,
      .ImageCount = env->ctx->fif,
      .PipelineCache = NULL,
      .PipelineInfoMain = {.RenderPass = env->ctx->renderpass,
                           .Subpass = 0,
                           .MSAASamples = VK_SAMPLE_COUNT_1_BIT},
      .UseDynamicRendering = false});
  igImplGlfw_InitForVulkan(env->wnd->handle, true);
  ImGuiIO* io = igGetIO_Nil();
  // io->MouseDrawCursor = true;

  for (int i = 0; i < NUM_FONT_SIZES; i++) {
    int font_sz = (i == FONT_SIZE_TINY) ? 13 : (20 + i * 4);
    float icon_min_advance = (i == FONT_SIZE_TINY) ? 18.0f : (26.0f + i * 6);
    float icon_offset_y = (i == FONT_SIZE_TINY) ? 1.0f : (2.0f + i);

    ImFontConfig icons_config = {.FontDataOwnedByAtlas = true,
                                 .OversampleH = 0,
                                 .OversampleV = 0,
                                 .GlyphMaxAdvanceX = FLT_MAX,
                                 .RasterizerDensity = 1,
                                 .RasterizerMultiply = 1,
                                 .EllipsisChar = 0,
                                 .MergeMode = true,
                                 .GlyphOffset = (ImVec2){0, icon_offset_y},
                                 .GlyphMinAdvanceX = icon_min_advance};

    usr->imgui_data.mono_font[i] = ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "app/res/fonts/mono_regular.ttf", font_sz, NULL, NULL);

    ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "app/res/fonts/iconfont.ttf", font_sz, &icons_config,
        (const ImWchar[]){0xe900, 0xeaea, 0});

    usr->imgui_data.regular_font[i] = ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "app/res/fonts/regular_regular.ttf", font_sz, NULL, NULL);

    ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "app/res/fonts/iconfont.ttf", font_sz, &icons_config,
        (const ImWchar[]){0xe900, 0xeaea, 0});

    usr->imgui_data.mono_font_bold[i] = ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "app/res/fonts/mono_bold.ttf", font_sz, NULL, NULL);

    ImFontAtlas_AddFontFromFileTTF(io->Fonts, "app/res/fonts/iconfont.ttf",
                                   font_sz, &icons_config,
                                   (const ImWchar[]){0xe900, 0xeaea, 0});

    usr->imgui_data.regular_font_bold[i] = ImFontAtlas_AddFontFromFileTTF(
        io->Fonts, "app/res/fonts/regular_bold.ttf", font_sz, NULL, NULL);

    ImFontAtlas_AddFontFromFileTTF(io->Fonts, "app/res/fonts/iconfont.ttf",
                                   font_sz, &icons_config,
                                   (const ImWchar[]){0xe900, 0xeaea, 0});
  }
  
  io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io->IniFilename = NULL;

  ImGuiStyle* style = igGetStyle();
  style->DockingNodeHasCloseButton = false;
  style->WindowMenuButtonPosition = ImGuiDir_None;
  style->TabCloseButtonMinWidthUnselected = -1;
  style->WindowBorderSize = style->FrameBorderSize = style->ChildBorderSize =
      style->PopupBorderSize = style->TabBorderSize = 0; // borderless for clean flat look
  style->FramePadding = (ImVec2){10, 10};
  style->ItemSpacing = (ImVec2){6, 6};
  style->ItemInnerSpacing = (ImVec2){6, 6};
  style->WindowPadding = (ImVec2){12, 12};
  style->GrabMinSize = 18;
  style->FrameRounding = style->TabRounding = style->ChildRounding =
      style->GrabRounding = style->PopupRounding = style->ScrollbarRounding =
          style->WindowRounding = style->TreeLinesRounding = 8; // smooth rounded corners
  style->ScrollbarSize = 10;
  style->DockingSeparatorSize = 1;
  style->ScrollbarPadding = 1;
  style->CellPadding.x = 2;

  igStyleColorsDark(style);
  style->Colors[ImGuiCol_Text] = (ImVec4){0.95f, 0.95f, 0.98f, 1.00f};
  style->Colors[ImGuiCol_WindowBg] = style->Colors[ImGuiCol_PopupBg] =
      (ImVec4){0.08f, 0.08f, 0.10f, 0.85f}; // translucent glass dark bg
  style->Colors[ImGuiCol_ChildBg] = (ImVec4){0.12f, 0.12f, 0.15f, 0.40f};
  style->Colors[ImGuiCol_Border] = (ImVec4){0.20f, 0.20f, 0.25f, 0.35f}; // soft border
  style->Colors[ImGuiCol_BorderShadow] = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
  style->Colors[ImGuiCol_FrameBg] = (ImVec4){0.15f, 0.15f, 0.18f, 0.70f};
  style->Colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.22f, 0.22f, 0.28f, 0.80f};
  style->Colors[ImGuiCol_FrameBgActive] = (ImVec4){0.10f, 0.10f, 0.12f, 0.95f};
  style->Colors[ImGuiCol_TitleBgActive] = (ImVec4){0.11f, 0.11f, 0.14f, 1.00f};
  style->Colors[ImGuiCol_Button] = (ImVec4){0.24f, 0.21f, 0.40f, 0.80f}; // premium purple/indigo
  style->Colors[ImGuiCol_ButtonHovered] = (ImVec4){0.32f, 0.28f, 0.55f, 0.95f};
  style->Colors[ImGuiCol_ButtonActive] = (ImVec4){0.18f, 0.16f, 0.32f, 1.00f};
  style->Colors[ImGuiCol_Header] = (ImVec4){0.24f, 0.21f, 0.40f, 0.60f};
  style->Colors[ImGuiCol_HeaderHovered] = (ImVec4){0.32f, 0.28f, 0.55f, 0.80f};
  style->Colors[ImGuiCol_HeaderActive] = (ImVec4){0.18f, 0.16f, 0.32f, 1.00f};
  style->Colors[ImGuiCol_TabHovered] = (ImVec4){0.32f, 0.28f, 0.55f, 0.80f};
  style->Colors[ImGuiCol_Tab] = (ImVec4){0.18f, 0.18f, 0.22f, 0.80f};
  style->Colors[ImGuiCol_TabSelected] = (ImVec4){0.24f, 0.21f, 0.40f, 1.00f};
  style->Colors[ImGuiCol_TabSelectedOverline] = (ImVec4){0.32f, 0.28f, 0.55f, 0.00f};
  style->Colors[ImGuiCol_TabDimmed] = (ImVec4){0.12f, 0.12f, 0.15f, 0.80f};
  style->Colors[ImGuiCol_TabDimmedSelected] = (ImVec4){0.18f, 0.18f, 0.22f, 1.00f};
  style->Colors[ImGuiCol_ModalWindowDimBg] = (ImVec4){0.02f, 0.02f, 0.03f, 0.60f};
  style->Colors[ImGuiCol_SliderGrab] = (ImVec4){0.32f, 0.28f, 0.55f, 0.90f};
  style->Colors[ImGuiCol_SliderGrabActive] = (ImVec4){0.42f, 0.38f, 0.70f, 1.00f};
}

void imgui_prerender() {
  igImplVulkan_NewFrame();
  igImplGlfw_NewFrame();
  igNewFrame();
}

void imgui_render(VkCommandBuffer cmd) {
  igImplVulkan_RenderDrawData(igGetDrawData(), cmd, NULL);
}

void imgui_destroy() {
  igImplGlfw_Shutdown();
  igImplVulkan_Shutdown();
  igDestroyContext(NULL);
}