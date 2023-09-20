#include "boatmovementsystem.h"

#include "inputsystem.h"
#include "meshcomponent.h"
#include "oceancomponent.h"
#include "profiling.h"
#include "sailingcomponents.h"
#include "tbcommon.h"
#include "transformcomponent.h"
#include "visualloggingsystem.h"
#include "world.h"

#include <SDL2/SDL_log.h>

#include <flecs.h>

void boat_movement_update_tick(ecs_iter_t *it) {
  TracyCZoneN(ctx, "Boat Movement System Tick", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  ecs_world_t *ecs = it->world;
  ECS_COMPONENT(ecs, BoatMovementSystem);
  ECS_COMPONENT(ecs, VisualLoggingSystem);
  ECS_COMPONENT(ecs, InputSystem);
  ECS_COMPONENT(ecs, OceanComponent);
  ECS_COMPONENT(ecs, TransformComponent);

  const BoatMovementSystem *sys = ecs_singleton_get(ecs, BoatMovementSystem);
  VisualLoggingSystem *vlog = ecs_singleton_get_mut(ecs, VisualLoggingSystem);
  const InputSystem *input = ecs_singleton_get(ecs, InputSystem);

  ecs_singleton_modified(ecs, VisualLoggingSystem);

  // Find oceans
  // For now we assume only one
  OceanComponent *ocean = NULL;
  TransformComponent *ocean_transform = NULL;
  {
    ecs_iter_t ocean_it = ecs_query_iter(ecs, sys->ocean_query);
    int32_t ocean_count = 0;
    while (ecs_iter_next(&ocean_it)) {

      if (ocean_count == 0 && ocean_it.count == 1) {
        ocean = ecs_field(&ocean_it, OceanComponent, 1);
        ocean_transform = ecs_field(&ocean_it, TransformComponent, 2);
      }

      ocean_count += ocean_it.count;
    }
    TB_CHECK(ocean_count == 1, "Not expecting more than one ocean");
  }

  TransformComponent *transforms = ecs_field(it, TransformComponent, 1);
  HullComponent *hulls = ecs_field(it, HullComponent, 2);

  for (int32_t i = 0; i < it->count; ++i) {
    TransformComponent *transform = &transforms[i];
    HullComponent *hull = &hulls[i];

    TransformComponent *boat_transform =
        tb_transform_get_parent_mut(ecs, transform);

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
    float half_width = hull->width * 0.5f;
    float half_depth = hull->depth * 0.5f;

    Quaternion boat_rot = boat_transform->transform.rotation;
    float3 forward =
        qrotf3(boat_rot, transform_get_forward(&transform->transform));
    float3 right = qrotf3(boat_rot, transform_get_right(&transform->transform));

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
      tb_vlog_location(vlog, (float3){point[0], 10.0f, point[2]}, 0.4f,
                       normf3((float3){point[0], 0, point[1]}));

      OceanSample sample = tb_sample_ocean(ocean, ecs, ocean_transform,
                                           (float2){point[0], point[2]});
      average_sample.pos += sample.pos;
      average_sample.tangent += sample.tangent;
      average_sample.binormal += sample.binormal;
    }
    average_sample.pos /= SAMPLE_COUNT;
    average_sample.tangent /= SAMPLE_COUNT;
    average_sample.tangent = normf3(average_sample.tangent);
    average_sample.binormal /= SAMPLE_COUNT;
    average_sample.binormal = normf3(average_sample.binormal);

    transform->transform.position[1] =
        lerpf(average_sample.pos[1], transform->transform.position[1],
              clampf(it->delta_time, 0.0f, 1.0f));

    float3 normal =
        normf3(crossf3(average_sample.tangent, average_sample.binormal));
    Quaternion rot = mf33_to_quat(
        m44tom33(look_at((float3){0}, average_sample.binormal, normal)));

    transform->transform.rotation = slerp(transform->transform.rotation, rot,
                                          clampf(it->delta_time, 0.0f, 1.0f));
#undef SAMPLE_COUNT

    // Modify boat rotation based on input
    {
      float rotation_alpha = 0.0f;
      bool rotating = false;
      if (input->keyboard.key_A == 1) {
        rotation_alpha = 1.0f;
        rotating = true;
      }
      if (input->keyboard.key_D == 1) {
        rotation_alpha = -1.0f;
        rotating = true;
      }
      if (!rotating && input->controller_count > 0) {
        float a = input->controller_states[0].left_stick[0];
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
        hull->heading_velocity += accel;
      } else if (hull->heading_velocity != 0.0f) {
        hull->heading_velocity -=
            accel_rate * SDL_copysignf(1, hull->heading_velocity);
        if (hull->heading_velocity > -0.01f && hull->heading_velocity < 0.01f) {
          hull->heading_velocity = 0.0f;
        }
      }

      // Clamp rotational velocity
      if (SDL_fabsf(hull->heading_velocity) > 1.0f) {
        hull->heading_velocity =
            1.0f * SDL_copysignf(1, hull->heading_velocity);
      }

      boat_transform->transform.rotation =
          mulq(boat_transform->transform.rotation,
               angle_axis_to_quat(
                   (float4){0, 1, 0, hull->heading_velocity * it->delta_time}));
    }

    // Move boat forward based on angle compared to the wind direction
    {
      // Project forward onto the XZ plane to get the forward we want to use
      // for movement
      float3 mov_forward = transform_get_forward(&boat_transform->transform);
      mov_forward = normf3((float3){mov_forward[0], 0.0f, mov_forward[2]});

      float movement_axis = 0.0f;
      if (input->keyboard.key_W > 0) {
        movement_axis = 1.0f;
      } else if (input->controller_count > 0) {
        const TBGameControllerState *state = &input->controller_states[0];
        movement_axis = clampf(state->left_trigger, -1.0f, 1.0f);
      }

      if (movement_axis == 0) {
        // Try to apply some drag if there's no input
        const float speed_threshold = 0.1f;
        const float drag = 0.1f;
        if (hull->speed > speed_threshold && hull->speed - drag >= 0.0f) {
          hull->speed = drag * -SDL_copysignf(1, hull->speed);
        } else if (hull->speed < SDL_FLT_EPSILON &&
                   hull->speed > -SDL_FLT_EPSILON) {
          hull->speed = 0.0f;
        }
      } else {
        hull->speed += 0.1f * movement_axis;
      }

      float3 velocity = mov_forward * hull->speed;

      // TEMP
      hull->max_speed = 25.0f;
      float speed_sq = hull->max_speed * hull->max_speed;
      if (magsqf3(velocity) > speed_sq) {
        hull->speed = hull->max_speed;
        velocity = normf3(velocity) * hull->max_speed;
      }

      boat_transform->transform.position += velocity * it->delta_time;
    }
  }

  TracyCZoneEnd(ctx);
}

void ths_register_boat_movement_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, BoatMovementSystem);
  ECS_COMPONENT(ecs, TransformComponent);
  ECS_COMPONENT(ecs, OceanComponent);
  ECS_COMPONENT(ecs, HullComponent);

  BoatMovementSystem sys = {
      .tmp_alloc = world->tmp_alloc,
      .ocean_query = ecs_query(ecs, {.filter.terms = {
                                         {.id = ecs_id(OceanComponent)},
                                         {.id = ecs_id(TransformComponent)},
                                     }})};
  ecs_set_ptr(ecs, ecs_id(BoatMovementSystem), BoatMovementSystem, &sys);

  ECS_SYSTEM(ecs, boat_movement_update_tick, EcsOnUpdate, TransformComponent,
             HullComponent)

  ths_register_sailing_components(world);
}

void ths_unregister_boat_movement_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, BoatMovementSystem);
  BoatMovementSystem *sys = ecs_singleton_get_mut(ecs, BoatMovementSystem);
  ecs_query_fini(sys->ocean_query);
  ecs_singleton_remove(ecs, BoatMovementSystem);
}
