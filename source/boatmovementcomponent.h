#pragma once

#include "simd.h"

#include <flecs.h>

// State for managing movement of the boat
// Both the speed and heading of the ship as well as how it rotates and bobs on
// the waves
typedef struct ThsBoatMovementComponent {
  float heading_change_speed; // How fast the boat will face the target heading
  float max_acceleration;
  float max_speed;
  float inertia;  // The magnitude of velocity required to start moving
  float friction; // How fast the boat will come to a stop

  float target_height_offset; // Target height offset to move to
  float3 target_heading;      // Direction we want the boat to face
  // Current heading is the attached transform component's forward
  float acceleration;
  float speed;
} ThsBoatMovementComponent;
extern ECS_COMPONENT_DECLARE(ThsBoatMovementComponent);
