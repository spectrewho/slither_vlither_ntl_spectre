#ifndef SETTINGS_H
#define SETTINGS_H

#include <thermite.h>

void ui_settings_init(tenv* env);
void ui_settings(tenv* env);
void ui_settings_destroy(tenv* env);
bool ui_settings_waiting_for_key(void);

#endif