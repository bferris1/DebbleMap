#include <pebble.h>
#include "stop_list_window.h"
#include "bus_list_window.h"
#include "data.h"
#include "message_window.h"
#define CONTENT_STOPS 0
#define CONTENT_BUSES 1
#define CONTENT_CONFIG 2
#define CONTENT_ERROR 3


Window *splash_window;

static GBitmap *s_bus_icon;
static BitmapLayer *s_bitmap_layer;
  
//storage for stops and routes
char **stopList;
char **routeList;

//function to free an array of strings
void free_array(char **array, int numElements){
    for(int i = 0; i<numElements; i++){
      free(array[i]);
    }
    free(array);
  }

//convenience function to send a single integer value to the phone

static void send_int(int key, int value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &value, sizeof(int), true);
  app_message_outbox_send();
}

static void detail_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context){
    send_int(0, selectedStopIndex);
}
static void main_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  //push details window to the stack
  selectedStopIndex = cell_index->row;
  
  bus_list_window_push(detail_menu_select_callback);
  send_int(0, selectedStopIndex);
}

	
// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
};

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *tuple;
  
  switch (dict_find(received, 0)->value->int8){
    case CONTENT_STOPS: //data are stops
    free_array(stopList, numStops);
    tuple = dict_find(received, 1);
    numStops = tuple->value->int16;
    //dynamically allocate array of pointers to stop names
    stopList = malloc(numStops*sizeof(char *));
    
    
    for(int i=0;i<numStops;i++){
    tuple = dict_find(received, i+2);
    if(tuple){
      //allocate char pointer for stop name and copy string to it
      stopList[i] = (char*) malloc(strlen(tuple->value->cstring) + 1);
      strcpy(stopList[i], tuple->value->cstring);
    }
  }
    //remove the bus list window (if the bus system is changed and ETAs are visible)
    bus_list_window_remove();
    stop_list_window_set_data(stopList);
    stop_list_window_push(&main_menu_select_callback);
    if(window_stack_contains_window(splash_window)){
      
      window_stack_remove(splash_window, false);
      gbitmap_destroy(s_bus_icon);
      bitmap_layer_destroy(s_bitmap_layer);
      window_destroy(splash_window);
      
    }
    
    
    break;
    
    case CONTENT_BUSES://data are bus arrival times
    free_array(routeList, numETAs);
    tuple = dict_find(received, 1);
    numETAs = tuple->value->int16;
    routeList = malloc(numETAs*sizeof(char *));
    if(numETAs>0){
      for(int j = 0;j<numETAs;j++){
        tuple = dict_find(received, j+2);
        routeList[j] = malloc(strlen(tuple->value->cstring) + 1);
        strcpy(routeList[j], tuple->value->cstring);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "%d %s",j, routesList[j].name);
      }
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "0 test %s", routesList[0].name);

      
    }
    //have the bus list window refresh to use the new data
      bus_list_window_set_data(routeList);
    
    break;
    
    case CONTENT_CONFIG: //data is config settings
    //this is not currently needed and does nothing
    
    
    
    break;
    //if an error message is received from the phone, display it
    case CONTENT_ERROR: //phone is reporting an error
    errorMessage = dict_find(received, 1)->value->cstring;
    window_stack_pop_all(false);
    dialog_message_window_push();
    
    break;
  }
  
}
  

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
    window_stack_pop_all(false);
    errorMessage = "A communication error has occurred.";
    dialog_message_window_push();

}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
    errorMessage = "A communication error has occurred.";
    window_stack_pop_all(false);
    dialog_message_window_push();
}


void init(void) {
  numETAs = 0;
  numStops = 0;
  //the splash window might be initialized here (or just combined with stop list window)
  splash_window = window_create();
  GRect bounds = layer_get_bounds(window_get_root_layer(splash_window));
  s_bitmap_layer = bitmap_layer_create(bounds);
  
  #ifdef PBL_COLOR
  s_bus_icon = gbitmap_create_with_resource(RESOURCE_ID_BUS_ICON_WHITE);
  window_set_background_color(splash_window, GColorDarkCandyAppleRed);
  #else
  s_bus_icon = gbitmap_create_with_resource(RESOURCE_ID_BUS_BW_ICON);
  #endif
  window_stack_push(splash_window, true);
  
  
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bus_icon);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(splash_window), bitmap_layer_get_layer(s_bitmap_layer));

  window_stack_push(splash_window, true);
  
  if(!connection_service_peek_pebble_app_connection()){
    errorMessage = "Bluetooth Connection Error";
    window_stack_pop_all(false);
    dialog_message_window_push();
  }else{
    
  // Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(dict_calc_buffer_size(10, sizeof(char[50])), dict_calc_buffer_size(1, sizeof(int)));
    
  }
	
}

void deinit(void) {
	app_message_deregister_callbacks();
  free_array(stopList, numStops);
  free_array(routeList, numETAs);
  // Destroy the BitmapLayer
  //bitmap_layer_destroy(s_bitmap_layer);
  //window_destroy(splash_window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}