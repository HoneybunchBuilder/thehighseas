#pragma once

#include "allocator.h"

typedef struct TbWorld TbWorld;

typedef struct BoatCameraSystem {
  TbAllocator tmp_alloc;
} BoatCameraSystem;

void ths_register_boat_camera_sys(TbWorld *world);
void ths_unregister_boat_camera_sys(TbWorld *world);
