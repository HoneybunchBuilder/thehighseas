#include "sailingcomponents.h"

#include "assetsystem.h"
#include "boatmovementsystem.h"
#include "meshcomponent.h"
#include "tbcommon.h"
#include "transformcomponent.h"
#include "world.h"

#include <SDL2/SDL_stdinc.h>
#include <flecs.h>
#include <json.h>

BoatMovementComponent deserialize_boat_movement_component(json_object *json) {
  BoatMovementComponent comp = {0};
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "heading_change_speed") == 0) {
      comp.heading_change_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_acceleration") == 0) {
      comp.max_acceleration = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_speed") == 0) {
      comp.max_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "inertia") == 0) {
      comp.inertia = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "friction") == 0) {
      comp.friction = (float)json_object_get_double(value);
    }
  }
  return comp;
}

HullComponent deserialize_hull_component(json_object *json) {
  HullComponent comp = {0};
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "bouyancy") == 0) {
      comp.bouyancy = (float)json_object_get_double(value);
    }
  }
  return comp;
}

BoatCameraComponent deserialize_boat_camera_component(json_object *json) {
  BoatCameraComponent comp = {0};
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "min_dist") == 0) {
      comp.min_dist = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_dist") == 0) {
      comp.max_dist = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "move_speed") == 0) {
      comp.move_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "zoom_speed") == 0) {
      comp.zoom_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "pitch_limit") == 0) {
      comp.pitch_limit = (float)json_object_get_double(value);
    }
  }
  return comp;
}

bool create_sailing_components(ecs_world_t *ecs, ecs_entity_t e,
                               const char *source_path, const cgltf_node *node,
                               json_object *extra) {
  (void)source_path;
  (void)node;
  if (extra) {
    json_object_object_foreach(extra, key, value) {
      if (SDL_strcmp(key, "id") == 0) {
        const char *id_str = json_object_get_string(value);
        if (SDL_strcmp(id_str, BoatMovementComponentIdStr) == 0) {
          ECS_COMPONENT(ecs, BoatMovementComponent);
          BoatMovementComponent comp =
              deserialize_boat_movement_component(extra);
          ecs_set_ptr(ecs, e, BoatMovementComponent, &comp);
        }
        if (SDL_strcmp(id_str, HullComponentIdStr) == 0) {
          ECS_COMPONENT(ecs, HullComponent);
          HullComponent comp = deserialize_hull_component(extra);
          ecs_set_ptr(ecs, e, HullComponent, &comp);
        }
        if (SDL_strcmp(id_str, BoatCameraComponentIdStr) == 0) {
          ECS_COMPONENT(ecs, BoatCameraComponent);
          BoatCameraComponent comp = deserialize_boat_camera_component(extra);
          ecs_set_ptr(ecs, e, BoatCameraComponent, &comp);
        }
      }
    }
  }
  return true;
}

void destroy_sailing_components(ecs_world_t *ecs) {
  ECS_COMPONENT(ecs, BoatMovementComponent);
  ECS_COMPONENT(ecs, HullComponent);
  ECS_COMPONENT(ecs, BoatCameraComponent);

  {
    ecs_filter_t *filter =
        ecs_filter(ecs, {
                            .terms =
                                {
                                    {.id = ecs_id(BoatMovementComponent)},
                                },
                        });
    ecs_iter_t it = ecs_filter_iter(ecs, filter);
    while (ecs_filter_next(&it)) {
      BoatMovementComponent *comp = ecs_field(&it, BoatMovementComponent, 1);
      for (int32_t i = 0; i < it.count; ++i) {
        *comp = (BoatMovementComponent){0};
      }
    }
    ecs_filter_fini(filter);
  }
  {
    ecs_filter_t *filter =
        ecs_filter(ecs, {
                            .terms =
                                {
                                    {.id = ecs_id(HullComponent)},
                                },
                        });
    ecs_iter_t it = ecs_filter_iter(ecs, filter);
    while (ecs_filter_next(&it)) {
      HullComponent *comp = ecs_field(&it, HullComponent, 1);
      for (int32_t i = 0; i < it.count; ++i) {
        *comp = (HullComponent){0};
      }
    }
    ecs_filter_fini(filter);
  }
  {
    ecs_filter_t *filter =
        ecs_filter(ecs, {
                            .terms =
                                {
                                    {.id = ecs_id(BoatCameraComponent)},
                                },
                        });
    ecs_iter_t it = ecs_filter_iter(ecs, filter);
    while (ecs_filter_next(&it)) {
      BoatCameraComponent *comp = ecs_field(&it, BoatCameraComponent, 1);
      for (int32_t i = 0; i < it.count; ++i) {
        *comp = (BoatCameraComponent){0};
      }
    }
    ecs_filter_fini(filter);
  }
}

void ths_register_sailing_components(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, AssetSystem);
  ECS_COMPONENT(ecs, BoatMovementSystem);

  // Register asset system for parsing sailing components
  AssetSystem asset = {
      .add_fn = create_sailing_components,
      .rem_fn = destroy_sailing_components,
  };
  ecs_set_ptr(ecs, ecs_id(BoatMovementSystem), AssetSystem, &asset);
}
