#include "ui_overlay.h"

#include "../user.h"
#include "../ui/chat.h"
#include "../network/ntl_client.h"

static const char* clean_ntl_nickname(const char* raw_nick) {
  if (!raw_nick) return "";
  if (strlen(raw_nick) >= 8) {
    bool is_hex = true;
    for (int i = 0; i < 8; i++) {
      char c = raw_nick[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        is_hex = false;
        break;
      }
    }
    if (is_hex) {
      return raw_nick + 8;
    }
  }
  return raw_nick;
}


void ui_overlay(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  game_data* gdata = &usr->gdata;
  user_settings* usrs = &usr->usrs;

  float mww2 = ctx->size[0] / 2.0f;
  float mhh2 = ctx->size[1] / 2.0f;

  int snakes_len = tdarray_length(gdata->data.snakes);
  if (snakes_len) {
    snake* me = gdata->data.snakes + (snakes_len - 1);

    if (gdata->data.snake_id == me->id) {
      float a = me->alive_amt * (1 - me->dead_amt);
      int sct = me->sct + me->rsc;
      float hx = me->xx + me->fx;
      float hy = me->yy + me->fy;
      gdata->data.score = (int)floorf((gdata->data.fpsls[sct] +
                                       me->fam / gdata->data.fmlts[sct] - 1) *
                                          15 -
                                      5) /
                          1;

      if (usrs->hotkeys[HOTKEY_ASSIST].active) {
        ImDrawList_AddLine(
            igGetWindowDrawList(),
            (ImVec2){mww2 + (hx - gdata->data.view_xx) * gdata->data.gsc,
                     mhh2 + (hy - gdata->data.view_yy) * gdata->data.gsc},
            (ImVec2){env->ms->pos[0], env->ms->pos[1]},
            igColorConvertFloat4ToU32(
                (ImVec4){usrs->laser_color[0], usrs->laser_color[1],
                         usrs->laser_color[2], usrs->laser_color[3] * a}),
            usrs->laser_thickness);
      }
    }
  }

  usr->r->global.minimap_opacity = 0;
  if (usrs->hotkeys[HOTKEY_HUD].active) {
    ImGuiStyle* style = igGetStyle();
    float frame_height = igGetFrameHeight();

    igPushFont(usr->imgui_data.mono_font[usrs->stats_font_size],
               usr->imgui_data.mono_font[usrs->stats_font_size]->LegacySize);
    float line_height = igGetCursorPosY();
    ImVec2 icon_sz;
    igCalcTextSize(&icon_sz, "\ue971", NULL, false, -1);
    ImVec2 char_sz;
    igCalcTextSize(&char_sz, "-", NULL, false, -1);
    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue971");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.5}, usrs->nickname);
    line_height = igGetCursorPosY() - line_height;

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ueaec");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.5}, usrs->ipv4);

    float ping_norm =
        (gdata->data.ping_follow - GOOD_PING) / (BAD_PING - GOOD_PING);
    float lag_norm = (gdata->data.lag_mult - 0.2f) / (1 - 0.2f);
    vec3 ping_col;
    glm_vec3_lerp((vec3){0.5f, 1, 0.5f}, (vec3){1, 0.5f, 0.5f}, ping_norm,
                  ping_col);
    vec3 ic_col;
    glm_vec3_lerp((vec3){1, 0.5f, 0.5f}, (vec3){1, 1, 1}, lag_norm, ic_col);

    igTextColored(
        (ImVec4){ic_col[0], ic_col[1], ic_col[2], glm_lerp(0.8, 0.3, lag_norm)},
        "\ue91b");
    igSameLine(0, -1);
    igTextColored(
        (ImVec4){ping_col[0], ping_col[1], ping_col[2], 0.6 * lag_norm},
        "%d ms", gdata->data.ping);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue99c");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.5}, "%d FPS", gdata->data.fps);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue952");
    igSameLine(0, -1);

    int tot_sec = (int)gdata->data.play_etm;
    int hours = tot_sec / 3600;
    int minutes = (tot_sec % 3600) / 60;
    int seconds = tot_sec % 60;

    igTextColored((ImVec4){1, 1, 1, 0.7}, "%02d:%02d:%02d", hours, minutes,
                  seconds);
    igText("");



    float px = (((gdata->data.view_xx - gdata->data.grd) * 2) /
                ((gdata->data.flux_grd) * 2));
    float py = (((gdata->data.view_yy - gdata->data.grd) * 2) /
                ((gdata->data.flux_grd) * 2));
    int pang = (int)roundf(glm_deg(atan2f(-py, px)));
    if (pang < 0) pang += 360;
    int dst = (int)roundf(sqrtf(px * px + py * py) * 100.0f);

    igSetCursorPosY(ctx->size[1] - (line_height * 3) - style->WindowPadding.y);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ueaeb");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.7}, "%d", gdata->data.kills);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue9d9");
    igSameLine(0, -1);
    // igPushFont(
    //     usr->imgui_data.mono_font_bold[usrs->stats_font_size],
    //     usr->imgui_data.mono_font_bold[usrs->stats_font_size]->LegacySize);
    igTextColored((ImVec4){1, 1, 1, 0.7}, "%d", gdata->data.rank);
    // igPopFont();
    igSameLine(0, 0);
    igTextColored((ImVec4){1, 1, 1, 0.5}, " / %d", gdata->data.slither_count);

    igTextColored((ImVec4){1, 1, 1, 0.3}, "\ue99e");
    igSameLine(0, -1);
    igPushFont(
        usr->imgui_data.mono_font_bold[usrs->stats_font_size],
        usr->imgui_data.mono_font_bold[usrs->stats_font_size]->LegacySize);
    igTextColored((ImVec4){1, 1, 1, 0.7}, "%d", gdata->data.score);
    igPopFont();

    igPopFont();

    if (gdata->data.gotlb) {
      igPushFont(usr->imgui_data.mono_font[usrs->lb_font_size],
                 usr->imgui_data.mono_font[usrs->lb_font_size]->LegacySize);
      ImVec2 psize;
      igCalcTextSize(&psize, "10.", NULL, false, -1);

      ImVec2 nksize;
      char tmp[MAX_NICKNAME_LEN + 1] = {0};
      memset(tmp, (int)'a', MAX_NICKNAME_LEN);
      igCalcTextSize(&nksize, tmp, NULL, false, -1);
      nksize.x *= 1.25f;

      ImVec2 scsize;
      igCalcTextSize(&scsize, "999999", NULL, false, -1);
      igPopFont();

      float tb_width =
          psize.x + nksize.x + scsize.x + (style->CellPadding.x * 2 * 3);
      igSetCursorPosX(ctx->size[0] - tb_width - style->WindowPadding.x);
      igSetCursorPosY(style->WindowPadding.y);

      if (igBeginTable("leaderboard_table", 3, ImGuiTableFlags_NoHostExtendX,
                       (ImVec2){}, 0)) {
        igTableSetupColumn("##position", ImGuiTableColumnFlags_WidthFixed,
                           psize.x, 0);
        igTableSetupColumn("##nickname", ImGuiTableColumnFlags_WidthFixed,
                           nksize.x, 0);
        igTableSetupColumn("##score", ImGuiTableColumnFlags_WidthFixed,
                           scsize.x, 0);

        for (int row = 0; row < NUM_LEADERBOARD_ENTRIES; row++) {
          bool is_my_snake = gdata->data.lb_pos == (row + 1);
          vec3s* scolor = gdata->cg_colors + gdata->data.lb.entries[row].cv;
          vec3 tcolor;
          glm_vec3_lerp((float*)scolor, (vec3){1, 1, 1}, 0.4f, tcolor);
          ImVec4 itcolor = {tcolor[0], tcolor[1], tcolor[2], 1};
          if (is_my_snake) {
            igPushFont(
                usr->imgui_data.mono_font_bold[usrs->lb_font_size],
                usr->imgui_data.mono_font_bold[usrs->lb_font_size]->LegacySize);
          } else {
            igPushFont(
                usr->imgui_data.mono_font[usrs->lb_font_size],
                usr->imgui_data.mono_font[usrs->lb_font_size]->LegacySize);
            itcolor.w = 0.6f;  // .7f * (.3f + .7f * (1 - (1 + row) / 10.0f));
          }

          igTableNextRow(ImGuiTableRowFlags_None, 0);
          igTableSetColumnIndex(0);
          igTextColored((ImVec4){1, 1, 1, itcolor.w}, "%2d.", row + 1);
          igTableSetColumnIndex(1);
          igTextColored(itcolor, "%s", gdata->data.lb.entries[row].nickname);
          igTableSetColumnIndex(2);
          igTextColored(itcolor, "%d", gdata->data.lb.entries[row].score);

          igPopFont();
        }
        igEndTable();
      }
    }

    usr->r->global.minimap_circ[2] = usrs->minimap_size;
    usr->r->global.minimap_circ[0] =
        ctx->size[0] - usr->r->global.minimap_circ[2] - style->WindowPadding.x;
    usr->r->global.minimap_circ[1] = ctx->size[1] -
                                     usr->r->global.minimap_circ[2] -
                                     style->WindowPadding.y - line_height;
    usr->r->global.minimap_opacity = 1;

    // Draw team player dots on the minimap
    float R_mm = usrs->minimap_size * 0.5f;
    float mx_center = usr->r->global.minimap_circ[0] + R_mm;
    float my_center = usr->r->global.minimap_circ[1] + R_mm;
    int ntl_count = 0;
    ntl_player* ntl_players = ntl_get_players(&ntl_count);
    if (ntl_count > 0 && gdata->data.grd > 0) {
      ImDrawList* draw_list = igGetWindowDrawList();
      for (int i = 0; i < ntl_count; i++) {
        if (strcmp(ntl_players[i].nickname, "00000000") == 0) continue;
        if (strcmp(ntl_players[i].server, usrs->ipv4) == 0) {
          float tx = atof(ntl_players[i].valx);
          float ty = atof(ntl_players[i].valy);
          if (tx > 0 && ty > 0) {
            float rx = (tx - gdata->data.grd) / gdata->data.grd;
            float ry = (ty - gdata->data.grd) / gdata->data.grd;
            
            float dist = sqrtf(rx * rx + ry * ry);
            if (dist > 1.0f) {
              rx /= dist;
              ry /= dist;
            }
            
            float sx = mx_center + rx * R_mm;
            float sy = my_center + ry * R_mm;
            
            ImU32 dot_color = igColorConvertFloat4ToU32((ImVec4){0.0f, 1.0f, 0.5f, 1.0f});
            ImU32 border_color = igColorConvertFloat4ToU32((ImVec4){0.0f, 0.0f, 0.0f, 0.8f});
            
            ImDrawList_AddCircleFilled(draw_list, (ImVec2){sx, sy}, 4.0f, dot_color, 12);
            ImDrawList_AddCircle(draw_list, (ImVec2){sx, sy}, 4.0f, border_color, 12, 1.0f);
            
            char clean_name[16];
            strncpy(clean_name, clean_ntl_nickname(ntl_players[i].nickname), 15);
            clean_name[15] = '\0';
            
            igPushFont(usr->imgui_data.mono_font[FONT_SIZE_TINY],
                       usr->imgui_data.mono_font[FONT_SIZE_TINY]->LegacySize);
            ImVec2 txtsz;
            igCalcTextSize(&txtsz, clean_name, NULL, false, -1);
            
            ImDrawList_AddText_Vec2(draw_list, (ImVec2){sx - txtsz.x * 0.5f, sy - txtsz.y - 4}, 
                                    igColorConvertFloat4ToU32((ImVec4){1.0f, 1.0f, 1.0f, 0.8f}), 
                                    clean_name, NULL);
            igPopFont();
          }
        }
      }
    }

    igPushFont(usr->imgui_data.mono_font[usrs->stats_font_size],
               usr->imgui_data.mono_font[usrs->stats_font_size]->LegacySize);
    ImVec2 lctxtsz;
    igCalcTextSize(&lctxtsz, "--360° 100%", NULL, false, -1);

    igSetCursorPosX(
        ctx->size[0] -
        (usr->r->global.minimap_circ[2] * 0.5 + style->WindowPadding.x) -
        lctxtsz.x * 0.5f);
    igSetCursorPosY(ctx->size[1] - style->WindowPadding.y - line_height);

    igTextColored((ImVec4){1, 1, 1, 0.3f}, "\ue947");
    igSameLine(0, -1);
    igTextColored((ImVec4){1, 1, 1, 0.7f}, "%d° %d%%", pang, dst);
    igPopFont();
  }
}

void ui_online_players_hud(tenv* env) {
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;
  if (!usrs->show_online_players_hud) return;

  static float last_scr_w = 0;
  static float last_scr_h = 0;
  static float ratio_x = -1;
  static float ratio_y = -1;

  float scr_w = env->ctx->size[0];
  float scr_h = env->ctx->size[1];

  if (last_scr_w > 0 && last_scr_h > 0 && (scr_w != last_scr_w || scr_h != last_scr_h)) {
    if (ratio_x >= 0 && ratio_y >= 0) {
      ImVec2 new_pos = { ratio_x * scr_w, ratio_y * scr_h };
      igSetWindowPos_Str("Online Players##ntl_hud", new_pos, ImGuiCond_Always);
    }
  }

  bool settings_open = gdata->show_settings_overlay || gdata->curr_screen == SETTINGS;

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
  if (!settings_open) {
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;
  }

  // Set default window pos/size on first use
  igSetNextWindowPos((ImVec2){20.0f, 150.0f}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
  igSetNextWindowSize((ImVec2){250.0f, 150.0f}, ImGuiCond_FirstUseEver);
  igSetNextWindowBgAlpha(0.6f);

  if (igBegin("Online Players##ntl_hud", &usrs->show_online_players_hud, flags)) {
    igPushFont(usr->imgui_data.mono_font[FONT_SIZE_TINY],
               usr->imgui_data.mono_font[FONT_SIZE_TINY]->LegacySize);

    int team_count = 0;
    ntl_player* players = ntl_get_players(&team_count);

    igTextColored((ImVec4){1.0f, 1.0f, 1.0f, 0.8f}, "Online players (key owner, nick, srv, ver):");
    
    for (int i = 0; i < team_count; i++) {
      if (strcmp(players[i].nickname, "00000000") == 0) continue;
      // Key owner in red/orange:
      igTextColored((ImVec4){0.85f, 0.2f, 0.2f, 1.0f}, "%s", players[i].tlm[0] ? players[i].tlm : "unknown");
      igSameLine(0, 4);
      // Cleaned nickname in cyan:
      igTextColored((ImVec4){0.2f, 0.8f, 0.8f, 1.0f}, "%s", clean_ntl_nickname(players[i].nickname));
      igSameLine(0, 4);
      // Server in cyan/gray:
      igTextColored((ImVec4){0.2f, 0.8f, 0.8f, 0.8f}, "%s", players[i].server);
      igSameLine(0, 4);
      // Version in cyan/gray:
      igTextColored((ImVec4){0.2f, 0.8f, 0.8f, 0.8f}, "v%s", players[i].ver);
    }
    
    if (team_count == 0) {
      igTextColored((ImVec4){0.6f, 0.6f, 0.6f, 1.0f}, "No team players online");
    }

    ImVec2 pos;
    igGetWindowPos(&pos);
    ratio_x = pos.x / scr_w;
    ratio_y = pos.y / scr_h;

    igPopFont();
  }
  igEnd();
  last_scr_w = scr_w;
  last_scr_h = scr_h;
}

void ui_player_details_hud(tenv* env) {
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;
  if (!usrs->show_player_details_hud) return;

  static float last_scr_w = 0;
  static float last_scr_h = 0;
  static float ratio_x = -1;
  static float ratio_y = -1;

  float scr_w = env->ctx->size[0];
  float scr_h = env->ctx->size[1];

  if (last_scr_w > 0 && last_scr_h > 0 && (scr_w != last_scr_w || scr_h != last_scr_h)) {
    if (ratio_x >= 0 && ratio_y >= 0) {
      ImVec2 new_pos = { ratio_x * scr_w, ratio_y * scr_h };
      igSetWindowPos_Str("Players List##ntl_hud", new_pos, ImGuiCond_Always);
    }
  }

  bool settings_open = gdata->show_settings_overlay || gdata->curr_screen == SETTINGS;

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
  if (!settings_open) {
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;
  }

  // Set default window pos/size on first use
  igSetNextWindowPos((ImVec2){env->ctx->size[0] - 320.0f, 150.0f}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
  igSetNextWindowSize((ImVec2){300.0f, 200.0f}, ImGuiCond_FirstUseEver);
  igSetNextWindowBgAlpha(0.6f);

  if (igBegin("Players List##ntl_hud", &usrs->show_player_details_hud, flags)) {
    igPushFont(usr->imgui_data.mono_font[FONT_SIZE_TINY],
               usr->imgui_data.mono_font[FONT_SIZE_TINY]->LegacySize);

    int team_count = 0;
    ntl_player* players = ntl_get_players(&team_count);

    igText("Players list:");
    igText("------------------------------");

    for (int i = 0; i < team_count; i++) {
      if (strcmp(players[i].nickname, "00000000") == 0) continue;
      // Determine if local player
      bool is_me = (strcmp(clean_ntl_nickname(players[i].nickname), usrs->nickname) == 0) || 
                   (strlen(players[i].nickname) >= 8 && strncmp(players[i].nickname, usrs->ntl_user_id, 8) == 0);
      
      ImVec4 name_col = is_me ? (ImVec4){0.2f, 0.8f, 0.8f, 1.0f} : (ImVec4){0.85f, 0.2f, 0.2f, 1.0f};
      ImVec4 stats_col = is_me ? (ImVec4){0.2f, 0.8f, 0.8f, 1.0f} : (ImVec4){0.7f, 0.7f, 0.7f, 1.0f};

      // Diamond symbol ◆ or crown or shield based on status
      const char* sym = "◆";
      if (players[i].is_bot) sym = "[Bot]";
      else if (players[i].is_sos) sym = "🆘";
      
      igTextColored(name_col, "%s %s : : %s", sym, players[i].tlm[0] ? players[i].tlm : "unknown", players[i].score);
      
      // Stats line:
      igTextColored(stats_col, "    %s", players[i].dt);
      
      // Server line with "Play" button
      bool on_menu = (strcmp(players[i].server, "_GAME_MENU_") == 0);
      if (on_menu) {
        igTextColored(stats_col, "    _GAME_MENU_");
      } else {
        igIndent(16.0f);
        
        // Draw the red "Play" button
        igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.75f, 0.15f, 0.15f, 1.0f});
        igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.90f, 0.20f, 0.20f, 1.0f});
        igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.60f, 0.10f, 0.10f, 1.0f});
        igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 4.0f);
        igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){6, 1});
        
        char btn_id[64];
        sprintf(btn_id, "Play##play_%d", i);
        
        if (igButton(btn_id, (ImVec2){34, 16})) {
          strcpy(usrs->ipv4, players[i].server);
          save_user_settings(usrs);
          
          if (gdata->connection) {
            gdata->connection->is_closing = true;
          }
          gdata->restart_req = true;
        }
        
        igPopStyleVar(2);
        igPopStyleColor(3);
        
        igSameLine(0, 4);
        igTextColored(stats_col, "%s", players[i].server);
        igUnindent(16.0f);
      }
    }

    if (team_count == 0) {
      igTextColored((ImVec4){0.6f, 0.6f, 0.6f, 1.0f}, "No team players online");
    }

    ImVec2 pos;
    igGetWindowPos(&pos);
    ratio_x = pos.x / scr_w;
    ratio_y = pos.y / scr_h;

    igPopFont();
  }
  igEnd();
  last_scr_w = scr_w;
  last_scr_h = scr_h;
}