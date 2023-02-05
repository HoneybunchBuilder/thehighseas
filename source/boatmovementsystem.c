#include "boatmovementsystem.h"

#include "inputcomponent.h"
#include "profiling.h"
#include "sailingcomponents.h"
#include "transformcomponent.h"
#include "world.h"

bool create_boat_movement_system(BoatMovementSystem *self,
                                 const BoatMovementSystemDescriptor *desc,
                                 uint32_t system_dep_count,
                                 System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *self = (BoatMovementSystem){
      .tmp_alloc = desc->tmp_alloc,
  };
  return true;
}

void destroy_boat_movement_system(BoatMovementSystem *self) {
  *self = (BoatMovementSystem){0};
}

void tick_boat_movement_system(BoatMovementSystem *self,
                               const SystemInput *input, SystemOutput *output,
                               float delta_seconds) {
  (void)self;
  (void)input;
  (void)output;
  (void)delta_seconds;
  TracyCZoneN(ctx, "Boat Movement System Tick", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  TracyCZoneEnd(ctx);
}

TB_DEFINE_SYSTEM(boat_movement, BoatMovementSystem,
                 BoatMovementSystemDescriptor)

void tb_boat_movement_system_descriptor(
    SystemDescriptor *desc, const BoatMovementSystemDescriptor *mov_desc) {
  *desc = (SystemDescriptor){
      .name = "BoatMovement",
      .size = sizeof(BoatMovementSystem),
      .id = BoatMovementSystemId,
      .desc = (InternalDescriptor)mov_desc,
      .dep_count = 2,
      .deps[0] = {1, {TransformComponentId, BoatMovementComponentId}},
      .deps[1] = {1, {InputComponentId}},
      .create = tb_create_boat_movement_system,
      .destroy = tb_destroy_boat_movement_system,
      .tick = tb_tick_boat_movement_system,
  };
}
