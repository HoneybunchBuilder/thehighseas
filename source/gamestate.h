#pragma once

#include <flecs.h>

typedef enum ThsGameSceneType {
  THS_GS_UNKNOWN = 0,
  THS_GS_MAIN_MENU,
  THS_GS_GAME_WORLD,
} ThsGameSceneType;

typedef struct ThsGameSceneSettings {
  ThsGameSceneType type;
} ThsGameSceneSettings;
extern ECS_COMPONENT_DECLARE(ThsGameSceneSettings);
