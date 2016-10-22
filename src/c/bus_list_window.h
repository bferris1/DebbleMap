#pragma once
# include <pebble.h>
# include "data.h"

void bus_list_window_push(void (*param_menu_select_callback)());

void bus_list_window_set_data(char **param_routeList);  

void bus_list_window_remove();