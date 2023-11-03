#pragma once

typedef enum GameSceneType {
  THS_GS_UNKNOWN = 0,
  THS_GS_MAIN_MENU,
  THS_GS_GAME_WORLD,
} GameSceneType;

typedef struct GameSceneSettings {
  GameSceneType type;
} GameSceneSettings;

typedef struct TbWorld TbWorld;

void ths_register_game_state_components(TbWorld *world);
