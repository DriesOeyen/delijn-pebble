#pragma once

#include <pebble.h>
#include "list.h"

#define KEY_TYPE 0
#define KEY_ERROR 1
#define KEY_BUS_LIJNID 2
#define KEY_BUS_DESTINATION 3
#define KEY_BUS_ETA 4
#define KEY_STOP_ID 5
#define KEY_STOP_STOPID 6
#define KEY_STOP_LIJNID 7
#define KEY_STOP_NAME 8

#define VALUE_TYPE_STOP 0
#define VALUE_TYPE_BUS 1
#define VALUE_TYPE_ERROR 2

#define ERROR_EMPTY 0
#define ERROR_CONNECTION 1
#define ERROR_UNKNOWN 2

#define MAX_STR_LENGTH 30

typedef struct{
	char title[MAX_STR_LENGTH];
	char subtitle[MAX_STR_LENGTH];
} menu_item_t;
typedef menu_item_t *menu_item_ptr_t;

char* translate_error(AppMessageResult result);