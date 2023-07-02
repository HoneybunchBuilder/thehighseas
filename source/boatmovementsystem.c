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
  tb_make_out_copy(out_hulls, self->tmp_alloc, hull_store, mov_entity_count,
                   HullComponent);
  // Every hull has a parent boat transform which is what we want to move
  EntityId *boat_entities =
      tb_alloc_nm_tp(self->tmp_alloc, hull_entity_count, EntityId);
  TransformComponent *out_boat_trans =
      tb_alloc_nm_tp(self->tmp_alloc, hull_entity_count, TransformComponent);

  // Calculate wind direction

  // TODO: look this up from component(s) in the world
  float wind_angle = 0.9f;
  Quaternion wind_rot = angle_axis_to_quat((f3tof4(TB_UP, wind_angle)));
  float3 wind_dir = -qrotf3(wind_rot, TB_RIGHT);

  for (uint32_t entity_idx = 0; entity_idx < mov_entity_count; ++entity_idx) {
    TransformComponent *hull_transform = &out_hull_trans[entity_idx];
    HullComponent *hull_comp = &out_hulls[entity_idx];

    out_boat_trans[entity_idx] = *tb_transform_get_parent(hull_transform);
    TransformComponent *boat_transform = &out_boat_trans[entity_idx];
    boat_entities[entity_idx] = hull_transform->parent;

    float3 hull_pos = boat_transform->transform.position;

    // Take six samples
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

#define SAMPLE_COUNT 6
    float half_width = hull_comp->width * 0.5f;
    float half_depth = hull_comp->depth * 0.5f;

    Quaternion boat_rot = boat_transform->transform.rotation;
    float3 forward =
        qrotf3(boat_rot, transform_get_forward(&hull_transform->transform));
    float3 right =
        qrotf3(boat_rot, transform_get_right(&hull_transform->transform));

    const float3 sample_points[SAMPLE_COUNT] = {
        hull_pos,
        hull_pos + (forward * half_depth), // bow
        hull_pos - (right * half_width),   // left center
        hull_pos + (right * half_width),   // right center
        hull_pos - (right * half_width) - (forward * half_depth), // left stern
        hull_pos + (right * half_width) - (forward * half_depth), // right stern
    };
    OceanSample average_sample = {.pos = {0}};
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i) {
      const float3 point = sample_points[i];
      tb_vlog_location(self->vlog, (float3){point[0], 10.0f, point[2]}, 0.4f,
                       normf3((float3){point[0], 0, point[1]}));

      OceanSample sample =
          tb_sample_ocean(ocean, out_ocean_trans, (float2){point[0], point[2]});
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
        normf3(crossf3(average_sample.tangent, average_sample.binormal));
    Quaternion rot = mf33_to_quat(
        m44tom33(look_at((float3){0}, average_sample.binormal, normal)));

    hull_transform->transform.rotation =
        slerp(hull_transform->transform.rotation, rot,
              clampf(delta_seconds, 0.0f, 1.0f));
#undef SAMPLE_COUNT

    // Modify boat rotation based on input
    {
      float rotation_alpha = 0.0f;
      bool rotating = false;
      if (input_comp->keyboard.key_A == 1) {
        rotation_alpha = 1.0f;
        rotating = true;
      }
      if (input_comp->keyboard.key_D == 1) {
        rotation_alpha = -1.0f;
        rotating = true;
      }
      if (!rotating && input_comp->controller_count > 0) {
        float a = -input_comp->controller_states[0].left_stick[0];
        float deadzone = 0.15f;
        if (a > -deadzone && a < deadzone) {
          a = 0.0f;
        }
        rotation_alpha = clampf(a, -1.0f, 1.0f);
        if (rotation_alpha != 0.0f) {
          rotating = true;
        }
      }

      const float accel_rate = 0.1f;
      if (rotating) {
        const float accel = accel_rate * rotation_alpha;
        hull_comp->heading_velocity += accel;
      } else if (hull_comp->heading_velocity != 0.0f) {
        hull_comp->heading_velocity -=
            accel_rate * SDL_copysignf(1, hull_comp->heading_velocity);
        if (hull_comp->heading_velocity > -0.01f &&
            hull_comp->heading_velocity < 0.01f) {
          hull_comp->heading_velocity = 0.0f;
        }
      }

      // Clamp rotational velocity
      if (SDL_fabsf(hull_comp->heading_velocity) > 1.0f) {
        hull_comp->heading_velocity =
            1.0f * SDL_copysignf(1, hull_comp->heading_velocity);
      }

      boat_transform->transform.rotation =
          mulq(boat_transform->transform.rotation,
               angle_axis_to_quat((float4){
                   0, 1, 0, hull_comp->heading_velocity * delta_seconds}));
    }

    // Move boat forward based on angle compared to the wind direction
    {
      // Project forward onto the XZ plane to get the forward we want to use
      // for movement
      float3 mov_forward = transform_get_forward(&boat_transform->transform);
      mov_forward = normf3((float3){mov_forward[0], 0.0f, mov_forward[2]});

      float movement_axis = 0.0f;
      if (input_comp->keyboard.key_W > 0) {
        movement_axis = 1.0f;
      } else if (input_comp->controller_count > 0) {
        const TBGameControllerState *state = &input_comp->controller_states[0];
        movement_axis = clampf(state->left_trigger, -1.0f, 1.0f);
      }

      if (movement_axis == 0) {
        // Try to apply some drag if there's no input
        const float speed_threshold = 0.1f;
        const float drag = 0.1f;
        if (hull_comp->speed > speed_threshold &&
            hull_comp->speed - drag >= 0.0f) {
          hull_comp->speed = drag * -SDL_copysignf(1, hull_comp->speed);
        } else if (hull_comp->speed < SDL_FLT_EPSILON &&
                   hull_comp->speed > -SDL_FLT_EPSILON) {
          hull_comp->speed = 0.0f;
        }
      } else {
        // Dot product between the boat heading and the wind direction to
        // determine acceleration
        float wind_alpha = dotf3(mov_forward, -wind_dir);
        // Apply acceleration to velocity and then clamp based on max speed
        float acceleration = lerpf(wind_alpha, 0.0f, 0.5f);
        hull_comp->speed += 0.1f * movement_axis;
      }

      float3 velocity = mov_forward * hull_comp->speed;

      // TEMP
      hull_comp->max_speed = 25.0f;
      float speed_sq = hull_comp->max_speed * hull_comp->max_speed;
      if (magsqf3(velocity) > speed_sq) {
        hull_comp->speed = hull_comp->max_speed;
        velocity = normf3(velocity) * hull_comp->max_speed;
      }

      boat_transform->transform.position += velocity * delta_seconds;
    }
  }

  // Write output
  output->set_count = 4;
  output->write_sets[0] = (SystemWriteSet){
      .entities = hull_entities,
      .count = hull_entity_count,
      .id = TransformComponentId,
      .components = (uint8_t *)out_hull_trans,
  };
  output->write_sets[1] = (SystemWriteSet){
      .entities = hull_entities,
      .count = hull_entity_count,
      .id = HullComponentId,
      .components = (uint8_t *)out_hulls,
  };
  output->write_sets[2] = (SystemWriteSet){
      .entities = boat_entities,
      .count = hull_entity_count,
      .id = TransformComponentId,
      .components = (uint8_t *)out_boat_trans,
  };
  output->write_sets[3] = (SystemWriteSet){
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
