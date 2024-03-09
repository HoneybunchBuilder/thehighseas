#include "boatcameracomponent.h"

#include "world.h"

#include <flecs.h>
#include <json.h>

ECS_COMPONENT_DECLARE(ThsBoatCameraComponent);

typedef struct ThsBoatCameraDescriptor {
  float min_dist;
  float max_dist;
  float move_speed;
  float zoom_speed;
  float pitch_limit;
} ThsBoatCameraDescriptor;
ECS_COMPONENT_DECLARE(ThsBoatCameraDescriptor);

bool ths_load_boat_camera_comp(TbWorld *world, ecs_entity_t ent,
                               const char *source_path, const cgltf_node *node,
                               json_object *object) {
  (void)node;
  (void)source_path;
  ThsBoatCameraComponent comp = {0};
  json_object_object_foreach(object, key, value) {
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
  ecs_set_ptr(world->ecs, ent, ThsBoatCameraComponent, &comp);
  return true;
}

void ths_destroy_boat_camera_comp(TbWorld *world, ecs_entity_t ent) {
  ecs_remove(world->ecs, ent, ThsBoatCameraComponent);
}

ecs_entity_t ths_register_boat_camera_comp(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT_DEFINE(ecs, ThsBoatCameraDescriptor);
  ECS_COMPONENT_DEFINE(ecs, ThsBoatCameraComponent);

  ecs_struct(ecs,
             {
                 .entity = ecs_id(ThsBoatCameraDescriptor),
                 .members =
                     {
                         {.name = "min_dist", .type = ecs_id(ecs_f32_t)},
                         {.name = "max_dist", .type = ecs_id(ecs_f32_t)},
                         {.name = "move_speed", .type = ecs_id(ecs_f32_t)},
                         {.name = "zoom_speed", .type = ecs_id(ecs_f32_t)},
                         {.name = "pitch_limit", .type = ecs_id(ecs_f32_t)},
                     },
             });

  return ecs_id(ThsBoatCameraDescriptor);
}

TB_REGISTER_COMP(ths, boat_camera)
