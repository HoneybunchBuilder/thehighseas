#pragma once

#include "allocator.h"

#define BoatMovementSystemId 0xDEADF005

typedef struct SystemDescriptor SystemDescriptor;
typedef struct VisualLoggingSystem VisualLoggingSystem;

typedef struct BoatMovementSystemDescriptor {
  Allocator tmp_alloc;
} BoatMovementSystemDescriptor;

typedef struct BoatMovementSystem {
  Allocator tmp_alloc;
  VisualLoggingSystem *vlog;
} BoatMovementSystem;

void tb_boat_movement_system_descriptor(
    SystemDescriptor *desc, const BoatMovementSystemDescriptor *mov_desc);
