#include "schedule.h"



// APPMESSAGE FUNCTIONS

// Outgoing ACK
void schedule_out_sent_handler(DictionaryIterator *sent, void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent successfully");
}

// Outgoing NACK
void schedule_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending message (%d)", reason);
}

// Incoming ACK
void schedule_in_received_handler(DictionaryIterator *received, void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message received successfully");
	Tuple *tup_type = dict_find(received, KEY_TYPE);
	menu_item_ptr_t menu_item;
	
	switch(tup_type->value->int8){
		case(VALUE_TYPE_BUS):
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Type of message: bus");
			Tuple *tup_bus_destination = dict_find(received, KEY_BUS_DESTINATION);
			Tuple *tup_bus_eta = dict_find(received, KEY_BUS_ETA);
			// Add to list of menu items
			menu_item = malloc(sizeof(menu_item_t));
			if(menu_item != NULL){
				// Parse incoming data
				snprintf(menu_item->title, MAX_STR_LENGTH, "%s", tup_bus_destination->value->cstring);
				snprintf(menu_item->subtitle, MAX_STR_LENGTH, "%s", tup_bus_eta->value->cstring);
				// Add to menu items and reload menu
				list_insert_at_index(schedule_menu_items, menu_item, num_schedule_menu_items);
				num_schedule_menu_items++;
				menu_layer_reload_data(schedule_menu_layer);
			} else{
				APP_LOG(APP_LOG_LEVEL_ERROR, "Couldn't allocate memory for menu item");
			}
			break;
		
		case(VALUE_TYPE_STOP):
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Type of message: stop");
			Tuple *tup_stop_name = dict_find(received, KEY_STOP_NAME);
			snprintf(schedule_stop_name, MAX_STR_LENGTH, "%s", tup_stop_name->value->cstring);
			menu_layer_reload_data(schedule_menu_layer);
			break;
		
		case(VALUE_TYPE_ERROR):
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Type of message: error");
			Tuple *tup_error = dict_find(received, KEY_ERROR);
			switch(tup_error->value->int8){
				case(ERROR_EMPTY):
					// No upcoming buses
					menu_item = malloc(sizeof(menu_item_t));
					if(menu_item != NULL){
						snprintf(menu_item->title, MAX_STR_LENGTH, "Bus gemist?");
						snprintf(menu_item->subtitle, MAX_STR_LENGTH, "Geen doorkomsten");
						// Add to menu items and reload menu
						list_insert_at_index(schedule_menu_items, menu_item, num_schedule_menu_items);
						num_schedule_menu_items++;
						menu_layer_reload_data(schedule_menu_layer);
					} else{
						APP_LOG(APP_LOG_LEVEL_ERROR, "Couldn't allocate memory for menu item");
					}
					break;
				case(ERROR_CONNECTION):
					// No internet connection
					menu_item = malloc(sizeof(menu_item_t));
					if(menu_item != NULL){
						snprintf(menu_item->title, MAX_STR_LENGTH, "Geen verbinding");
						snprintf(menu_item->subtitle, MAX_STR_LENGTH, "Je telefoon is offline");
						// Add to menu items and reload menu
						list_insert_at_index(schedule_menu_items, menu_item, num_schedule_menu_items);
						num_schedule_menu_items++;
						menu_layer_reload_data(schedule_menu_layer);
					} else{
						APP_LOG(APP_LOG_LEVEL_ERROR, "Couldn't allocate memory for menu item");
					}
					break;
				default:
					APP_LOG(APP_LOG_LEVEL_ERROR, "Received unknown error code from JS app (%d)", tup_error->value->int8);
					break;
			}
			break;
		
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Received message with unknown type (%d)", tup_type->value->int8);
			break;
	}
}

// Incoming NACK
void schedule_in_dropped_handler(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "Error receiving message (%d)", reason);
}

// Request schedule
void request_schedule(int id){
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet tup_type = TupletInteger(KEY_TYPE, VALUE_TYPE_STOP);
	Tuplet tup_stop_id = TupletInteger(KEY_STOP_ID, id);
	dict_write_tuplet(iter, &tup_type);
	dict_write_tuplet(iter, &tup_stop_id);
	app_message_outbox_send();
}



// SCHEDULE WINDOW FUNCTIONS

// Get number of sections
static uint16_t schedule_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data){
	return NUM_SCHEDULE_MENU_SECTIONS;
}

// Get number of rows in a section
static uint16_t schedule_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data){
	if(num_schedule_menu_items == 0){
		return 1; // Allows loading message to appear
	}
	return num_schedule_menu_items;
}

// Get height of header
static int16_t schedule_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data){
	return MENU_CELL_BASIC_HEADER_HEIGHT; // Use default height from pebble.h
}

// Draw header
static void schedule_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data){
	menu_cell_basic_header_draw(ctx, cell_layer, schedule_stop_name);
}

// Draw item
static void schedule_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data){
	menu_item_ptr_t menu_item;
	if(list_size(schedule_menu_items) > 0){
		menu_item = list_get_data_at_index(schedule_menu_items, cell_index->row);
		menu_cell_basic_draw(ctx, cell_layer, menu_item->title, menu_item->subtitle, NULL);
	} else{
		menu_cell_basic_draw(ctx, cell_layer, "Laden...", NULL, NULL);
	}
}

// Init schedule window
void schedule_window_load(Window *window){
	// Init menu
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	schedule_menu_layer = menu_layer_create(bounds);

	// Init menu callbacks
	menu_layer_set_callbacks(schedule_menu_layer, NULL, (MenuLayerCallbacks){
		.get_num_sections = schedule_menu_get_num_sections_callback,
		.get_num_rows = schedule_menu_get_num_rows_callback,
		.get_header_height = schedule_menu_get_header_height_callback,
		.draw_header = schedule_menu_draw_header_callback,
		.draw_row = schedule_menu_draw_row_callback,
	});
	
	// Init menu items list
	schedule_menu_items = list_alloc();
	num_schedule_menu_items = 0;
	snprintf(schedule_stop_name, MAX_STR_LENGTH, "Laden...");
	
	// Bind menu
	menu_layer_set_click_config_onto_window(schedule_menu_layer, window);
	layer_add_child(window_layer, menu_layer_get_layer(schedule_menu_layer));

	// Init AppMessage
	app_message_register_inbox_received(schedule_in_received_handler);
	app_message_register_inbox_dropped(schedule_in_dropped_handler);
	app_message_register_outbox_sent(schedule_out_sent_handler);
	app_message_register_outbox_failed(schedule_out_failed_handler);
}

// Deinit schedule window
void schedule_window_unload(Window *window){
	app_message_deregister_callbacks();
	menu_layer_destroy(schedule_menu_layer);
	list_free_all(&schedule_menu_items);
	window_destroy(schedule_window);
}

void show_schedule(int id){
	request_schedule(id);
	schedule_window = window_create();
	window_set_window_handlers(schedule_window, (WindowHandlers) {
		.load = schedule_window_load,
		.unload = schedule_window_unload,
	});
	window_stack_push(schedule_window, true);
}