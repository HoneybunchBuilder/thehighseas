#pragma once

#include "simd.h"

#include <flecs.h>

typedef struct ThsBoatCameraComponent {
  float min_dist;
  float max_dist;
  float move_speed;
  float zoom_speed;
  float pitch_limit;

  float target_dist;
  float3 target_hull_to_camera;
} ThsBoatCameraComponent;
extern ECS_COMPONENT_DECLARE(ThsBoatCameraComponent);
