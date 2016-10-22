#pragma once
# include <pebble.h>

void stop_list_window_push(void (*param_menu_select_callback)());

void stop_list_window_set_data(char **stopList);