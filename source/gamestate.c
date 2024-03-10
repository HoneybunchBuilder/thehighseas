#include "gamestate.h"

#include "tbcommon.h"
#include "tbgltf.h"
#include "world.h"

#include <flecs.h>
#include <json.h>

ECS_COMPONENT_DECLARE(ThsGameSceneSettings);

typedef struct ThsGameSceneDescriptor {
  const char *scene_type;
} ThsGameSceneDescriptor;
ECS_COMPONENT_DECLARE(ThsGameSceneDescriptor);

bool ths_load_game_state_comp(TbWorld *world, ecs_entity_t ent,
                              const char *source_path, const cgltf_node *node,
                              json_object *object) {
  (void)source_path;
  (void)node;
  ThsGameSceneSettings comp = {0};
  json_object_object_foreach(object, key, value) {
    if (SDL_strcmp(key, "scene_type") == 0) {
      const char *scene_type_str = json_object_get_string(value);
      if (SDL_strcmp(scene_type_str, "game_world") == 0) {
        comp.type = THS_GS_GAME_WORLD;
      } else if (SDL_strcmp(scene_type_str, "main_menu") == 0) {
        comp.type = THS_GS_MAIN_MENU;
      } else {
        TB_CHECK(false, "Deserialization Failure");
      }
    }
  }
  ecs_set_ptr(world->ecs, ent, ThsGameSceneSettings, &comp);
  return true;
}

void ths_destroy_game_state_comp(TbWorld *world, ecs_entity_t ent) {
  ecs_remove(world->ecs, ent, ThsGameSceneSettings);
}

ecs_entity_t ths_register_game_state_comp(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT_DEFINE(ecs, ThsGameSceneDescriptor);
  ECS_COMPONENT_DEFINE(ecs, ThsGameSceneSettings);

  ecs_struct(ecs,
             {
                 .entity = ecs_id(ThsGameSceneDescriptor),
                 .members =
                     {
                         {.name = "scene_type", .type = ecs_id(ecs_string_t)},
                     },
             });

  return ecs_id(ThsGameSceneDescriptor);
}

TB_REGISTER_COMP(ths, game_state)
