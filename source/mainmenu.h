#pragma once

#include "allocator.h"

typedef struct TbWorld TbWorld;

// Unload the scene and load the main menu world
// which will kickstart the main menu system
void ths_return_to_main_menu(TbWorld *world);
