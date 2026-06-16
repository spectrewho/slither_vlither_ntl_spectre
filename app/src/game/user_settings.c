#include "user_settings.h"

#include <string.h>
#include <thermite.h>

void user_settings_default(user_settings* usr_settings) {
  usr_settings->ui_font_size = FONT_SIZE_SMALL;
  usr_settings->lb_font_size = FONT_SIZE_REGULAR;
  usr_settings->snake_names_font_size = FONT_SIZE_REGULAR;
  usr_settings->stats_font_size = FONT_SIZE_REGULAR;

  strcpy(usr_settings->version, SETTINGS_VERSION);

  usr_settings->bd_color[0] = 1;
  usr_settings->bd_color[1] = 0.25f;
  usr_settings->bd_color[2] = 0.25f;
  usr_settings->laser_color[0] = 0.5f;
  usr_settings->laser_color[1] = 1;
  usr_settings->laser_color[2] = 0.5f;
  usr_settings->laser_color[3] = 1;
  usr_settings->laser_thickness = 2;
  usr_settings->cursor_size = 48;
  usr_settings->minimap_size = 300;
  usr_settings->zoom_step = 0.1f;
  usr_settings->snake_scores = true;
  usr_settings->restart_rc = false;
  usr_settings->quit_mc = false;
  usr_settings->smooth_zoom = false;
  usr_settings->vsync = false;
  usr_settings->instant_restart = false;
  usr_settings->bot_radius_mult = 20;
  usr_settings->bot_follow_circle_score = 2000;

  // normal mode
  usr_settings->modes[0].food_flicker = true;
  usr_settings->modes[0].food_float = true;
  usr_settings->modes[0].uniform_food_color = false;
  usr_settings->modes[0].food_type = 0;
  usr_settings->modes[0].food_scale = 1;
  usr_settings->modes[0].food_color[0] = 1;
  usr_settings->modes[0].food_color[1] = 1;
  usr_settings->modes[0].food_color[2] = 1;
  usr_settings->modes[0].boost_type = 0;
  usr_settings->modes[0].qsm = 1;
  usr_settings->modes[0].bg_scale = 599 / 4096.0f;
  usr_settings->modes[0].boost_strength = 1;
  usr_settings->modes[0].show_crosshair = false;
  usr_settings->modes[0].show_boost = true;
  usr_settings->modes[0].show_shadows = true;
  usr_settings->modes[0].show_background = true;
  usr_settings->modes[0].show_accessories = true;
  usr_settings->modes[0].death_effect = true;
  usr_settings->modes[0].player_names_outline = false;
  usr_settings->modes[0].render_mode = 0;

  // assist mode
  usr_settings->modes[1].food_flicker = false;
  usr_settings->modes[1].food_float = false;
  usr_settings->modes[1].uniform_food_color = true;
  usr_settings->modes[1].food_type = 1;
  usr_settings->modes[1].food_scale = 1;
  usr_settings->modes[1].food_color[0] = 0.7f;
  usr_settings->modes[1].food_color[1] = 0.7f;
  usr_settings->modes[1].food_color[2] = 0.7f;
  usr_settings->modes[1].boost_type = 1;
  usr_settings->modes[1].qsm = 1;
  usr_settings->modes[1].bg_scale = 599 / 4096.0f;
  usr_settings->modes[1].boost_strength = 1;
  usr_settings->modes[1].show_crosshair = true;
  usr_settings->modes[1].show_boost = false;
  usr_settings->modes[1].show_shadows = true;
  usr_settings->modes[1].show_background = false;
  usr_settings->modes[1].show_accessories = false;
  usr_settings->modes[1].death_effect = false;
  usr_settings->modes[1].player_names_outline = true;
  usr_settings->modes[1].render_mode = 1;

  usr_settings->hotkeys[HOTKEY_HUD] = (hotkey){GLFW_KEY_H, true, 0, "HUD"};
  usr_settings->hotkeys[HOTKEY_SHOW_NAMES] =
      (hotkey){GLFW_KEY_1, true, 0, "Show names"};
  usr_settings->hotkeys[HOTKEY_BIG_FOOD] =
      (hotkey){GLFW_KEY_F, false, 0, "Big food"};
  usr_settings->hotkeys[HOTKEY_ASSIST] =
      (hotkey){GLFW_KEY_R, false, 1, "Assist"};
  usr_settings->hotkeys[HOTKEY_BOT] =
      (hotkey){GLFW_KEY_T, false, 0, "Bot"};
  usr_settings->hotkeys[HOTKEY_MENU] =
      (hotkey){GLFW_KEY_E, true, 0, "Hotkey menu"};
  usr_settings->hotkeys[HOTKEY_RESTART] =
      (hotkey){GLFW_KEY_0, false, 0, "Restart"};
  usr_settings->hotkeys[HOTKEY_QUIT] = (hotkey){GLFW_KEY_9, false, 0, "Quit"};

  // NTL default configurations:
  memset(usr_settings->ntl_teams, 0, sizeof(usr_settings->ntl_teams));
  usr_settings->ntl_team_count = 0;
  usr_settings->ntl_active_team_idx = -1;
  memset(usr_settings->ntl_user_id, 0, sizeof(usr_settings->ntl_user_id));
  usr_settings->show_chat_hud = true;
  usr_settings->show_online_players_hud = true;
  usr_settings->show_player_details_hud = true;
}

void write_default_settings(user_settings* usr_settings) {
  user_settings_default(usr_settings);

  FILE* f = fopen(USER_SETTINGS_FILE, "wb");
  if (f == NULL) {
    printf("Error creating settings file.");
    exit(-1);
  }

  fwrite(usr_settings, sizeof(user_settings), 1, f);
  fclose(f);
}

void read_user_settings(user_settings* usr_settings) {
  // Initialize to default first
  user_settings_default(usr_settings);

  FILE* f = fopen(USER_SETTINGS_FILE, "rb");
  if (f == NULL) {
    printf("[Settings] settings file not found, creating default settings.\n");
    write_default_settings(usr_settings);
    return;
  }

  // Get file size
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (file_size <= 0) {
    fclose(f);
    printf("[Settings] settings file empty, recreating defaults.\n");
    write_default_settings(usr_settings);
    return;
  }

  long to_read = file_size < (long)sizeof(user_settings) ? file_size : (long)sizeof(user_settings);
  
  user_settings temp;
  user_settings_default(&temp);

  size_t read_bytes = fread(&temp, 1, to_read, f);
  fclose(f);

  if (read_bytes < 4 || strncmp(temp.version, SETTINGS_VERSION, 3) != 0) {
    printf("[Settings] settings file version mismatch or invalid (read: %zu, ver: %s), recreating defaults.\n", read_bytes, temp.version);
    write_default_settings(usr_settings);
    return;
  }

  // Copy read bytes over default-initialized settings struct
  memcpy(usr_settings, &temp, to_read);
  printf("[Settings] Successfully loaded user settings (size: %ld, version: %s, active team idx: %d, teams: %d).\n", 
         file_size, usr_settings->version, usr_settings->ntl_active_team_idx, usr_settings->ntl_team_count);
}

void save_user_settings(user_settings* usr_settings) {
  FILE* f = fopen(USER_SETTINGS_FILE, "wb");
  if (f == NULL) {
    printf("[Settings] Error saving settings file.\n");
    exit(-1);
  }

  fwrite(usr_settings, sizeof(user_settings), 1, f);
  fclose(f);
  printf("[Settings] Successfully saved settings to %s\n", USER_SETTINGS_FILE);
}
