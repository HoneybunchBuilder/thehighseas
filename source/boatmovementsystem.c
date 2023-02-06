#include "boatmovementsystem.h"

#include "inputcomponent.h"
#include "oceancomponent.h"
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
  (void)input;
  (void)output;
  (void)delta_seconds;
  TracyCZoneN(ctx, "Boat Movement System Tick", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  EntityId *mov_entities = tb_get_column_entity_ids(input, 0);
  EntityId *hull_entities = tb_get_column_entity_ids(input, 1);
  EntityId *ocean_entities = tb_get_column_entity_ids(input, 3);
  uint32_t mov_entity_count = tb_get_column_component_count(input, 0);
  uint32_t hull_entity_count = tb_get_column_component_count(input, 1);
  if (mov_entity_count == 0 || mov_entity_count != hull_entity_count) {
    TracyCZoneEnd(ctx);
    return;
  }

  const PackedComponentStore *base_trans_store =
      tb_get_column_check_id(input, 0, 0, TransformComponentId);
  const PackedComponentStore *boat_mov_store =
      tb_get_column_check_id(input, 0, 1, BoatMovementComponentId);

  const PackedComponentStore *hull_trans_store =
      tb_get_column_check_id(input, 1, 0, TransformComponentId);
  const PackedComponentStore *hull_store =
      tb_get_column_check_id(input, 1, 1, HullComponentId);

  const PackedComponentStore *input_store =
      tb_get_column_check_id(input, 2, 0, InputComponentId);
  const InputComponent *input_comp =
      tb_get_component(input_store, 0, InputComponent);

  const PackedComponentStore *ocean_trans_store =
      tb_get_column_check_id(input, 3, 0, TransformComponentId);
  const PackedComponentStore *ocean_store =
      tb_get_column_check_id(input, 3, 1, OceanComponentId);

  const OceanComponent *ocean =
      tb_get_component(ocean_store, 0, OceanComponent);

  tb_make_out_copy(out_ocean_trans, self->tmp_alloc, ocean_trans_store, 1,
                   TransformComponent);
  tb_make_out_copy(out_hull_trans, self->tmp_alloc, hull_trans_store,
                   mov_entity_count, TransformComponent);

  for (uint32_t entity_idx = 0; entity_idx < mov_entity_count; ++entity_idx) {
    TransformComponent *hull_transform = &out_hull_trans[entity_idx];
    float3 hull_pos = hull_transform->transform.position;

    OceanSample sample = tb_sample_ocean(ocean, out_ocean_trans,
                                         (float2){hull_pos[0], hull_pos[2]});
    hull_transform->transform.position[1] = sample.pos[1];
    hull_transform->dirty = true;
    hull_pos = hull_transform->transform.position;
    float3 normal = normf3(crossf3(sample.tangent, sample.tangent));
  }

  // Write output
  output->set_count = 2;
  output->write_sets[0] = (SystemWriteSet){
      .entities = hull_entities,
      .count = hull_entity_count,
      .id = TransformComponentId,
      .components = (uint8_t *)out_hull_trans,
  };
  output->write_sets[1] = (SystemWriteSet){
      .entities = ocean_entities,
      .count = 1,
      .id = TransformComponentId,
      .components = (uint8_t *)out_ocean_trans,
  };

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
      .dep_count = 4,
      .deps[0] = {2, {TransformComponentId, BoatMovementComponentId}},
      .deps[1] = {2, {TransformComponentId, HullComponentId}},
      .deps[2] = {1, {InputComponentId}},
      .deps[3] = {2, {TransformComponentId, OceanComponentId}},
      .create = tb_create_boat_movement_system,
      .destroy = tb_destroy_boat_movement_system,
      .tick = tb_tick_boat_movement_system,
  };
}
