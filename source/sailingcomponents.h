#pragma once

#include "simd.h"

#define WindComponentId 0xDEAD0000
#define WindComponentIdStr "0xDEAD0000"
#define BoatMovementComponentId 0xDEAD0001
#define BoatMovementComponentIdStr "0xDEAD0001"
#define HullComponentId 0xDEAD0002
#define HullComponentIdStr "0xDEAD0002"
#define MastComponentId 0xDEAD0003
#define MastComponentIdStr "0xDEAD0003"
#define BoatCameraComponentId 0xDEAD0004
#define BoatCameraComponentIdStr "0xDEAD0004"

typedef struct TbWorld TbWorld;

typedef struct MeshComponent MeshComponent;
typedef struct TransformComponent TransformComponent;

// Tracks the state of the wind
// Currently this is intended to mostly be a single use global component
// but in the future the idea is to look up this component based on
// some sort of weather pattern lookup
typedef struct WindComponent {
  float3 direction;
  float strength;
} WindComponent;

// State for managing movement of the boat
// Both the speed and heading of the ship as well as how it rotates and bobs on
// the waves
typedef struct BoatMovementComponent {
  float target_height_offset; // Target height offset to move to

  float heading_change_speed; // How fast the boat will face the target heading
  float3 target_heading;      // Direction we want the boat to face
  // Current heading is the attached transform component's forward

  float max_acceleration;
  float acceleration;
  float max_speed;
  float speed;

  float inertia;  // The magnitude of velocity required to start moving
  float friction; // How fast the boat will come to a stop
} BoatMovementComponent;

typedef struct HullComponent {
  float bouyancy; // How fast the hull will lerp to the target height
  // Filled in via on_loaded
  float width;
  float depth;

  float max_speed;
  float speed;

  float heading_velocity; // how fast the heading changes
} HullComponent;

typedef struct MastComponent {
  float heading_change_speed; // How fast the mast will face the target heading
  float3 target_heading;      // Direction we want the mast to face
} MastComponent;

typedef struct BoatCameraComponent {
  float min_dist;
  float max_dist;
  float move_speed;
  float zoom_speed;
  float pitch_limit;

  float target_dist;
  float3 target_hull_to_camera;
} BoatCameraComponent;

void ths_register_sailing_components(TbWorld *world);
