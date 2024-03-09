#include "boatmovementcomponent.h"

#include "world.h"
#include <json.h>

ECS_COMPONENT_DECLARE(ThsBoatMovementComponent);

typedef struct ThsBoatMovementDescriptor {
  float heading_change_speed;
  float acceleration;
  float max_speed;
  float inertia;
  float friction;
} ThsBoatMovementDescriptor;
ECS_COMPONENT_DECLARE(ThsBoatMovementDescriptor);

bool ths_load_boat_movement_comp(TbWorld *world, ecs_entity_t ent,
                                 const char *source_path,
                                 const cgltf_node *node, json_object *json) {
  (void)node;
  (void)source_path;
  ThsBoatMovementComponent comp = {0};
  json_object_object_foreach(json, key, value) {
    if (SDL_strcmp(key, "heading_change_speed") == 0) {
      comp.heading_change_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "acceleration") == 0) {
      comp.acceleration = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "max_speed") == 0) {
      comp.max_speed = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "inertia") == 0) {
      comp.inertia = (float)json_object_get_double(value);
    } else if (SDL_strcmp(key, "friction") == 0) {
      comp.friction = (float)json_object_get_double(value);
    }
  }
  ecs_set_ptr(world->ecs, ent, ThsBoatMovementComponent, &comp);
  return true;
}

void ths_destroy_boat_movement_comp(TbWorld *world, ecs_entity_t ent) {
  ecs_remove(world->ecs, ent, ThsBoatMovementComponent);
}

ecs_entity_t ths_register_boat_movement_comp(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT_DEFINE(ecs, ThsBoatMovementDescriptor);
  ECS_COMPONENT_DEFINE(ecs, ThsBoatMovementComponent);

  ecs_struct(
      ecs,
      {
          .entity = ecs_id(ThsBoatMovementDescriptor),
          .members =
              {
                  {.name = "heading_change_speed", .type = ecs_id(ecs_f32_t)},
                  {.name = "acceleration", .type = ecs_id(ecs_f32_t)},
                  {.name = "max_delta", .type = ecs_id(ecs_f32_t)},
                  {.name = "inertia", .type = ecs_id(ecs_f32_t)},
                  {.name = "friction", .type = ecs_id(ecs_f32_t)},
              },
      });

  return ecs_id(ThsBoatMovementDescriptor);
}

TB_REGISTER_COMP(ths, boat_movement)