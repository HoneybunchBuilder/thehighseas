#pragma once

#include "allocator.h"

#define BoatMovementSystemId 0xDEADF005

typedef struct SystemDescriptor SystemDescriptor;
typedef struct InputSystem InputSystem;
typedef struct VisualLoggingSystem VisualLoggingSystem;

typedef struct BoatMovementSystemDescriptor {
  Allocator tmp_alloc;
} BoatMovementSystemDescriptor;

typedef struct BoatMovementSystem {
  Allocator tmp_alloc;
  VisualLoggingSystem *vlog;
  InputSystem *input;
} BoatMovementSystem;

void tb_boat_movement_system_descriptor(
    SystemDescriptor *desc, const BoatMovementSystemDescriptor *mov_desc);
