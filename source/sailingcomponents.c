#include "sailingcomponents.h"
#include "SDL2/SDL_stdinc.h"
#include "json-c/json_object.h"
#include "json-c/linkhash.h"
#include "meshcomponent.h"
#include "tbcommon.h"
#include "transformcomponent.h"
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
  *comp = (BoatMovementComponent){0};
}

bool deserialize_boat_movement_component(json_object *json, void *out_desc) {
  BoatMovementComponentDesc *desc = (BoatMovementComponentDesc *)out_desc;
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "heading_change_speed") == 0) {
      desc->heading_change_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_acceleration") == 0) {
      desc->max_acceleration = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_speed") == 0) {
      desc->max_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "inertia") == 0) {
      desc->inertia = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "friction") == 0) {
      desc->friction = (float)json_object_get_double(value);
    }
  }
  return true;
}

bool create_hull_component(HullComponent *comp, const HullComponentDesc *desc,
                           uint32_t system_dep_count,
                           System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (HullComponent){
      .bouyancy = desc->bouyancy,
      .heading_velocity = 0.0f,
  };
  return true;
}

void destroy_hull_component(HullComponent *comp, uint32_t system_dep_count,
                            System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *comp = (HullComponent){.bouyancy = 0};
}

bool deserialize_hull_component(json_object *json, void *out_desc) {
  HullComponentDesc *desc = (HullComponentDesc *)out_desc;
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "bouyancy") == 0) {
      desc->bouyancy = (float)json_object_get_double(value);
    }
  }
  return true;
}

bool hull_component_on_loaded(EntityId id, const World *world,
                              void *component) {
  HullComponent *self = (HullComponent *)component;
  // Look up some component stores
  const ComponentStore *transform_store = NULL;
  const ComponentStore *mesh_store = NULL;
  for (uint32_t i = 0; i < world->component_store_count; ++i) {
    const ComponentStore *store = &world->component_stores[i];
    if (store->id == TransformComponentId) {
      transform_store = store;
    }
    if (store->id == MeshComponentId) {
      mesh_store = store;
    }
  }
  TB_CHECK_RETURN(transform_store != NULL && mesh_store != NULL,
                  "Required stores not found", false);
  // Look up hull's transform
  const TransformComponent *hull_trans =
      tb_get_component(transform_store, id, TransformComponent);
  EntityId child = hull_trans->children[2];
  const MeshComponent *child_mesh =
      tb_get_component(mesh_store, child, MeshComponent);
  const TransformComponent *child_transform =
      tb_get_component(transform_store, child, TransformComponent);

  float4x4 obj_mat = transform_to_matrix(&child_transform->transform);
  AABB aabb = (AABB){
      f4tof3(mulf44(obj_mat, f3tof4(child_mesh->local_aabb.min, 1.0f))),
      f4tof3(mulf44(obj_mat, f3tof4(child_mesh->local_aabb.max, 1.0f))),
  };

  float3 diff = aabb.max - aabb.min;
  self->width = diff[0];
  self->depth = diff[2];

  return true;
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
TB_DEFINE_COMPONENT(hull, HullComponent, HullComponentDesc)
TB_DEFINE_COMPONENT(mast, MastComponent, MastComponent)
TB_DEFINE_COMPONENT(boat_camera, BoatCameraComponent, BoatCameraComponentDesc)

void tb_wind_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "Wind",
      .size = sizeof(WindComponent),
      .id = WindComponentId,
      .id_str = WindComponentIdStr,
      .create = tb_create_wind_component,
      .destroy = tb_destroy_wind_component,
  };
}

void tb_boat_movement_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "BoatMovement",
      .size = sizeof(BoatMovementComponent),
      .id = BoatMovementComponentId,
      .id_str = BoatMovementComponentIdStr,
      .create = tb_create_boat_movement_component,
      .destroy = tb_destroy_boat_movement_component,
      .deserialize = deserialize_boat_movement_component,
  };
}

void tb_hull_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "Hull",
      .size = sizeof(HullComponent),
      .id = HullComponentId,
      .id_str = HullComponentIdStr,
      .create = tb_create_hull_component,
      .destroy = tb_destroy_hull_component,
      .deserialize = deserialize_hull_component,
      .on_loaded = hull_component_on_loaded,
  };
}

void tb_mast_component_descriptor(ComponentDescriptor *desc) {
  *desc = (ComponentDescriptor){
      .name = "Mast",
      .size = sizeof(MastComponent),
      .id = MastComponentId,
      .id_str = MastComponentIdStr,
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
