#include "sailingcomponents.h"
#include "SDL2/SDL_stdinc.h"
#include "json-c/json_object.h"
#include "json-c/linkhash.h"
#include "world.h"

bool create_wind_component(WindComponent *comp, const WindComponent *desc,
                           uint32_t system_dep_count,
                           System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = *desc;
  return true;
}

void destroy_wind_component(WindComponent *comp, uint32_t system_dep_count,
                            System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (WindComponent){.strength = 0};
}

bool create_boat_movement_component(BoatMovementComponent *comp,
                                    const BoatMovementComponentDesc *desc,
                                    uint32_t system_dep_count,
                                    System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (BoatMovementComponent){
      .bouyancy = desc->bouyancy,
      .heading_change_speed = desc->heading_change_speed,
      .max_acceleration = desc->max_acceleration,
      .max_speed = desc->max_speed,
      .inertia = desc->inertia,
      .friction = desc->friction,
  };
  return true;
}

void destroy_boat_movement_component(BoatMovementComponent *comp,
                                     uint32_t system_dep_count,
                                     System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (BoatMovementComponent){.bouyancy = 0};
}

bool create_mast_component(MastComponent *comp, const MastComponent *desc,
                           uint32_t system_dep_count,
                           System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = *desc;
  return true;
}

void destroy_mast_component(MastComponent *comp, uint32_t system_dep_count,
                            System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (MastComponent){.heading_change_speed = 0};
}

bool create_boat_camera_component(BoatCameraComponent *comp,
                                  const BoatCameraComponentDesc *desc,
                                  uint32_t system_dep_count,
                                  System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (BoatCameraComponent){
      .min_dist = desc->min_dist,
      .max_dist = desc->max_dist,
      .move_speed = desc->move_speed,
      .zoom_speed = desc->zoom_speed,
      .pitch_limit = desc->pitch_limit,
  };
  return true;
}

bool deserialize_boat_camera_component(json_object *json, void *out_desc) {
  BoatCameraComponentDesc *desc = (BoatCameraComponentDesc *)out_desc;
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "min_dist") == 0) {
      desc->min_dist = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_dist") == 0) {
      desc->max_dist = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "move_speed") == 0) {
      desc->move_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "zoom_speed") == 0) {
      desc->zoom_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "pitch_limit") == 0) {
      desc->pitch_limit = (float)json_object_get_double(value);
    }
  }
  return true;
}

void destroy_boat_camera_component(BoatCameraComponent *comp,
                                   uint32_t system_dep_count,
                                   System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (BoatCameraComponent){.move_speed = 0};
}

TB_DEFINE_COMPONENT(wind, WindComponent, WindComponent)
TB_DEFINE_COMPONENT(boat_movement, BoatMovementComponent,
                    BoatMovementComponentDesc)
TB_DEFINE_COMPONENT(mast, MastComponent, MastComponent)
TB_DEFINE_COMPONENT(boat_camera, BoatCameraComponent, BoatCameraComponentDesc)

void tb_wind_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "Wind",
      .size = sizeof(WindComponent),
      .id = WindComponentId,
      .create = tb_create_wind_component,
      .destroy = tb_destroy_wind_component,
  };
}

void tb_boat_movement_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "BoatMovement",
      .size = sizeof(BoatMovementComponent),
      .id = BoatMovementComponentId,
      .create = tb_create_boat_movement_component,
      .destroy = tb_destroy_boat_movement_component,
  };
}

void tb_mast_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "Mast",
      .size = sizeof(MastComponent),
      .id = MastComponentId,
      .create = tb_create_mast_component,
      .destroy = tb_destroy_mast_component,
  };
}

void tb_boat_camera_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "BoatCamera",
      .size = sizeof(BoatCameraComponent),
      .desc_size = sizeof(BoatCameraComponentDesc),
      .id = BoatCameraComponentId,
      .id_str = BoatCameraComponentIdStr,
      .create = tb_create_boat_camera_component,
      .deserialize = deserialize_boat_camera_component,
      .destroy = tb_destroy_boat_camera_component,
  };
}
