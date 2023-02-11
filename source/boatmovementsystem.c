#include "boatmovementsystem.h"

#include "inputcomponent.h"
#include "meshcomponent.h"
#include "oceancomponent.h"
#include "profiling.h"
#include "sailingcomponents.h"
#include "transformcomponent.h"
#include "visualloggingsystem.h"
#include "world.h"

bool create_boat_movement_system(BoatMovementSystem *self,
                                 const BoatMovementSystemDescriptor *desc,
                                 uint32_t system_dep_count,
                                 System *const *system_deps) {
  VisualLoggingSystem *vlog =
      tb_get_system(system_deps, system_dep_count, VisualLoggingSystem);
  *self = (BoatMovementSystem){
      .tmp_alloc = desc->tmp_alloc,
      .vlog = vlog,
  };
  return true;
}

void destroy_boat_movement_system(BoatMovementSystem *self) {
  *self = (BoatMovementSystem){0};
}

void tick_boat_movement_system(BoatMovementSystem *self,
                               const SystemInput *input, SystemOutput *output,
                               float delta_seconds) {
  TracyCZoneN(ctx, "Boat Movement System Tick", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  EntityId *hull_entities = tb_get_column_entity_ids(input, 1);
  EntityId *ocean_entities = tb_get_column_entity_ids(input, 3);
  uint32_t mov_entity_count = tb_get_column_component_count(input, 0);
  uint32_t hull_entity_count = tb_get_column_component_count(input, 1);
  if (mov_entity_count == 0 || mov_entity_count != hull_entity_count) {
    TracyCZoneEnd(ctx);
    return;
  }

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
    const HullComponent *hull_comp =
        tb_get_component(hull_store, entity_idx, HullComponent);

    // Take four samples
    // One at the port, two at the stern
    // one port, one starboard
    // one in the middle forward
    //    *    |
    //   / \   |
    //  /   \  |
    //  * * *  |
    //  |   |  |
    //  |   |  |
    //  *___*  |
    //         |

#define SAMPLE_COUNT 5
    const float3 min = hull_comp->child_mesh_aabb.min;
    const float3 max = hull_comp->child_mesh_aabb.max;

    const float2 sample_points[SAMPLE_COUNT] = {
        {hull_pos[0] + max[0], hull_pos[2]},          // Bow
        {hull_pos[0] + min[0], hull_pos[2] + min[2]}, // Port Stern
        {hull_pos[0] + min[0], hull_pos[2] + max[2]}, // Starboard Stern
        {hull_pos[0], hull_pos[2] + min[2]},          // Port
        {hull_pos[0], hull_pos[2] + max[2]},          // Starboard
    };
    OceanSample average_sample = {.pos = {0}};
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i) {
      const float2 point = sample_points[i];
      tb_vlog_location(self->vlog, (float3){point[0], 10.0f, point[1]}, 0.4f,
                       normf3((float3){point[0], 0, point[1]}));

      OceanSample sample = tb_sample_ocean(ocean, out_ocean_trans, point);
      average_sample.pos += sample.pos;
      average_sample.tangent += sample.tangent;
      average_sample.binormal += sample.binormal;
    }
    average_sample.pos /= SAMPLE_COUNT;
    average_sample.tangent /= SAMPLE_COUNT;
    average_sample.tangent = normf3(average_sample.tangent);
    average_sample.binormal /= SAMPLE_COUNT;
    average_sample.binormal = normf3(average_sample.binormal);

    hull_transform->transform.position[1] = average_sample.pos[1];

    float3 normal =
        normf3(crossf3(average_sample.binormal, average_sample.tangent));
    Quaternion slerped_rot = slerp(
        hull_transform->transform.rotation,
        quat_from_axes(average_sample.tangent, average_sample.binormal, normal),
        clampf(delta_seconds, 0.0f, 1.0f));
    hull_transform->transform.rotation = slerped_rot;
#undef SAMPLE_COUNT
  }

  // TODO: Handle input
  (void)input_comp;

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
      .system_dep_count = 1,
      .system_deps[0] = VisualLoggingSystemId,
      .create = tb_create_boat_movement_system,
      .destroy = tb_destroy_boat_movement_system,
      .tick = tb_tick_boat_movement_system,
  };
}
