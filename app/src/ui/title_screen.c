#include "title_screen.h"
#include <stdarg.h>
#include <stdio.h>

#include "../network/server.h"
#include "../user.h"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>

static bool is_version_newer(const char* remote, const char* local) {
  int r_major = 0, r_minor = 0, r_patch = 0;
  int l_major = 0, l_minor = 0, l_patch = 0;
  sscanf(remote, "%d.%d.%d", &r_major, &r_minor, &r_patch);
  sscanf(local, "%d.%d.%d", &l_major, &l_minor, &l_patch);
  if (r_major > l_major) return true;
  if (r_major < l_major) return false;
  if (r_minor > l_minor) return true;
  if (r_minor < l_minor) return false;
  return r_patch > l_patch;
}

static bool update_available = false;
static char latest_version[16] = {0};
static bool update_checked = false;

DWORD WINAPI check_update_thread(LPVOID param) {
  HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
  BOOL bResults = FALSE;
  DWORD dwSize = 0;
  LPSTR pszOutBuffer = NULL;

  hSession = WinHttpOpen(L"Vlither Updater/1.0", 
                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                         WINHTTP_NO_PROXY_NAME, 
                         WINHTTP_NO_PROXY_BYPASS, 0);

  if (hSession) {
    hConnect = WinHttpConnect(hSession, L"raw.githubusercontent.com",
                              INTERNET_DEFAULT_HTTPS_PORT, 0);
  }

  if (hConnect) {
    // We will target a version.txt file in the repository path later
    hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/Luckyyt623/Vlither_android/master/version.txt", 
                                  NULL, WINHTTP_NO_REFERER, 
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                  WINHTTP_FLAG_SECURE);
  }

  if (hRequest) {
    bResults = WinHttpSendRequest(hRequest,
                                  WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                  WINHTTP_NO_REQUEST_DATA, 0, 
                                  0, 0);
  }

  if (bResults) {
    bResults = WinHttpReceiveResponse(hRequest, NULL);
  }

  if (bResults) {
    DWORD dwStatusCode = 0;
    DWORD dwStatusSize = sizeof(dwStatusCode);
    if (WinHttpQueryHeaders(hRequest, 
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
                            WINHTTP_HEADER_NAME_BY_INDEX, 
                            &dwStatusCode, &dwStatusSize, WINHTTP_NO_HEADER_INDEX)) {
      if (dwStatusCode == 200) {
        pszOutBuffer = malloc(16);
        if (pszOutBuffer) {
          memset(pszOutBuffer, 0, 16);
          if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, 15, &dwSize)) {
            for (DWORD i = 0; i < dwSize; i++) {
              if (pszOutBuffer[i] == '\r' || pszOutBuffer[i] == '\n' || pszOutBuffer[i] == ' ') {
                pszOutBuffer[i] = '\0';
                break;
              }
            }
            if (dwSize > 0 && is_version_newer(pszOutBuffer, APP_VERSION)) {
              strcpy(latest_version, pszOutBuffer);
              update_available = true;
            }
          }
          free(pszOutBuffer);
        }
      }
    }
  }

  if (hRequest) WinHttpCloseHandle(hRequest);
  if (hConnect) WinHttpCloseHandle(hConnect);
  if (hSession) WinHttpCloseHandle(hSession);

  update_checked = true;
  return 0;
}

void trigger_update_check() {
  if (!update_checked) {
    CreateThread(NULL, 0, check_update_thread, NULL, 0, NULL);
  }
}
#else
static bool update_available = false;
static char latest_version[16] = {0};
void trigger_update_check() {}
#endif

static void igTextCentered(const char* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  ImVec2 sz;
  igCalcTextSize(&sz, buf, NULL, false, -1);
  igSetCursorPosX(igGetWindowWidth() / 2.0f - sz.x / 2.0f);
  igText(buf);
}

void ui_title_screen_init(tenv* env) {
  tuser_data* usr = env->usr;
  usr->viewport_widget.logo_descriptor = igImplVulkan_AddTexture(
      usr->r->linear_sampler, usr->r->logo_tex->view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      
  trigger_update_check();
}

void ui_title_screen(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  user_settings* usrs = &usr->usrs;
  ImGuiStyle* style = igGetStyle();
  ImGuiIO* io = igGetIO_Nil();
  game_data* gdata = &usr->gdata;
  
  // top-right version info
  char version_str[32] = {0};
  sprintf(version_str, "v%s Official", APP_VERSION);
  ImVec2 vtxtsz; igCalcTextSize(&vtxtsz, version_str, NULL, false, -1);
  igSetCursorPosX(ctx->size[0] - vtxtsz.x - 20);
  igSetCursorPosY(20);
  igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL],
             usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
  igTextColored((ImVec4){0.32f, 0.28f, 0.55f, 1.0f}, version_str);
  igPopFont();

  if (update_available) {
    char pill_str[64];
    sprintf(pill_str, "\ue991  Update Available: Restart to update");
    
    igPushFont(usr->imgui_data.regular_font[FONT_SIZE_SMALL], 
               usr->imgui_data.regular_font[FONT_SIZE_SMALL]->LegacySize);
    ImVec2 pill_sz;
    igCalcTextSize(&pill_sz, pill_str, NULL, false, -1);
    
    float pill_w = pill_sz.x + 24.0f;
    float pill_h = 32.0f;
    float pill_x = ctx->size[0] - pill_w - 20.0f;
    float pill_y = 50.0f;
    
    // Draw rounded background pill using drawlist
    ImDrawList* dl = igGetWindowDrawList();
    ImU32 pill_bg = igColorConvertFloat4ToU32((ImVec4){0.75f, 0.15f, 0.15f, 0.85f});
    ImU32 pill_border = igColorConvertFloat4ToU32((ImVec4){0.90f, 0.20f, 0.20f, 1.0f});
    
    ImDrawList_AddRectFilled(dl, (ImVec2){pill_x, pill_y}, (ImVec2){pill_x + pill_w, pill_y + pill_h}, 
                            pill_bg, 16.0f, ImDrawFlags_RoundCornersAll);
    ImDrawList_AddRect(dl, (ImVec2){pill_x, pill_y}, (ImVec2){pill_x + pill_w, pill_y + pill_h}, 
                       pill_border, 16.0f, ImDrawFlags_RoundCornersAll, 1.5f);
                       
    // Text centered inside the pill
    igSetCursorScreenPos((ImVec2){pill_x + 12.0f, pill_y + (pill_h - pill_sz.y) / 2.0f});
    igTextColored((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, pill_str);
    igPopFont();
  }

  usr->r->global.bg_opacity = 0;
  usr->r->global.bd_opacity = 0;
  usr->r->global.minimap_opacity = 0;

  // Center glassmorphism card
  float card_w = 420.0f;
  float card_h = 490.0f;
  float card_x = ctx->size[0] / 2.0f - card_w / 2.0f;
  float card_y = ctx->size[1] / 2.0f - card_h / 2.0f;

  igSetNextWindowPos((ImVec2){card_x, card_y}, ImGuiCond_Always, (ImVec2){});
  igSetNextWindowSize((ImVec2){card_w, card_h}, ImGuiCond_Always);
  
  igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 12.0f);
  igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){20, 20});
  
  igBeginChild_Str("##menu_card", (ImVec2){card_w, card_h}, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  
  // Logo
  float logo_w = 260.0f;
  float logo_h = 75.0f;
  igSetCursorPosX(card_w / 2.0f - logo_w / 2.0f);
  igSetCursorPosY(20);
  igImage((ImTextureRef){ NULL, (ImTextureID)usr->viewport_widget.logo_descriptor }, (ImVec2){logo_w, logo_h}, 
          (ImVec2){0,0}, (ImVec2){1,1});

  // Stats Row
  igSetCursorPosY(115);
  igPushFont(usr->imgui_data.mono_font[FONT_SIZE_SMALL], 
             usr->imgui_data.mono_font[FONT_SIZE_SMALL]->LegacySize);
  
  float col_w = (card_w - 40.0f) / 3.0f;
  
  // Column 0: Best Score
  char score_str[64];
  sprintf(score_str, "\ue99e %d", usrs->score);
  ImVec2 score_sz;
  igCalcTextSize(&score_sz, score_str, NULL, false, -1);
  float x0 = 20.0f + (col_w - score_sz.x) / 2.0f;
  igSetCursorPosX(x0);
  igTextColored((ImVec4){0.15f, 0.68f, 0.37f, 1}, "\ue99e");
  igSameLine(0, -1);
  igTextColored((ImVec4){1, 1, 1, 0.7f}, " %d", usrs->score);
  
  // Column 1: Kills
  char kills_str[64];
  sprintf(kills_str, "\ueaeb %d", usrs->kills);
  ImVec2 kills_sz;
  igCalcTextSize(&kills_sz, kills_str, NULL, false, -1);
  float x1 = 20.0f + col_w + (col_w - kills_sz.x) / 2.0f;
  igSameLine(x1, -1);
  igTextColored((ImVec4){0.8f, 0.2f, 0.2f, 1}, "\ueaeb");
  igSameLine(0, -1);
  igTextColored((ImVec4){1, 1, 1, 0.7f}, " %d", usrs->kills);
  
  // Column 2: Play time
  int tot_sec = (int)usrs->play_time;
  int hours = tot_sec / 3600;
  int minutes = (tot_sec % 3600) / 60;
  int seconds = tot_sec % 60;
  char time_str[64];
  sprintf(time_str, "\ue952 %02d:%02d", hours * 60 + minutes, seconds);
  ImVec2 time_sz;
  igCalcTextSize(&time_sz, time_str, NULL, false, -1);
  float x2 = 20.0f + col_w * 2.0f + (col_w - time_sz.x) / 2.0f;
  igSameLine(x2, -1);
  igTextColored((ImVec4){0.2f, 0.6f, 0.8f, 1}, "\ue952");
  igSameLine(0, -1);
  igTextColored((ImVec4){1, 1, 1, 0.7f}, " %02d:%02d", hours * 60 + minutes, seconds);
  
  igPopFont();

  // Inputs
  igPushFont(usr->imgui_data.regular_font[usrs->ui_font_size],
             usr->imgui_data.regular_font[usrs->ui_font_size]->LegacySize);
             
  float input_w = card_w - 40.0f;
  
  igSetCursorPosY(160);
  igSetCursorPosX(20.0f);
  igPushItemWidth(input_w);
  igInputTextWithHint("##nickname_input", "Nickname", usrs->nickname,
                      MAX_NICKNAME_LEN + 1, ImGuiInputTextFlags_None, NULL, NULL);
                      
  igSetCursorPosY(215);
  igSetCursorPosX(20.0f);
  igInputTextWithHint("##ipv4_input", "Server IP:Port", usrs->ipv4, MAX_IPV4_LEN + 1,
                      ImGuiInputTextFlags_None, NULL, NULL);
  igPopItemWidth();

  // Buttons
  // 1. Play Button (Green)
  igSetCursorPosY(275);
  igSetCursorPosX(20.0f);
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.15f, 0.68f, 0.37f, 0.90f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.18f, 0.80f, 0.44f, 1.00f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.12f, 0.58f, 0.31f, 1.00f});
  if (igButton("\uea1c  PLAY", (ImVec2){input_w, 45})) {
    usr->gdata.conn = CONNECTING;
    usr->gdata.curr_screen = PLAYING;
    usrs->hotkeys[HOTKEY_BOT].active = false;
    glfwSetTime(0);
    server_connect(env);
  }
  igPopStyleColor(3);

  // 2. Skin editor & Settings (Gray/Blue)
  igSetCursorPosY(335);
  igSetCursorPosX(20.0f);
  float btn_half_w = (input_w - 10.0f) / 2.0f;
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.21f, 0.24f, 0.31f, 0.85f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.28f, 0.32f, 0.42f, 0.95f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.16f, 0.18f, 0.24f, 1.00f});
  
  if (igButton("\ue90c  Skin Editor", (ImVec2){btn_half_w, 40})) {
    usr->gdata.curr_screen = SKIN_EDITOR;
  }
  igSameLine(0, 10.0f);
  if (igButton("\ue991  Settings", (ImVec2){btn_half_w, 40})) {
    usr->gdata.conn = CONNECTING;
    usr->gdata.curr_screen = PLAYING;
    usr->gdata.show_settings_overlay = true;
    usr->gdata.settings_opened_from_menu = true;
    usrs->hotkeys[HOTKEY_BOT].active = true;
    glfwSetTime(0);
    server_connect(env);
  }
  igPopStyleColor(3);

  // 3. Quit Button (Soft Red)
  igSetCursorPosY(390);
  igSetCursorPosX(20.0f);
  igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.42f, 0.16f, 0.16f, 0.80f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.55f, 0.20f, 0.20f, 0.95f});
  igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.30f, 0.11f, 0.11f, 1.00f});
  if (igButton("\ue9b6  Quit Game", (ImVec2){input_w, 40})) {
    env->config.running = false;
    save_user_settings(usrs);
  }
  igPopStyleColor(3);

  igPopFont();
  igEndChild();
  igPopStyleVar(2);
}

void ui_title_screen_destroy(tenv* env) {
  tuser_data* usr = env->usr;
  igImplVulkan_RemoveTexture(usr->viewport_widget.logo_descriptor);
}
