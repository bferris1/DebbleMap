#include <pebble.h>
#include "data.h"
#include "bus_list_window.h"

static Window *s_window;

static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static TextLayer *bus_loading_text_layer;

static char **routeList;


//pointer to function to be called when an item is selected
static void (*menu_select_callback)();

static uint16_t detail_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  if(numETAs){
    return numETAs;
  }else{
    return 0;
  }
}



static int16_t detail_menu_layer_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){

  return graphics_text_layout_get_content_size(routeList[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), PBL_IF_ROUND_ELSE(GRect(0,0,176,180), GRect(0, 0, 140, 100)), GTextOverflowModeWordWrap, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft)).h+8;
   
}



static void detail_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  //todo: layer_get_bounds here with cell_layer to determine the GRect to draw into
  GRect bounds = layer_get_bounds(cell_layer);
  bounds.origin.x +=2;
  bounds.size.w -=4;
  graphics_draw_text(ctx, routeList[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), bounds, GTextOverflowModeWordWrap, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), NULL);

}

//refreshes the menu layer
static void refreshMenuLayer(){
  if(s_menu_layer){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Refreshing Buses Menu Layer");
    menu_layer_reload_data(s_menu_layer);
    layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  }
}

static void window_load(Window *window){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Calling the load handler for the bus list window");
  #ifdef PBL_COLOR
  window_set_background_color(window, GColorBlack);
  #endif
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  bounds.origin.y += STATUS_BAR_LAYER_HEIGHT;
  bounds.size.h -= STATUS_BAR_LAYER_HEIGHT;
  //add some loading text
  #ifdef PBL_ROUND
  bus_loading_text_layer = text_layer_create(bounds);
  text_layer_set_text_alignment(bus_loading_text_layer, GTextAlignmentCenter);
  #else
  bus_loading_text_layer = text_layer_create(GRect(5, 0+STATUS_BAR_LAYER_HEIGHT, 144-5, 168-STATUS_BAR_LAYER_HEIGHT));
  #endif
  
  text_layer_set_font(bus_loading_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(bus_loading_text_layer, "Loading Buses...");
  text_layer_set_text_color(bus_loading_text_layer, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
  text_layer_set_background_color(bus_loading_text_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(bus_loading_text_layer));
  #ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(bus_loading_text_layer, 5);
  #endif
  
  //get menu layer ready
  
  s_menu_layer = menu_layer_create(bounds);
  
  //set callbacks for the bus details menu layer
   menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = detail_menu_get_num_rows_callback,
    .draw_row = detail_menu_draw_row_callback,
    .get_cell_height = detail_menu_layer_get_cell_height_callback,
    .select_click = menu_select_callback,
  });
  
  // Set up the status bar last to ensure it is on top of other Layers
  s_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
  
  
  
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
#if defined(PBL_COLOR)
  menu_layer_set_highlight_colors(s_menu_layer, GColorRed, GColorWhite);
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
#endif
  
  
}

static void window_unload(){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Calling the unload handler for the bus list window");
  menu_layer_destroy(s_menu_layer);
  status_bar_layer_destroy(s_status_bar);
  text_layer_destroy(bus_loading_text_layer);
  window_destroy(s_window);
  s_window = NULL;
}



void bus_list_window_push(void (*param_menu_select_callback)()) {
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

void bus_list_window_set_data(char **param_routeList){
  routeList = param_routeList;
  
  if(numETAs ==0){
    text_layer_set_text(bus_loading_text_layer, "There are no buses serving this stop right now.");
  }else{
    refreshMenuLayer();
    layer_add_child(window_get_root_layer(s_window), menu_layer_get_layer(s_menu_layer));
  }
  refreshMenuLayer();
}

void bus_list_window_remove(){
  window_stack_remove(s_window, true);
}




