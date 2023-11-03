#include "gamestate.h"

#include "assetsystem.h"
#include "tbcommon.h"
#include "tbgltf.h"
#include "world.h"

#include <flecs.h>
#include <json.h>

#define GameStateID "THS_GAMESTATE"

GameSceneSettings deserialize_game_scene_settings(json_object *json) {
  GameSceneSettings comp = {0};
  json_object_object_foreach(json, key, value) {
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
  return comp;
}

bool load_game_state(ecs_world_t *ecs, ecs_entity_t e, const char *source_path,
                     const cgltf_node *node, json_object *extra) {
  ECS_COMPONENT(ecs, GameSceneSettings);
  ECS_TAG(ecs, NoTransform);
  (void)source_path;
  (void)node;

  if (extra) {
    json_object_object_foreach(extra, key, value) {
      if (SDL_strcmp(key, "id") == 0) {
        const char *id_str = json_object_get_string(value);
        if (SDL_strcmp(id_str, GameStateID) == 0) {
          GameSceneSettings comp = deserialize_game_scene_settings(extra);
          ecs_set_ptr(ecs, e, GameSceneSettings, &comp);
          // This entity does not need to exist in space
          ecs_add_id(ecs, e, NoTransform);
          break;
        }
      }
    }
  }

  return true;
}
void free_game_state(ecs_world_t *ecs) {
  ECS_COMPONENT(ecs, GameSceneSettings);
  ecs_set(ecs, ecs_id(GameSceneSettings), GameSceneSettings, {0});
}

void ths_register_game_state_components(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, AssetSystem);
  ECS_COMPONENT(ecs, GameSceneSettings);

  AssetSystem asset = {
      .add_fn = load_game_state,
      .rem_fn = free_game_state,
  };
  ecs_set_ptr(ecs, ecs_id(GameSceneSettings), AssetSystem, &asset);
}
