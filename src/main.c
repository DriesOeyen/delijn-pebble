#include "main.h"



// APPMESSAGE FUNCTIONS

// Outgoing ACK
void main_out_sent_handler(DictionaryIterator *sent, void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent successfully");
}

// Outgoing NACK
void main_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending message (%s)", translate_error(reason));
}

// Incoming ACK
void main_in_received_handler(DictionaryIterator *received, void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message received successfully");
	Tuple *tup_type = dict_find(received, KEY_TYPE);
	menu_item_ptr_t menu_item;
	
	switch(tup_type->value->int8){
		case(VALUE_TYPE_STOP):
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Type of message: stop");
			main_menu_enable = 1;
			Tuple *tup_stop_id = dict_find(received, KEY_STOP_ID);
			Tuple *tup_stop_lijnid = dict_find(received, KEY_STOP_LIJNID);
			Tuple *tup_stop_name = dict_find(received, KEY_STOP_NAME);
			// Add to list of menu items
			menu_item = malloc(sizeof(menu_item_t));
			if(menu_item != NULL){
				// Parse incoming data
				snprintf(menu_item->title, MAX_STR_LENGTH, "%s", tup_stop_name->value->cstring);
				if(tup_stop_lijnid->value->int16 == -1){
					snprintf(menu_item->subtitle, MAX_STR_LENGTH, "Alle lijnen");
				} else{
					snprintf(menu_item->subtitle, MAX_STR_LENGTH, "Lijn %d", tup_stop_lijnid->value->int16);
				}
				// Add to menu items and reload menu
				list_insert_at_index(main_menu_items, menu_item, tup_stop_id->value->uint8);
				num_main_menu_items++;
				menu_layer_reload_data(main_menu_layer);
			} else{
				APP_LOG(APP_LOG_LEVEL_ERROR, "Couldn't allocate memory for menu item");
			}
			break;
		
		case(VALUE_TYPE_ERROR):
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Type of message: error");
			Tuple *tup_error = dict_find(received, KEY_ERROR);
			switch(tup_error->value->int8){
				case(ERROR_EMPTY):
					// No stops
					menu_item = malloc(sizeof(menu_item_t));
					if(menu_item != NULL){
						snprintf(menu_item->title, MAX_STR_LENGTH, "Geen haltes");
						snprintf(menu_item->subtitle, MAX_STR_LENGTH, "Stel in via de Pebble app");
						// Add to menu items and reload menu
						list_insert_at_index(main_menu_items, menu_item, num_main_menu_items);
						num_main_menu_items++;
						menu_layer_reload_data(main_menu_layer);
					} else{
						APP_LOG(APP_LOG_LEVEL_ERROR, "Couldn't allocate memory for menu item");
					}
					break;
				default:
					APP_LOG(APP_LOG_LEVEL_ERROR, "Received unknown error code from JS app (%d)", tup_error->value->int8);
			}
			break;
		
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Received message with unknown type (%d)", tup_type->value->int8);
	}
}

// Incoming NACK
void main_in_dropped_handler(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Error receiving message (%s)", translate_error(reason));
}



// MAIN WINDOW FUNCTIONS

// Get number of sections
static uint16_t main_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data){
	return NUM_MAIN_MENU_SECTIONS;
}

// Get number of rows in a section
static uint16_t main_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data){
	if(num_main_menu_items == 0){
		return 1; // Allows loading message to appear
	}
	return num_main_menu_items;
}

// Get height of header
static int16_t main_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data){
	return MENU_CELL_BASIC_HEADER_HEIGHT; // Use default height from pebble.h
}

// Draw header
static void main_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data){
	switch(section_index){
		case 0: // First section
			menu_cell_basic_header_draw(ctx, cell_layer, "Mijn lijnen");
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Attempted to draw unknown header: %d", section_index);
	}
}

// Draw item
static void main_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data){
	menu_item_ptr_t menu_item;
	if(main_menu_enable == 0 && list_size(main_menu_items) > 0){
		menu_item = list_get_data_at_index(main_menu_items, cell_index->row);
		menu_cell_basic_draw(ctx, cell_layer, menu_item->title, menu_item->subtitle, NULL);
	} else if(list_size(main_menu_items) > 0){
		menu_item = list_get_data_at_index(main_menu_items, cell_index->row);
		menu_cell_basic_draw(ctx, cell_layer, menu_item->title, menu_item->subtitle, menu_icon_stop);
	}else{
		menu_cell_basic_draw(ctx, cell_layer, "Laden...", NULL, NULL);
	}
}

// Handle select button push
void main_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data){
	if(main_menu_enable){
		// Load a schedule
		APP_LOG(APP_LOG_LEVEL_DEBUG, "User selected stop %d", cell_index->row);
		show_schedule(cell_index->row);
	}
}

// Load window
void main_window_load(Window *window){
	// Init bus stop icon
	menu_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NODE);

	// Init menu
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	main_menu_layer = menu_layer_create(bounds);

	// Init menu callbacks
	menu_layer_set_callbacks(main_menu_layer, NULL, (MenuLayerCallbacks){
		.get_num_sections = main_menu_get_num_sections_callback,
		.get_num_rows = main_menu_get_num_rows_callback,
		.get_header_height = main_menu_get_header_height_callback,
		.draw_header = main_menu_draw_header_callback,
		.draw_row = main_menu_draw_row_callback,
		.select_click = main_menu_select_callback,
	});
	
	// Init menu items list
	main_menu_items = list_alloc();
	num_main_menu_items = 0;
	main_menu_enable = 0;
	
	// Bind menu
	menu_layer_set_click_config_onto_window(main_menu_layer, window);
	layer_add_child(window_layer, menu_layer_get_layer(main_menu_layer));
}

// Unload window
void main_window_unload(Window *window){
	app_message_deregister_callbacks();
	menu_layer_destroy(main_menu_layer);
	list_free_all(&main_menu_items);
	gbitmap_destroy(menu_icon_stop);
}

// Init
void init(){
	// Init window
	main_window = window_create();
	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});
	window_stack_push(main_window, true);
	
	// Init AppMessage
	app_message_register_inbox_received(main_in_received_handler);
	app_message_register_inbox_dropped(main_in_dropped_handler);
	app_message_register_outbox_sent(main_out_sent_handler);
	app_message_register_outbox_failed(main_out_failed_handler);

	const uint32_t inbound_size = app_message_inbox_size_maximum();
	const uint32_t outbound_size = app_message_outbox_size_maximum();
	app_message_open(inbound_size, outbound_size);
	
	// Check Bluetooth connection
	if(!bluetooth_connection_service_peek()){
		// No connection
		menu_item_ptr_t menu_item = malloc(sizeof(menu_item_t));
		if(menu_item != NULL){
			snprintf(menu_item->title, MAX_STR_LENGTH, "Geen verbinding");
			snprintf(menu_item->subtitle, MAX_STR_LENGTH, "Je telefoon is niet verbonden");
			// Add to menu items and reload menu
			list_insert_at_index(main_menu_items, menu_item, num_main_menu_items);
			num_main_menu_items++;
			menu_layer_reload_data(main_menu_layer);
		} else{
			APP_LOG(APP_LOG_LEVEL_ERROR, "Couldn't allocate memory for menu item");
		}
	}
}

// Deinit
void deinit(){
	window_destroy(main_window);
}

// Main
int main(void){
	init();
	app_event_loop();
	deinit();
}