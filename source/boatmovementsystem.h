#pragma once

#include "allocator.h"

typedef struct TbWorld TbWorld;
typedef struct ecs_query_t ecs_query_t;

#define BoatMovementSystemId 0xDEADF005

typedef struct ThsBoatMovementSystem {
  TbAllocator tmp_alloc;
  ecs_query_t *ocean_query;
} ThsBoatMovementSystem;

void ths_register_boat_movement_sys(TbWorld *world);
void ths_unregister_boat_movement_sys(TbWorld *world);
