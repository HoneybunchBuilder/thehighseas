#include "mainmenu.h"

#include "gamestate.h"
#include "imguisystem.h"
#include "profiling.h"
#include "tbcommon.h"
#include "tbimgui.h"
#include "world.h"

#include <flecs.h>

ECS_SYSTEM_DECLARE(main_menu_tick);

void main_menu_tick(ecs_iter_t *it) {
  ecs_world_t *ecs = it->world;
  ECS_COMPONENT(ecs, GameSceneSettings);
  ECS_COMPONENT(ecs, ImGuiSystem);

  GameSceneSettings *gss = ecs_field(it, GameSceneSettings, 1);
  TB_CHECK(it->count <= 1, "More game state objects than expected");
  // Do nothing if we're not in the main menu
  if (it->count == 0 || gss->type != THS_GS_MAIN_MENU) {
    ecs_enable(ecs, ecs_id(main_menu_tick), false);
    return;
  }

  ImGuiSystem *ui = ecs_singleton_get_mut(ecs, ImGuiSystem);
  TB_CHECK(ui, "Unexpectedly missing UI system");

  if (ui->context_count == 0) {
    return;
  }

  {
    TracyCZoneNC(ctx, "Core UI System Tick", TracyCategoryColorUI, true);
    ImGuiContext *imgui = ui->contexts[0].context;
    igSetCurrentContext(imgui);
    const ImGuiIO *io = &imgui->IO;

    igSetNextWindowPos(
        (ImVec2){io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.5f},
        ImGuiCond_Always, (ImVec2){0.5f, 0.5f});
    if (igBegin("The High Seas", NULL,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_AlwaysAutoResize)) {
      const ImVec2 size = (ImVec2){80, 22};
      {
        igBeginGroup();
        igText("%s", "The High Seas");
        igEndGroup();
      }
      igNewLine();

      // Nudge this whole group over just a little bit
      igSetCursorPosX(igGetCursorPosX() + 5);
      {
        igBeginGroup();
        if (igButton("Continue", size)) {
          // Load the latest available save if possible
        }
        igNewLine();
        if (igButton("New Game", size)) {
          // Fade to back and begin new game
        }
        igNewLine();
        if (igButton("Load Game", size)) {
          // Show load game dialog
        }
        igNewLine();
        if (igButton("Settings", size)) {
          // Show settings dialog
        }
        igNewLine();
        if (igButton("Exit", size)) {
          exit(0);
        }
        igEndGroup();
      }

      igNewLine();

      igEnd();
    }

    TracyCZoneEnd(ctx);
  }
}

void ths_register_main_menu_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, GameSceneSettings);

  ths_register_game_state_components(world);

  ECS_SYSTEM_DEFINE(ecs, main_menu_tick, EcsOnUpdate, GameSceneSettings);
}

void ths_unregister_main_menu_sys(TbWorld *world) {
  // Nothing to do... for now
  (void)world;
}
