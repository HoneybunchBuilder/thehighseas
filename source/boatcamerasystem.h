#pragma once

#include "allocator.h"

#define BoatCameraSystemId 0xDEADF004

typedef struct SystemDescriptor SystemDescriptor;
typedef struct InputSystem InputSystem;

typedef struct BoatCameraSystemDescriptor {
  Allocator tmp_alloc;
} BoatCameraSystemDescriptor;

typedef struct BoatCameraSystem {
  Allocator tmp_alloc;
  InputSystem *input;
} BoatCameraSystem;

void tb_boat_camera_system_descriptor(
    SystemDescriptor *desc, const BoatCameraSystemDescriptor *cam_desc);
