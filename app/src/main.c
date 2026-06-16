#include "game/loop.h"
#include "game/ui_overlay.h"
#include "ui/skin_editor.h"
#include "ui/title_screen.h"
#include "ui/settings.h"
#include "ui/viewport.h"
#include "ui/chat.h"
#include "user.h"
#include "network/ntl_client.h"

#ifdef _WIN32
#include <windows.h>
#endif

void tinput(tenv* env) {
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;

  if (twindow_closed(env->wnd)) {
    env->config.running = false;
    save_user_settings(usrs);
  }
  if (tkeyboard_key_pressed(env->kb, GLFW_KEY_F11)) {
    twindow_toggle_fullscreen(env->wnd);
  }
  if (usr->gdata.curr_screen == PLAYING) {
    bool typing = chat_is_typing() || igIsAnyItemActive() || ui_settings_waiting_for_key();
    
    bool dot_pressed = false;
    bool esc_pressed = false;
    
    if (!typing) {
      dot_pressed = tkeyboard_key_pressed(env->kb, GLFW_KEY_PERIOD);
      esc_pressed = tkeyboard_key_pressed(env->kb, GLFW_KEY_ESCAPE);
    }

    static double last_dot_press = 0;
    double now = glfwGetTime();

    bool toggle_settings = false;
    if (esc_pressed) {
      toggle_settings = true;
    }
    if (dot_pressed) {
      double diff = now - last_dot_press;
      last_dot_press = now;
      if (diff < 0.35) {
        toggle_settings = true;
      }
    }

    if (toggle_settings) {
      usr->gdata.show_settings_overlay = !usr->gdata.show_settings_overlay;
      if (usr->gdata.show_settings_overlay) {
        usrs->hotkeys[HOTKEY_BOT].active = true;
        usr->gdata.settings_opened_from_menu = false;
      } else {
        if (usr->gdata.settings_opened_from_menu) {
          if (usr->gdata.connection) {
            usr->gdata.connection->is_closing = true;
          }
          usr->gdata.settings_opened_from_menu = false;
        }
      }
    }
  }
}

void tlaunch(tenv* env) {
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  srand(time(NULL));

  memset(usrs, 0, sizeof(user_settings));
  strcpy(usrs->ipv4, "15.204.212.200:444");
  strcpy(usrs->nickname, "");

  usrs->custom_skin = false;
  usrs->default_skin = rand() % 9;
  usrs->accessory = NO_ACCESSORY;

  read_user_settings(usrs);
  usrs->hotkeys[HOTKEY_BOT].active = false;

  env->config.vsync = usrs->vsync;
  env->config.fullscreen = false;
  env->config.title = "Slither.io (Unofficial)";
}

void tinit(tenv* env) {
  tuser_data* usr = env->usr;

  imgui_init(env);
  env->usr->r = renderer_create(env);
  ui_viewport_init(env);
  ui_title_screen_init(env);
  ui_skin_editor_init(env);
  ui_settings_init(env);
  ui_chat_init(env);
  game_data_init(env);
  ntl_client_start(env);
}

void tdestroy(tenv* env) {
  ntl_client_stop();
  game_data_destroy(env);
  ui_settings_destroy(env);
  ui_skin_editor_destroy(env);
  ui_title_screen_destroy(env);
  ui_chat_destroy(env);
  ui_viewport_destroy(env);
  renderer_destroy(env->usr->r, env->ctx);
  imgui_destroy();
}

void trender(tenv* env) {
  tuser_data* usr = env->usr;
  tcontext* ctx = env->ctx;
  game_data* gdata = &usr->gdata;

  imgui_prerender();
  // render begin
  ImGuiStyle* style = igGetStyle();
  ui_viewport(env);

  igSetNextWindowPos(igGetMainViewport()->Pos, ImGuiCond_None, (ImVec2){});
  igSetNextWindowSize(igGetMainViewport()->Size, ImGuiCond_None);
  ImGuiWindowFlags holder_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (usr->gdata.curr_screen == PLAYING && !usr->gdata.show_settings_overlay) {
    holder_flags |= ImGuiWindowFlags_NoInputs;
  }
  igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0);
  igBegin("##fullscreen_holder", NULL, holder_flags);
  igPopStyleVar(1);
  switch (usr->gdata.curr_screen) {
    case TITLE_SCREEN:
      ui_title_screen(env);
      break;
    case SKIN_EDITOR:
      ui_skin_editor(env);
      break;
    case PLAYING:
      game_loop(env);
      if (usr->gdata.show_settings_overlay) {
        ui_settings(env);
      }
      break;
    case SETTINGS:
      ui_settings(env);
      break;
  }
  igEnd();

  // Render NTL HUD windows on title screen, game screen, and settings screen
  if (usr->gdata.curr_screen == TITLE_SCREEN || usr->gdata.curr_screen == PLAYING || usr->gdata.curr_screen == SETTINGS) {
    ui_chat(env);
    ui_online_players_hud(env);
    ui_player_details_hud(env);
  }
  // render end

  igRender();
  if (tcontext_begin(ctx)) {
    renderer_render(usr->r, ctx, (vec4){0.086f, 0.109f, 0.133f, 1});
    tcontext_clear(ctx, (vec4){0, 0, 0, 1.0f});
    imgui_render(ctx->frames[ctx->current_frame].cmd);
    renderer_render_cursor(usr->r, ctx);
    tcontext_end(ctx);
  }
  renderer_clear_instances(usr->r);

  // Yield thread to prevent 100% CPU/GPU saturation and eliminate tab switching lag when settings are open
  if (usr->gdata.show_settings_overlay || usr->gdata.curr_screen == SETTINGS) {
    #ifdef _WIN32
    Sleep(8);
    #endif
  }
}

void tresize(tenv* env) { ui_viewport_resize(env); }

TDEF_ENTRY();