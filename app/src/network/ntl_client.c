#include "ntl_client.h"
#include "../user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>

static HANDLE ntl_thread = NULL;
static bool ntl_running = false;
static ntl_player ntl_players_list[MAX_NTL_PLAYERS];
static int ntl_player_count = 0;
static CRITICAL_SECTION ntl_lock;
static bool lock_initialized = false;

// NTL Chat structures
#define MAX_PENDING_CHAT_MESSAGES 16

typedef struct pending_chat {
  char sender[32];
  char text[128];
} pending_chat;

static pending_chat pending_chat_queue[MAX_PENDING_CHAT_MESSAGES];
static int pending_chat_count = 0;

#define MAX_OUTGOING_CHAT_MESSAGES 16
static char outgoing_chat_queue[MAX_OUTGOING_CHAT_MESSAGES][128];
static int outgoing_chat_count = 0;

typedef struct ntl_msg_cache {
  char user_id[9];
  char last_msg[128];
} ntl_msg_cache;

static ntl_msg_cache msg_caches[MAX_NTL_PLAYERS];
static int msg_cache_count = 0;

static const char* clean_nick(const char* raw_nick) {
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
    if (is_hex) return raw_nick + 8;
  }
  return raw_nick;
}

static void url_decode(const char* src, char* dest, int max_len) {
  int d_idx = 0;
  for (int i = 0; src[i] && d_idx < max_len - 1; ) {
    if (src[i] == '%') {
      if (src[i+1] && src[i+2]) {
        char hex[3] = { src[i+1], src[i+2], '\0' };
        dest[d_idx++] = (char)strtol(hex, NULL, 16);
        i += 3;
      } else {
        dest[d_idx++] = src[i++];
      }
    } else if (src[i] == '+') {
      dest[d_idx++] = ' ';
      i++;
    } else {
      dest[d_idx++] = src[i++];
    }
  }
  dest[d_idx] = '\0';
}

static void decode_html_entities(const char* src, char* dest, int max_len) {
  int d_idx = 0;
  for (int i = 0; src[i] && d_idx < max_len - 1; ) {
    if (src[i] == '&') {
      if (strncmp(src + i, "&nbsp;", 6) == 0) {
        dest[d_idx++] = ' ';
        i += 6;
      } else if (strncmp(src + i, "&amp;", 5) == 0) {
        dest[d_idx++] = '&';
        i += 5;
      } else if (strncmp(src + i, "&lt;", 4) == 0) {
        dest[d_idx++] = '<';
        i += 4;
      } else if (strncmp(src + i, "&gt;", 4) == 0) {
        dest[d_idx++] = '>';
        i += 4;
      } else if (strncmp(src + i, "&quot;", 6) == 0) {
        dest[d_idx++] = '"';
        i += 6;
      } else if (strncmp(src + i, "&#39;", 5) == 0) {
        dest[d_idx++] = '\'';
        i += 5;
      } else if (strncmp(src + i, "&#039;", 6) == 0) {
        dest[d_idx++] = '\'';
        i += 6;
      } else {
        dest[d_idx++] = src[i++];
      }
    } else {
      dest[d_idx++] = src[i++];
    }
  }
  dest[d_idx] = '\0';
}

void ntl_client_send_msg(const char* text) {
  if (!lock_initialized) return;
  EnterCriticalSection(&ntl_lock);
  if (outgoing_chat_count < MAX_OUTGOING_CHAT_MESSAGES) {
    strncpy(outgoing_chat_queue[outgoing_chat_count], text, 127);
    outgoing_chat_queue[outgoing_chat_count][127] = '\0';
    outgoing_chat_count++;
  }
  LeaveCriticalSection(&ntl_lock);
}

bool ntl_client_poll_chat(char* sender, char* text) {
  if (!lock_initialized) return false;
  bool has_msg = false;
  EnterCriticalSection(&ntl_lock);
  if (pending_chat_count > 0) {
    strcpy(sender, pending_chat_queue[0].sender);
    strcpy(text, pending_chat_queue[0].text);
    for (int i = 1; i < pending_chat_count; i++) {
      pending_chat_queue[i - 1] = pending_chat_queue[i];
    }
    pending_chat_count--;
    has_msg = true;
  }
  LeaveCriticalSection(&ntl_lock);
  return has_msg;
}


static void extract_json_value(const char* obj, const char* key, char* dest, int max_len) {
  dest[0] = '\0';
  char search_key[64];
  sprintf(search_key, "\"%s\":", key);
  const char* p = strstr(obj, search_key);
  if (!p) return;
  p += strlen(search_key);
  while (*p == ' ' || *p == '\t') p++;
  if (*p == '"') {
    p++;
    int i = 0;
    while (*p && *p != '"' && i < max_len) {
      dest[i++] = *p++;
    }
    dest[i] = '\0';
  } else {
    int i = 0;
    while (*p && *p != ',' && *p != '}' && *p != ' ' && *p != '\t' && i < max_len) {
      dest[i++] = *p++;
    }
    dest[i] = '\0';
  }
}

static void url_encode(const char* src, char* dest, int max_len) {
  int d_idx = 0;
  for (int i = 0; src[i] && d_idx < max_len - 3; i++) {
    char c = src[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      dest[d_idx++] = c;
    } else {
      sprintf(dest + d_idx, "%%%02X", (unsigned char)c);
      d_idx += 3;
    }
  }
  dest[d_idx] = '\0';
}

DWORD WINAPI ntl_sync_thread(LPVOID param) {
  tenv* env = (tenv*)param;
  tuser_data* usr = env->usr;
  user_settings* usrs = &usr->usrs;
  game_data* gdata = &usr->gdata;

  HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

  hSession = WinHttpOpen(L"Vlither NTL/1.0", 
                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                         WINHTTP_NO_PROXY_NAME, 
                         WINHTTP_NO_PROXY_BYPASS, 0);

  if (!hSession) return 0;

  printf("[NTL sync] background thread started.\n");

  while (ntl_running) {
    char active_tid[33] = {0};
    char active_auth[65] = {0};
    if (usrs->ntl_active_team_idx >= 0 && usrs->ntl_active_team_idx < usrs->ntl_team_count) {
      strcpy(active_tid, usrs->ntl_teams[usrs->ntl_active_team_idx].team_id);
      strcpy(active_auth, usrs->ntl_teams[usrs->ntl_active_team_idx].auth_key);
    }

    if (active_auth[0] == '\0' || active_tid[0] == '\0') {
      printf("[NTL sync] Warning: No active Team ID or Auth Key configured/selected. Configure in Settings.\n");
      Sleep(3000);
      continue;
    }

    // Generate random hex NTL ID if empty
    if (usrs->ntl_user_id[0] == '\0') {
      for (int i = 0; i < 8; i++) {
        sprintf(usrs->ntl_user_id + i, "%x", rand() % 16);
      }
      usrs->ntl_user_id[8] = '\0';
      printf("[NTL sync] Generated new random client ID: %s\n", usrs->ntl_user_id);
      save_user_settings(usrs);
    }

    // NTL Nickname format: {8_char_user_id}{nickname}
    char nick_with_id[128];
    sprintf(nick_with_id, "%s%s", usrs->ntl_user_id, usrs->nickname);

    char enc_nick[256] = {0};
    char enc_srv[128] = {0};
    char enc_dt[256] = {0};
    
    url_encode(nick_with_id, enc_nick, sizeof(enc_nick));

    // Dynamic Server representation
    if (gdata->conn == CONNECTED && gdata->curr_screen == PLAYING) {
      url_encode(usrs->ipv4, enc_srv, sizeof(enc_srv));
    } else {
      strcpy(enc_srv, "_GAME_MENU_");
    }

    // Dynamic stats text string
    char raw_dt[256];
    sprintf(raw_dt, "FPS: %d @ %d(%d) ms @ %d K/m", 
            gdata->data.fps, gdata->data.ping, gdata->data.ping, gdata->data.kills);
    url_encode(raw_dt, enc_dt, sizeof(enc_dt));

    float valx = 0;
    float valy = 0;
    int snakes_len = tdarray_length(gdata->data.snakes);
    if (snakes_len > 0) {
      snake* me = gdata->data.snakes + (snakes_len - 1);
      valx = me->xx + me->fx;
      valy = me->yy + me->fy;
    } else {
      valx = gdata->data.view_xx;
      valy = gdata->data.view_yy;
    }

    char raw_msg[512] = {0};
    EnterCriticalSection(&ntl_lock);
    if (outgoing_chat_count > 0) {
      int len = 0;
      for (int i = 0; i < outgoing_chat_count; i++) {
        if (i > 0) {
          if (len + 3 < sizeof(raw_msg)) {
            strcat(raw_msg, " | ");
            len += 3;
          }
        }
        if (len + strlen(outgoing_chat_queue[i]) < sizeof(raw_msg)) {
          strcat(raw_msg, outgoing_chat_queue[i]);
          len += strlen(outgoing_chat_queue[i]);
        }
      }
      outgoing_chat_count = 0;
    }
    LeaveCriticalSection(&ntl_lock);

    char enc_msg[1024] = {0};
    if (raw_msg[0] != '\0') {
      url_encode(raw_msg, enc_msg, sizeof(enc_msg));
    }

    int my_snake_id = (gdata->conn == CONNECTED && gdata->curr_screen == PLAYING) ? gdata->data.snake_id : -1;
    char query[2048];
    sprintf(query, "/slither/ntlplay-mt.php?auth=%s&tid=%s&nick=%s&score=%d&valx=%d&valy=%d&bot=%s&sos=false&food=false&srv=%s&msg=%s&dt=%s&i=%d&ver=9.19&di=1000",
            active_auth, active_tid, enc_nick, gdata->data.score, (int)valx, (int)valy,
            usrs->hotkeys[HOTKEY_BOT].active ? "true" : "false", enc_srv, enc_msg, enc_dt,
            my_snake_id);

    WCHAR wquery[1024];
    MultiByteToWideChar(CP_UTF8, 0, query, -1, wquery, 1024);

    printf("[NTL sync] Sending update for nick='%s', tid='%s', srv='%s'...\n", nick_with_id, active_tid, enc_srv);

    hConnect = WinHttpConnect(hSession, L"ntl-slither.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect) {
      hRequest = WinHttpOpenRequest(hConnect, L"GET", wquery, NULL, WINHTTP_NO_REFERER, 
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
      
      // Disable SSL validation if needed, NTL might use self-signed certificates
      DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | 
                      SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE | 
                      SECURITY_FLAG_IGNORE_CERT_CN_INVALID | 
                      SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
      WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    } else {
      printf("[NTL sync] WinHttpConnect failed (error: %lu)\n", GetLastError());
    }

    BOOL bResults = FALSE;
    if (hRequest) {
      bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
      if (!bResults) {
        printf("[NTL sync] WinHttpSendRequest failed (error: %lu)\n", GetLastError());
      }
    }

    if (bResults) {
      bResults = WinHttpReceiveResponse(hRequest, NULL);
      if (!bResults) {
        printf("[NTL sync] WinHttpReceiveResponse failed (error: %lu)\n", GetLastError());
      }
    }

    if (bResults) {
      DWORD dwSize = 0;
      DWORD dwDownloaded = 0;
      LPSTR pszOutBuffer = NULL;
      LPSTR totalBuffer = NULL;
      DWORD totalSize = 0;

      do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;

        pszOutBuffer = malloc(dwSize + 1);
        if (!pszOutBuffer) break;

        ZeroMemory(pszOutBuffer, dwSize + 1);
        if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
          totalBuffer = realloc(totalBuffer, totalSize + dwDownloaded + 1);
          memcpy(totalBuffer + totalSize, pszOutBuffer, dwDownloaded);
          totalSize += dwDownloaded;
          totalBuffer[totalSize] = '\0';
        }
        free(pszOutBuffer);
      } while (dwSize > 0);

      if (totalBuffer && totalSize > 0) {
        // Parse JSON array
        ntl_player temp_players[MAX_NTL_PLAYERS];
        int temp_count = 0;

        const char* p = totalBuffer;
        while ((p = strstr(p, "{")) && temp_count < MAX_NTL_PLAYERS) {
          const char* end = strstr(p, "}");
          if (!end) break;

          int obj_len = (int)(end - p) + 1;
          char* obj = malloc(obj_len + 1);
          memcpy(obj, p, obj_len);
          obj[obj_len] = '\0';

          ntl_player* pl = &temp_players[temp_count];
          extract_json_value(obj, "nick", pl->nickname, 31);
          extract_json_value(obj, "score", pl->score, 15);
          extract_json_value(obj, "srv", pl->server, 31);
          extract_json_value(obj, "valx", pl->valx, 15);
          extract_json_value(obj, "valy", pl->valy, 15);

          char bot_val[16] = {0};
          extract_json_value(obj, "bot", bot_val, 15);
          pl->is_bot = (strcmp(bot_val, "true") == 0);

          char sos_val[16] = {0};
          extract_json_value(obj, "sos", sos_val, 15);
          pl->is_sos = (strcmp(sos_val, "true") == 0);

          // Extract version, key owner, and details stats
          extract_json_value(obj, "ver", pl->ver, 15);
          extract_json_value(obj, "owner", pl->tlm, 31);
          extract_json_value(obj, "dt", pl->dt, 127);
          extract_json_value(obj, "sid", pl->sid, 15);

          // Parse msg and url decode it
          char pl_msg[128] = {0};
          extract_json_value(obj, "msg", pl_msg, 127);

          char decoded_msg[128] = {0};
          if (pl_msg[0] != '\0') {
            char raw_decoded[128] = {0};
            url_decode(pl_msg, raw_decoded, sizeof(raw_decoded));
            decode_html_entities(raw_decoded, decoded_msg, sizeof(decoded_msg));
          }

          if (strcmp(pl->nickname, nick_with_id) != 0) {
            char pl_id[9] = {0};
            if (strlen(pl->nickname) >= 8) {
              strncpy(pl_id, pl->nickname, 8);
              pl_id[8] = '\0';
            }

            if (pl_id[0] != '\0') {
              EnterCriticalSection(&ntl_lock);
              int cached_idx = -1;
              for (int c = 0; c < msg_cache_count; c++) {
                if (strcmp(msg_caches[c].user_id, pl_id) == 0) {
                  cached_idx = c;
                  break;
                }
              }
              if (decoded_msg[0] != '\0') {
                if (cached_idx != -1) {
                  if (strcmp(msg_caches[cached_idx].last_msg, decoded_msg) != 0) {
                    strcpy(msg_caches[cached_idx].last_msg, decoded_msg);
                    if (pending_chat_count < MAX_PENDING_CHAT_MESSAGES) {
                      strcpy(pending_chat_queue[pending_chat_count].sender, clean_nick(pl->nickname));
                      strcpy(pending_chat_queue[pending_chat_count].text, decoded_msg);
                      pending_chat_count++;
                    }
                  }
                } else if (msg_cache_count < MAX_NTL_PLAYERS) {
                  int new_cache_idx = msg_cache_count++;
                  strcpy(msg_caches[new_cache_idx].user_id, pl_id);
                  strcpy(msg_caches[new_cache_idx].last_msg, decoded_msg);
                  if (pending_chat_count < MAX_PENDING_CHAT_MESSAGES) {
                    strcpy(pending_chat_queue[pending_chat_count].sender, clean_nick(pl->nickname));
                    strcpy(pending_chat_queue[pending_chat_count].text, decoded_msg);
                    pending_chat_count++;
                  }
                }
              } else {
                if (cached_idx != -1) {
                  msg_caches[cached_idx].last_msg[0] = '\0';
                }
              }
              LeaveCriticalSection(&ntl_lock);
            }
            temp_count++;
          }

          free(obj);
          p = end + 1;
        }

        printf("[NTL sync] Sync successful! Parsed %d online team members.\n", temp_count);

        EnterCriticalSection(&ntl_lock);
        ntl_player_count = temp_count;
        memcpy(ntl_players_list, temp_players, sizeof(ntl_player) * temp_count);
        LeaveCriticalSection(&ntl_lock);

        free(totalBuffer);
      } else {
        printf("[NTL sync] Error: Empty response body received from server.\n");
      }
    }

    if (hRequest) { WinHttpCloseHandle(hRequest); hRequest = NULL; }
    if (hConnect) { WinHttpCloseHandle(hConnect); hConnect = NULL; }

    // Sleep for 3 seconds before next update
    for (int i = 0; i < 30 && ntl_running; i++) {
      Sleep(100);
    }
  }

  if (hSession) WinHttpCloseHandle(hSession);
  return 0;
}

void ntl_client_start(tenv* env) {
  if (ntl_running) return;

  if (!lock_initialized) {
    InitializeCriticalSection(&ntl_lock);
    lock_initialized = true;
  }

  ntl_running = true;
  ntl_player_count = 0;
  ntl_thread = CreateThread(NULL, 0, ntl_sync_thread, env, 0, NULL);
}

void ntl_client_stop(void) {
  if (!ntl_running) return;

  ntl_running = false;
  if (ntl_thread) {
    WaitForSingleObject(ntl_thread, 2000);
    CloseHandle(ntl_thread);
    ntl_thread = NULL;
  }
}

ntl_player* ntl_get_players(int* count) {
  if (!ntl_running || !lock_initialized) {
    *count = 0;
    return NULL;
  }
  EnterCriticalSection(&ntl_lock);
  *count = ntl_player_count;
  LeaveCriticalSection(&ntl_lock);
  return ntl_players_list;
}
#else
void ntl_client_start(tenv* env) {}
void ntl_client_stop(void) {}
ntl_player* ntl_get_players(int* count) {
  *count = 0;
  return NULL;
}
#endif
