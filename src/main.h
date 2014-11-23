#pragma once

#include "global.h"
#include "schedule.h"

#define NUM_MAIN_MENU_SECTIONS 1
int num_main_menu_items;
list_ptr_t main_menu_items;
int main_menu_enable;

static Window *main_window;
static MenuLayer *main_menu_layer;
static GBitmap *menu_icon_stop;