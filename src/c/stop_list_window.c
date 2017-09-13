#include <pebble.h>
#include "data.h"
#include "stop_list_window.h"

//the window in which the list of nearby bus stops will be displayed
static Window *s_window;

static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;

static char **stopList;

static void (*menu_select_callback)();

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  if(numStops){
        return numStops;
  }
    else{
      return 0;
    }  
}

static int16_t menu_layer_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
    return graphics_text_layout_get_content_size(stopList[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), PBL_IF_ROUND_ELSE(GRect(0,0,176,180), GRect(0, 0, 140, 100)), GTextOverflowModeWordWrap, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft)).h+8;
  
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    //draw this into the bounds of the provided layer, adding padding to the left side
    GRect bounds = layer_get_bounds(cell_layer);
    bounds.origin.x +=2;
    bounds.size.w -=4;
    graphics_draw_text(ctx, stopList[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), bounds, GTextOverflowModeWordWrap, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), NULL);

}




static void window_load(Window *window) {
  #ifdef PBL_COLOR
  window_set_background_color(window, GColorBlack);
  #endif
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  //resize bounds to leave room for status bar
  bounds.origin.y += STATUS_BAR_LAYER_HEIGHT;
  bounds.size.h -= STATUS_BAR_LAYER_HEIGHT;
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .get_cell_height = menu_layer_get_cell_height_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  //set menu layer colours for colour watches
#if defined(PBL_COLOR)
  menu_layer_set_highlight_colors(s_menu_layer, GColorRed, GColorWhite);
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
#endif
  
  //add status bar to the window layer
  // Set up the status bar last to ensure it is on top of other Layers
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
  
  //add menu layer to the window
  layer_add_child(window_get_root_layer(window), menu_layer_get_layer(s_menu_layer));
  //refresh menu layer data
  menu_layer_reload_data(s_menu_layer);
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, 0), MenuRowAlignCenter, true);

}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  status_bar_layer_destroy(s_status_bar);
  window_destroy(s_window);
}

void stop_list_window_push(void (*param_menu_select_callback)()) {
  if(!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  menu_select_callback = param_menu_select_callback;
  window_stack_push(s_window, true);
}

void stop_list_window_set_data(char **param_stopList){
  stopList = param_stopList;

  if(s_menu_layer){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Refreshing Stops Menu Layer");
    menu_layer_reload_data(s_menu_layer);
    layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
    menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, 0), MenuRowAlignCenter, true);
  }
}
