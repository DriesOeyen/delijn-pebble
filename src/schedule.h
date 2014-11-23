#pragma once

#include "global.h"

#define NUM_SCHEDULE_MENU_SECTIONS 1
int num_schedule_menu_items;
list_ptr_t schedule_menu_items;
char schedule_stop_name[MAX_STR_LENGTH];

static Window *schedule_window;
static MenuLayer *schedule_menu_layer;

void show_schedule(int id);