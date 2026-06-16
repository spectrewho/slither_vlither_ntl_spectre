#include "settings.h"
#include "../user.h"

void ui_settings_init(tenv* env) {}

static int waiting_for_key_idx = -1;

bool ui_settings_waiting_for_key(void) {
  return waiting_for_key_idx != -1;
}

const char* get_key_name(int key) {
  if (key >= 'A' && key <= 'Z') {
    static char buf[26][2] = {0};
    int idx = key - 'A';
    buf[idx][0] = (char)key;
    buf[idx][1] = '\0';
    return buf[idx];
  }
  if (key >= '0' && key <= '9') {
    static char buf[10][2] = {0};
    int idx = key - '0';
    buf[idx][0] = (char)key;
    buf[idx][1] = '\0';
    return buf[idx];
  }
  switch (key) {
    case GLFW_KEY_SPACE: return "Space";
    case GLFW_KEY_ESCAPE: return "Esc";
    case GLFW_KEY_ENTER: return "Enter";
    case GLFW_KEY_TAB: return "Tab";
    case GLFW_KEY_BACKSPACE: return "Bksp";
    case GLFW_KEY_INSERT: return "Ins";
    case GLFW_KEY_DELETE: return "Del";
    case GLFW_KEY_RIGHT: return "Right";
    case GLFW_KEY_LEFT: return "Left";
    case GLFW_KEY_DOWN: return "Down";
    case GLFW_KEY_UP: return "Up";
    case GLFW_KEY_PAGE_UP: return "PgUp";
    case GLFW_KEY_PAGE_DOWN: return "PgDn";
    case GLFW_KEY_HOME: return "Home";
    case GLFW_KEY_END: return "End";
    case GLFW_KEY_CAPS_LOCK: return "Caps";
    case GLFW_KEY_SCROLL_LOCK: return "ScrLk";
    case GLFW_KEY_NUM_LOCK: return "NumLk";
    case GLFW_KEY_PRINT_SCREEN: return "PrtSc";
    case GLFW_KEY_PAUSE: return "Pause";
    case GLFW_KEY_F1: return "F1";
    case GLFW_KEY_F2: return "F2";
    case GLFW_KEY_F3: return "F3";
    case GLFW_KEY_F4: return "F4";
    case GLFW_KEY_F5: return "F5";
    case GLFW_KEY_F6: return "F6";
    case GLFW_KEY_F7: return "F7";
    case GLFW_KEY_F8: return "F8";
    case GLFW_KEY_F9: return "F9";
    case GLFW_KEY_F10: return "F10";
    case GLFW_KEY_F11: return "F11";
    case GLFW_KEY_F12: return "F12";
    case GLFW_KEY_KP_0: return "KP0";
    case GLFW_KEY_KP_1: return "KP1";
    case GLFW_KEY_KP_2: return "KP2";
    case GLFW_KEY_KP_3: return "KP3";
    case GLFW_KEY_KP_4: return "KP4";
    case GLFW_KEY_KP_5: return "KP5";
    case GLFW_KEY_KP_6: return "KP6";
    case GLFW_KEY_KP_7: return "KP7";
    case GLFW_KEY_KP_8: return "KP8";
    case GLFW_KEY_KP_9: return "KP9";
    case GLFW_KEY_KP_DECIMAL: return "KP.";
    case GLFW_KEY_KP_DIVIDE: return "KP/";
    case GLFW_KEY_KP_MULTIPLY: return "KP*";
    case GLFW_KEY_KP_SUBTRACT: return "KP-";
    case GLFW_KEY_KP_ADD: return "KP+";
    case GLFW_KEY_KP_ENTER: return "KPEnt";
    case GLFW_KEY_KP_EQUAL: return "KP=";
    case GLFW_KEY_LEFT_SHIFT: return "LShift";
    case GLFW_KEY_LEFT_CONTROL: return "LCtrl";
    case GLFW_KEY_LEFT_ALT: return "LAlt";
    case GLFW_KEY_LEFT_SUPER: return "LSuper";
    case GLFW_KEY_RIGHT_SHIFT: return "RShift";
    case GLFW_KEY_RIGHT_CONTROL: return "RCtrl";
    case GLFW_KEY_RIGHT_ALT: return "RAlt";
    case GLFW_KEY_RIGHT_SUPER: return "RSuper";
    case GLFW_KEY_MENU: return "Menu";
    case GLFW_KEY_PERIOD: return ".";
    case GLFW_KEY_COMMA: return ",";
    case GLFW_KEY_MINUS: return "-";
    case GLFW_KEY_EQUAL: return "=";
    case GLFW_KEY_LEFT_BRACKET: return "[";
    case GLFW_KEY_RIGHT_BRACKET: return "]";
    case GLFW_KEY_SEMICOLON: return ";";
    case GLFW_KEY_APOSTROPHE: return "'";
    case GLFW_KEY_BACKSLASH: return "\\";
    case GLFW_KEY_SLASH: return "/";
    case GLFW_KEY_GRAVE_ACCENT: return "`";
    default: {
      static char fallback[16];
      sprintf(fallback, "Key%d", key);
      return fallback;
    }
  }
}

// Helper: close settings overlay and optionally disconnect
static void close_settings(game_data* gdata) {
  gdata->show_settings_overlay = false;
  if (gdata->settings_opened_from_menu) {
    if (gdata->connection) gdata->connection->is_closing = true;
    gdata->settings_opened_from_menu = false;
  }
  if (gdata->curr_screen == SETTINGS) gdata->curr_screen = TITLE_SCREEN;
}

static char edit_team_id[129] = {0};
static char edit_auth_key[65] = {0};
static char edit_save_as[33] = {0};
static bool edit_buffers_initialized = false;
static bool show_keys = false;

static void init_edit_buffers(user_settings* usrs) {
  if (usrs->ntl_active_team_idx >= 0 && usrs->ntl_active_team_idx < usrs->ntl_team_count) {
    strcpy(edit_team_id, usrs->ntl_teams[usrs->ntl_active_team_idx].team_id);
    strcpy(edit_auth_key, usrs->ntl_teams[usrs->ntl_active_team_idx].auth_key);
    strcpy(edit_save_as, usrs->ntl_teams[usrs->ntl_active_team_idx].name);
  } else {
    edit_team_id[0] = '\0';
    edit_auth_key[0] = '\0';
    edit_save_as[0] = '\0';
  }
  edit_buffers_initialized = true;
}

#define BEGIN_CARD(id, title) \
  igPushStyleColor_Vec4(ImGuiCol_ChildBg, (ImVec4){0.05f, 0.06f, 0.09f, 0.45f}); \
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.22f, 0.30f, 0.35f}); \
  igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 12.0f); \
  igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){16, 14}); \
  igBeginChild_Str(id, (ImVec2){0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse); \
  igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR], \
             usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize); \
  igTextColored((ImVec4){0.06f, 0.73f, 0.51f, 1.0f}, title); \
  igPopFont()

#define BEGIN_CARD_SEPARATOR() \
  igDummy((ImVec2){0, 4}); \
  igSeparator(); \
  igDummy((ImVec2){0, 8})

#define END_CARD() \
  igEndChild(); \
  igPopStyleVar(2); \
  igPopStyleColor(2)

// Helper macros for 2-column label+widget rows
#define SETTINGS_ROW_BEGIN(label_str) \
  igTableNextRow(ImGuiTableRowFlags_None, 0); \
  igTableSetColumnIndex(0); \
  igAlignTextToFramePadding(); \
  igText(label_str); \
  igTableSetColumnIndex(1)

#define SETTINGS_TABLE_BEGIN(id) \
  igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){4, 4}); \
  if (igBeginTable(id, 2, ImGuiTableFlags_None, (ImVec2){}, 0)) { \
    igTableSetupColumn("lbl", ImGuiTableColumnFlags_WidthStretch, 0.45f, 0); \
    igTableSetupColumn("wgt", ImGuiTableColumnFlags_WidthStretch, 0.55f, 0)

#define SETTINGS_TABLE_END() \
    igEndTable(); \
  } \
  igPopStyleVar(1)

void ui_settings(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;

  // Initialize edit buffers when settings screen opens
  static bool was_shown = false;
  if (gdata->show_settings_overlay && !was_shown) {
    init_edit_buffers(usrs);
    was_shown = true;
  }
  if (!gdata->show_settings_overlay) {
    was_shown = false;
  }

  // ─── Key binder polling ───
  if (waiting_for_key_idx != -1) {
    for (int k = 32; k <= 348; k++) {
      if (tkeyboard_key_pressed(env->kb, k)) {
        if (k == GLFW_KEY_ESCAPE) {
          waiting_for_key_idx = -1;
        } else {
          bool in_use = false;
          for (int d = 0; d < NUM_HOTKEYS; d++)
            if (k == usrs->hotkeys[d].key && waiting_for_key_idx != d) in_use = true;
          if (!in_use) {
            usrs->hotkeys[waiting_for_key_idx].key = k;
            save_user_settings(usrs);
          }
          waiting_for_key_idx = -1;
        }
        break;
      }
    }
  }

  // ─── Window setup ───
  igPushFont(usr->imgui_data.regular_font[usrs->ui_font_size],
             usr->imgui_data.regular_font[usrs->ui_font_size]->LegacySize);

  float w = 780.0f, h = 540.0f;
  float x = ctx->size[0] * 0.5f - w * 0.5f;
  float y = ctx->size[1] * 0.5f - h * 0.5f;

  igSetNextWindowPos((ImVec2){x, y}, ImGuiCond_Always, (ImVec2){});
  igSetNextWindowSize((ImVec2){w, h}, ImGuiCond_Always);

  // Glassmorphism styling
  igPushStyleColor_Vec4(ImGuiCol_WindowBg, (ImVec4){0.05f, 0.05f, 0.07f, 0.88f});
  igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.20f, 0.22f, 0.30f, 0.40f});
  igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 16.0f);
  igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){20, 20});

  igBegin("Settings", NULL,
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

  // ─── Header bar ───
  {
    igPushFont(usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR],
               usr->imgui_data.regular_font_bold[FONT_SIZE_REGULAR]->LegacySize);
    igTextColored((ImVec4){0.06f, 0.73f, 0.51f, 1.0f}, "\ue991");
    igSameLine(0, 8);
    igText("Settings Dashboard");
    igPopFont();

    // Close button
    igSameLine(w - 44, -1);
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0, 0, 0, 0});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.85f, 0.25f, 0.25f, 0.40f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.85f, 0.25f, 0.25f, 0.70f});
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 6.0f);
    if (igButton("X", (ImVec2){26, 26})) close_settings(gdata);
    igPopStyleVar(1);
    igPopStyleColor(3);
  }

  igDummy((ImVec2){0, 8});
  igSeparator();
  igDummy((ImVec2){0, 8});

  float content_w = w - 40.0f;
  ImVec2 avail;
  igGetContentRegionAvail(&avail);
  float content_h = avail.y - 48.0f;

  // ─── Single Scroll Pane ───
  igBeginChild_Str("settings_scroll_pane", (ImVec2){content_w, content_h}, ImGuiChildFlags_None, ImGuiWindowFlags_None);

  // Table Grid Layout for Cards
  igPushStyleVar_Vec2(ImGuiStyleVar_CellPadding, (ImVec2){8, 8});
  if (igBeginTable("settings_grid", 2, ImGuiTableFlags_None, (ImVec2){content_w, 0}, 0)) {
    igTableSetupColumn("left_col", ImGuiTableColumnFlags_WidthStretch, 0.5f, 0);
    igTableSetupColumn("right_col", ImGuiTableColumnFlags_WidthStretch, 0.5f, 0);

    // Row 1: General Interface & General Controls
    igTableNextRow(ImGuiTableRowFlags_None, 0);
    igTableSetColumnIndex(0);
    {
      BEGIN_CARD("gen_interface", "General: Interface");
      
      // Reset defaults button inside the card header
      ImVec2 card_avail;
      igGetContentRegionAvail(&card_avail);
      igSameLine(card_avail.x - 110, 0);
      igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.18f, 0.20f, 0.25f, 0.50f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.25f, 0.28f, 0.35f, 0.70f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.15f, 0.16f, 0.20f, 0.90f});
      igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 4.0f);
      if (igButton("Reset Defaults", (ImVec2){110, 20})) {
        ntl_team_profile ntl_teams_tmp[12];
        int team_count_tmp = usrs->ntl_team_count;
        int active_team_idx_tmp = usrs->ntl_active_team_idx;
        char user_id_tmp[9];
        memcpy(ntl_teams_tmp, usrs->ntl_teams, sizeof(usrs->ntl_teams));
        strcpy(user_id_tmp, usrs->ntl_user_id);

        user_settings_default(usrs);

        memcpy(usrs->ntl_teams, ntl_teams_tmp, sizeof(usrs->ntl_teams));
        usrs->ntl_team_count = team_count_tmp;
        usrs->ntl_active_team_idx = active_team_idx_tmp;
        strcpy(usrs->ntl_user_id, user_id_tmp);

        init_edit_buffers(usrs);

        env->config.vsync = usrs->vsync;
        twindow_request_refresh(env->wnd);
      }
      igPopStyleVar(1);
      igPopStyleColor(3);

      BEGIN_CARD_SEPARATOR();
      SETTINGS_TABLE_BEGIN("gen_interface_tbl");

      SETTINGS_ROW_BEGIN("VSync");
      if (igCheckbox("##vsync", &usrs->vsync)) {
        env->config.vsync = usrs->vsync;
        twindow_request_refresh(env->wnd);
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Cursor size");
      igSetNextItemWidth(-1);
      if (igSliderInt("##cursor", &usrs->cursor_size, 16, 64, "%d px", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Minimap size");
      igSetNextItemWidth(-1);
      if (igSliderInt("##minimap", &usrs->minimap_size, 128, 512, "%d px", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Border color");
      igSetNextItemWidth(-1);
      if (igColorEdit3("##bdcol", usrs->bd_color, ImGuiColorEditFlags_NoInputs)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("UI font");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##uifont", (int*)&usrs->ui_font_size, (const char*[]){"Small", "Regular", "Large"}, 3, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Stats font");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##statfont", (int*)&usrs->stats_font_size, (const char*[]){"Small", "Regular", "Large"}, 3, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Leaderboard font");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##lbfont", (int*)&usrs->lb_font_size, (const char*[]){"Small", "Regular", "Large"}, 3, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Names font");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##namefont", (int*)&usrs->snake_names_font_size, (const char*[]){"Small", "Regular", "Large"}, 3, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Show scores");
      if (igCheckbox("##scores", &usrs->snake_scores)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Show NTL Chat");
      if (igCheckbox("##show_chat", &usrs->show_chat_hud)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Show NTL Players");
      if (igCheckbox("##show_online", &usrs->show_online_players_hud)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Show NTL List");
      if (igCheckbox("##show_details", &usrs->show_player_details_hud)) {
        save_user_settings(usrs);
      }

      SETTINGS_TABLE_END();
      END_CARD();
      igDummy((ImVec2){0, 8});
    }

    igTableSetColumnIndex(1);
    {
      BEGIN_CARD("gen_controls", "General: Controls & Bot");
      BEGIN_CARD_SEPARATOR();
      SETTINGS_TABLE_BEGIN("gen_controls_tbl");

      SETTINGS_ROW_BEGIN("Smooth zoom");
      if (igCheckbox("##smzoom", &usrs->smooth_zoom)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Zoom step");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##zstep", &usrs->zoom_step, 0.05f, 0.5f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Instant restart");
      if (igCheckbox("##irestart", &usrs->instant_restart)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Restart (right click)");
      if (igCheckbox("##rrc", &usrs->restart_rc)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Quit (middle click)");
      if (igCheckbox("##qmc", &usrs->quit_mc)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Laser color");
      igSetNextItemWidth(-1);
      if (igColorEdit4("##lascol", usrs->laser_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Laser thickness");
      igSetNextItemWidth(-1);
      if (igSliderInt("##lasthk", &usrs->laser_thickness, 1, 4, "%d px", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Bot circle score");
      igSetNextItemWidth(-1);
      if (igSliderInt("##bcirc", &usrs->bot_follow_circle_score, 1000, 6000, "%d", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Bot radius mult");
      igSetNextItemWidth(-1);
      if (igSliderInt("##brad", &usrs->bot_radius_mult, 10, 40, "%dx", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_TABLE_END();
      END_CARD();
      igDummy((ImVec2){0, 8});
    }

    // Row 2: Normal Mode Rendering & Normal Mode Effects
    igTableNextRow(ImGuiTableRowFlags_None, 0);
    igTableSetColumnIndex(0);
    {
      igPushID_Int(100);
      gameplay_mode* mode = usrs->modes + 0;

      BEGIN_CARD("normal_rendering", "Normal Mode: Rendering");
      BEGIN_CARD_SEPARATOR();
      SETTINGS_TABLE_BEGIN("normal_rendering_tbl");

      SETTINGS_ROW_BEGIN("Crosshair");
      if (igCheckbox("##xhair", &mode->show_crosshair)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Background");
      if (igCheckbox("##bg", &mode->show_background)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("BG scale");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##bgs", &mode->bg_scale, 0.05f, 4.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Accessories");
      if (igCheckbox("##acc", &mode->show_accessories)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Shadows");
      if (igCheckbox("##shad", &mode->show_shadows)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Name outlines");
      if (igCheckbox("##nout", &mode->player_names_outline)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Death effect");
      if (igCheckbox("##deff", &mode->death_effect)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Segment gap");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##qsm", &mode->qsm, 1.0f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Render mode");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##rmode", &mode->render_mode, (const char*[]){"Texture", "Solid", "Flat"}, 3, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_TABLE_END();
      END_CARD();
      igPopID();
      igDummy((ImVec2){0, 8});
    }

    igTableSetColumnIndex(1);
    {
      igPushID_Int(100);
      gameplay_mode* mode = usrs->modes + 0;

      BEGIN_CARD("normal_effects", "Normal Mode: Effects & Food");
      BEGIN_CARD_SEPARATOR();
      SETTINGS_TABLE_BEGIN("normal_effects_tbl");

      SETTINGS_ROW_BEGIN("Boost effect");
      if (igCheckbox("##boost", &mode->show_boost)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Boost type");
      igBeginDisabled(!mode->show_boost);
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##btype", &mode->boost_type, (const char*[]){"Normal", "Simple"}, 2, -1)) {
        save_user_settings(usrs);
      }
      igEndDisabled();

      SETTINGS_ROW_BEGIN("Boost strength");
      igBeginDisabled(!mode->show_boost);
      igSetNextItemWidth(-1);
      if (igSliderFloat("##bstr", &mode->boost_strength, 0.25f, 3.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }
      igEndDisabled();

      SETTINGS_ROW_BEGIN("Food shader");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##ftype", &mode->food_type, (const char*[]){"Solid", "Rings"}, 2, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food scale");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##fscale", &mode->food_scale, 0.25f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food float");
      if (igCheckbox("##fflt", &mode->food_float)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food flicker");
      if (igCheckbox("##fflk", &mode->food_flicker)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Uniform color");
      if (igCheckbox("##ufc", &mode->uniform_food_color)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food color");
      igBeginDisabled(!mode->uniform_food_color);
      igSetNextItemWidth(-1);
      if (igColorEdit3("##fcol", mode->food_color, ImGuiColorEditFlags_NoInputs)) {
        save_user_settings(usrs);
      }
      igEndDisabled();

      SETTINGS_TABLE_END();
      END_CARD();
      igPopID();
      igDummy((ImVec2){0, 8});
    }

    // Row 3: Assist Mode Rendering & Assist Mode Effects
    igTableNextRow(ImGuiTableRowFlags_None, 0);
    igTableSetColumnIndex(0);
    {
      igPushID_Int(101);
      gameplay_mode* mode = usrs->modes + 1;

      BEGIN_CARD("assist_rendering", "Assist Mode: Rendering");
      BEGIN_CARD_SEPARATOR();
      SETTINGS_TABLE_BEGIN("assist_rendering_tbl");

      SETTINGS_ROW_BEGIN("Crosshair");
      if (igCheckbox("##xhair", &mode->show_crosshair)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Background");
      if (igCheckbox("##bg", &mode->show_background)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("BG scale");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##bgs", &mode->bg_scale, 0.05f, 4.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Accessories");
      if (igCheckbox("##acc", &mode->show_accessories)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Shadows");
      if (igCheckbox("##shad", &mode->show_shadows)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Name outlines");
      if (igCheckbox("##nout", &mode->player_names_outline)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Death effect");
      if (igCheckbox("##deff", &mode->death_effect)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Segment gap");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##qsm", &mode->qsm, 1.0f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Render mode");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##rmode", &mode->render_mode, (const char*[]){"Texture", "Solid", "Flat"}, 3, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_TABLE_END();
      END_CARD();
      igPopID();
      igDummy((ImVec2){0, 8});
    }

    igTableSetColumnIndex(1);
    {
      igPushID_Int(101);
      gameplay_mode* mode = usrs->modes + 1;

      BEGIN_CARD("assist_effects", "Assist Mode: Effects & Food");
      BEGIN_CARD_SEPARATOR();
      SETTINGS_TABLE_BEGIN("assist_effects_tbl");

      SETTINGS_ROW_BEGIN("Boost effect");
      if (igCheckbox("##boost", &mode->show_boost)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Boost type");
      igBeginDisabled(!mode->show_boost);
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##btype", &mode->boost_type, (const char*[]){"Normal", "Simple"}, 2, -1)) {
        save_user_settings(usrs);
      }
      igEndDisabled();

      SETTINGS_ROW_BEGIN("Boost strength");
      igBeginDisabled(!mode->show_boost);
      igSetNextItemWidth(-1);
      if (igSliderFloat("##bstr", &mode->boost_strength, 0.25f, 3.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }
      igEndDisabled();

      SETTINGS_ROW_BEGIN("Food shader");
      igSetNextItemWidth(-1);
      if (igCombo_Str_arr("##ftype", &mode->food_type, (const char*[]){"Solid", "Rings"}, 2, -1)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food scale");
      igSetNextItemWidth(-1);
      if (igSliderFloat("##fscale", &mode->food_scale, 0.25f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food float");
      if (igCheckbox("##fflt", &mode->food_float)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food flicker");
      if (igCheckbox("##fflk", &mode->food_flicker)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Uniform color");
      if (igCheckbox("##ufc", &mode->uniform_food_color)) {
        save_user_settings(usrs);
      }

      SETTINGS_ROW_BEGIN("Food color");
      igBeginDisabled(!mode->uniform_food_color);
      igSetNextItemWidth(-1);
      if (igColorEdit3("##fcol", mode->food_color, ImGuiColorEditFlags_NoInputs)) {
        save_user_settings(usrs);
      }
      igEndDisabled();

      SETTINGS_TABLE_END();
      END_CARD();
      igPopID();
      igDummy((ImVec2){0, 8});
    }

    // Row 4: Action Bindings & NTL Mod & Team Settings
    igTableNextRow(ImGuiTableRowFlags_None, 0);
    igTableSetColumnIndex(0);
    {
      BEGIN_CARD("hk_card", "Action Bindings (Hotkeys)");
      BEGIN_CARD_SEPARATOR();

      igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){8, 6});
      if (igBeginTable("hk_tbl", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH, (ImVec2){}, 0)) {
        igTableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.45f, 0);
        igTableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 100.0f, 0);
        igTableSetupColumn("Mode", ImGuiTableColumnFlags_WidthStretch, 0.35f, 0);
        igTableHeadersRow();

        for (int i = 0; i < NUM_HOTKEYS; i++) {
          hotkey* hk = usrs->hotkeys + i;
          igTableNextRow(ImGuiTableRowFlags_None, 0);

          igTableSetColumnIndex(0);
          igAlignTextToFramePadding();
          igText(hk->description);

          igTableSetColumnIndex(1);
          igPushID_Int(i);

          bool is_binding = (waiting_for_key_idx == i);
          if (is_binding) {
            igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.90f, 0.55f, 0.08f, 0.70f});
            igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.95f, 0.60f, 0.15f, 0.90f});
            igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.80f, 0.45f, 0.05f, 1.0f});
          } else {
            igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.12f, 0.14f, 0.20f, 0.50f});
            igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.18f, 0.22f, 0.30f, 0.70f});
            igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.08f, 0.10f, 0.15f, 0.90f});
          }
          igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 6.0f);

          const char* btn_text = is_binding ? "Press Key..." : get_key_name(hk->key);
          if (igButton(btn_text, (ImVec2){-1, 26})) {
            waiting_for_key_idx = is_binding ? -1 : i;
          }

          igPopStyleVar(1);
          igPopStyleColor(3);

          igTableSetColumnIndex(2);
          igSetNextItemWidth(-1);
          if (i == HOTKEY_RESTART || i == HOTKEY_QUIT) {
            igBeginDisabled(true);
            igCombo_Str_arr("##m", &(int){0}, (const char*[]){"Toggle"}, 1, -1);
            igEndDisabled();
          } else {
            if (igCombo_Str_arr("##m", &hk->mode, (const char*[]){"Toggle", "Hold"}, 2, -1)) {
              save_user_settings(usrs);
            }
          }
          igPopID();
        }

        igEndTable();
      }
      igPopStyleVar(1);

      END_CARD();
      igDummy((ImVec2){0, 8});
    }

    igTableSetColumnIndex(1);
    {
      BEGIN_CARD("ntl_card", "NTL Mod & Team Settings");
      BEGIN_CARD_SEPARATOR();

      if (igBeginTable("ntl_inputs_tbl", 2, ImGuiTableFlags_None, (ImVec2){}, 0)) {
        igTableSetupColumn("lbl", ImGuiTableColumnFlags_WidthFixed, 80.0f, 0);
        igTableSetupColumn("val", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

        igTableNextRow(0, 0);
        igTableSetColumnIndex(0);
        igAlignTextToFramePadding();
        igText("Team ID:");
        igTableSetColumnIndex(1);
        igSetNextItemWidth(-1);
        if (igInputText("##ntl_tid", edit_team_id, sizeof(edit_team_id), ImGuiInputTextFlags_None, NULL, NULL)) {
          if (usrs->ntl_active_team_idx >= 0 && usrs->ntl_active_team_idx < usrs->ntl_team_count) {
            strcpy(usrs->ntl_teams[usrs->ntl_active_team_idx].team_id, edit_team_id);
            save_user_settings(usrs);
          }
        }

        igTableNextRow(0, 0);
        igTableSetColumnIndex(0);
        igAlignTextToFramePadding();
        igText("Auth Key:");
        igTableSetColumnIndex(1);
        igSetNextItemWidth(-1);
        if (igInputText("##ntl_auth", edit_auth_key, sizeof(edit_auth_key),
                    show_keys ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password, NULL, NULL)) {
          if (usrs->ntl_active_team_idx >= 0 && usrs->ntl_active_team_idx < usrs->ntl_team_count) {
            strcpy(usrs->ntl_teams[usrs->ntl_active_team_idx].auth_key, edit_auth_key);
            save_user_settings(usrs);
          }
        }

        igTableNextRow(0, 0);
        igTableSetColumnIndex(0);
        igAlignTextToFramePadding();
        igText("Save as:");
        igTableSetColumnIndex(1);
        igSetNextItemWidth(-1);
        if (igInputText("##ntl_name", edit_save_as, sizeof(edit_save_as), ImGuiInputTextFlags_None, NULL, NULL)) {
          if (usrs->ntl_active_team_idx >= 0 && usrs->ntl_active_team_idx < usrs->ntl_team_count) {
            strcpy(usrs->ntl_teams[usrs->ntl_active_team_idx].name, edit_save_as);
            save_user_settings(usrs);
          }
        }

        igEndTable();
      }

      igDummy((ImVec2){0, 6});

      igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){8, 5});

      ImVec2 avail;
      igGetContentRegionAvail(&avail);
      float avail_w = avail.x;

      // --- Row 1: Saved keys combo + Show/Hide keys ---
      // Saved keys combo (green)
      igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.06f, 0.60f, 0.35f, 0.80f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.06f, 0.73f, 0.51f, 1.0f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.04f, 0.55f, 0.38f, 1.0f});
      const char* combo_preview = "Saved keys";
      if (usrs->ntl_active_team_idx >= 0 && usrs->ntl_active_team_idx < usrs->ntl_team_count) {
        combo_preview = usrs->ntl_teams[usrs->ntl_active_team_idx].name;
      }
      igSetNextItemWidth(avail_w - 108.0f);
      if (igBeginCombo("##saved_keys", combo_preview, ImGuiComboFlags_None)) {
        for (int i = 0; i < usrs->ntl_team_count; i++) {
          bool is_selected = (usrs->ntl_active_team_idx == i);
          if (igSelectable_Bool(usrs->ntl_teams[i].name, is_selected, ImGuiSelectableFlags_None, (ImVec2){0, 0})) {
            usrs->ntl_active_team_idx = i;
            strcpy(edit_team_id, usrs->ntl_teams[i].team_id);
            strcpy(edit_auth_key, usrs->ntl_teams[i].auth_key);
            strcpy(edit_save_as, usrs->ntl_teams[i].name);
            save_user_settings(usrs);
          }
        }
        igEndCombo();
      }
      igPopStyleColor(3);

      igSameLine(0, 8.0f);

      // Show current keys button (green)
      igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.06f, 0.60f, 0.35f, 0.80f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.06f, 0.73f, 0.51f, 1.0f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.04f, 0.55f, 0.38f, 1.0f});
      if (igButton(show_keys ? "Hide keys" : "Show keys", (ImVec2){100, 26})) {
        show_keys = !show_keys;
      }
      igPopStyleColor(3);

      igDummy((ImVec2){0, 4.0f}); // spacing between rows

      // --- Row 2: Save + Delete + no team ---
      float btn_w = (avail_w - 16.0f) / 3.0f;

      // Save button (green)
      igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.06f, 0.60f, 0.35f, 0.80f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.06f, 0.73f, 0.51f, 1.0f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.04f, 0.55f, 0.38f, 1.0f});
      if (igButton("Save", (ImVec2){btn_w, 26})) {
        if (edit_save_as[0] != '\0' && edit_team_id[0] != '\0') {
          int found_idx = -1;
          for (int i = 0; i < usrs->ntl_team_count; i++) {
            if (strcmp(usrs->ntl_teams[i].name, edit_save_as) == 0) {
              found_idx = i;
              break;
            }
          }
          if (found_idx != -1) {
            strcpy(usrs->ntl_teams[found_idx].team_id, edit_team_id);
            strcpy(usrs->ntl_teams[found_idx].auth_key, edit_auth_key);
            usrs->ntl_active_team_idx = found_idx;
          } else if (usrs->ntl_team_count < 12) {
            int new_idx = usrs->ntl_team_count;
            strcpy(usrs->ntl_teams[new_idx].name, edit_save_as);
            strcpy(usrs->ntl_teams[new_idx].team_id, edit_team_id);
            strcpy(usrs->ntl_teams[new_idx].auth_key, edit_auth_key);
            usrs->ntl_active_team_idx = new_idx;
            usrs->ntl_team_count++;
          }
          save_user_settings(usrs);
        }
      }
      igPopStyleColor(3);

      igSameLine(0, 8.0f);

      // Delete button (red)
      igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.70f, 0.15f, 0.15f, 0.80f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.85f, 0.20f, 0.20f, 1.0f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.55f, 0.10f, 0.10f, 1.0f});
      if (igButton("Delete", (ImVec2){btn_w, 26})) {
        int idx = usrs->ntl_active_team_idx;
        if (idx >= 0 && idx < usrs->ntl_team_count) {
          for (int j = idx; j < usrs->ntl_team_count - 1; j++) {
            usrs->ntl_teams[j] = usrs->ntl_teams[j + 1];
          }
          usrs->ntl_team_count--;
          usrs->ntl_active_team_idx = usrs->ntl_team_count > 0 ? 0 : -1;
          init_edit_buffers(usrs);
          save_user_settings(usrs);
        }
      }
      igPopStyleColor(3);

      igSameLine(0, 8.0f);

      // no team button (orange)
      igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.80f, 0.35f, 0.08f, 0.80f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.90f, 0.45f, 0.15f, 1.0f});
      igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.70f, 0.30f, 0.05f, 1.0f});
      if (igButton("no team", (ImVec2){btn_w, 26})) {
        usrs->ntl_active_team_idx = -1;
        edit_team_id[0] = '\0';
        edit_auth_key[0] = '\0';
        edit_save_as[0] = '\0';
        save_user_settings(usrs);
      }
      igPopStyleColor(3);

      igPopStyleVar(1);

      END_CARD();
      igDummy((ImVec2){0, 8});
    }

    igEndTable();
  }
  igPopStyleVar(1); // CellPadding

  igEndChild(); // settings_scroll_pane

  // ─── Footer buttons ───
  {
    igSeparator();
    igDummy((ImVec2){0, 4});

    // Close (left)
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.18f, 0.20f, 0.25f, 0.50f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.25f, 0.28f, 0.35f, 0.70f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.15f, 0.16f, 0.20f, 0.90f});
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
    if (igButton("Close Window", (ImVec2){140, 32})) {
      close_settings(gdata);
    }
    igPopStyleVar(1);
    igPopStyleColor(3);

    // Save & Apply (right)
    igSameLine(w - 20 - 140, 0);
    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.06f, 0.73f, 0.51f, 0.80f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.06f, 0.73f, 0.51f, 1.0f});
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.04f, 0.55f, 0.38f, 1.0f});
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
    if (igButton("Save & Apply", (ImVec2){140, 32})) {
      // Auto-commit typed NTL values
      if (edit_save_as[0] != '\0' && edit_team_id[0] != '\0') {
        int found_idx = -1;
        for (int i = 0; i < usrs->ntl_team_count; i++) {
          if (strcmp(usrs->ntl_teams[i].name, edit_save_as) == 0) {
            found_idx = i;
            break;
          }
        }
        if (found_idx != -1) {
          strcpy(usrs->ntl_teams[found_idx].team_id, edit_team_id);
          strcpy(usrs->ntl_teams[found_idx].auth_key, edit_auth_key);
          usrs->ntl_active_team_idx = found_idx;
        } else if (usrs->ntl_team_count < 12) {
          int new_idx = usrs->ntl_team_count;
          strcpy(usrs->ntl_teams[new_idx].name, edit_save_as);
          strcpy(usrs->ntl_teams[new_idx].team_id, edit_team_id);
          strcpy(usrs->ntl_teams[new_idx].auth_key, edit_auth_key);
          usrs->ntl_active_team_idx = new_idx;
          usrs->ntl_team_count++;
        }
      }
      save_user_settings(usrs);
      close_settings(gdata);
    }
    igPopStyleVar(1);
    igPopStyleColor(3);
  }

  igEnd();
  igPopStyleVar(2);
  igPopStyleColor(2);
  igPopFont();
}

void ui_settings_destroy(tenv* env) {}
