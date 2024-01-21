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

#include <SDL3/SDL_log.h>

#include <flecs.h>

void boat_movement_update_tick(ecs_iter_t *it) {
  TracyCZoneN(ctx, "Boat Movement System Tick", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  ecs_world_t *ecs = it->world;
  ECS_COMPONENT(ecs, ThsBoatMovementSystem);
  ECS_COMPONENT(ecs, TbVisualLoggingSystem);
  ECS_COMPONENT(ecs, TbInputSystem);
  ECS_COMPONENT(ecs, TbOceanComponent);
  ECS_COMPONENT(ecs, TbTransformComponent);

  const tb_auto *sys = ecs_singleton_get(ecs, ThsBoatMovementSystem);
  tb_auto *vlog = ecs_singleton_get_mut(ecs, TbVisualLoggingSystem);
  const tb_auto *input = ecs_singleton_get(ecs, TbInputSystem);

  ecs_singleton_modified(ecs, TbVisualLoggingSystem);

  // Find oceans
  // For now we assume only one
  TbOceanComponent *ocean = NULL;
  ecs_entity_t ocean_ent = 0;
  {
    ecs_iter_t ocean_it = ecs_query_iter(ecs, sys->ocean_query);
    int32_t ocean_count = 0;
    while (ecs_iter_next(&ocean_it)) {

      if (ocean_count == 0 && ocean_it.count == 1) {
        ocean = ecs_field(&ocean_it, TbOceanComponent, 1);
        ocean_ent = ocean_it.entities[0];
      }

      ocean_count += ocean_it.count;
    }
    TB_CHECK(ocean_count == 1, "Not expecting more than one ocean");
  }

  tb_auto *transforms = ecs_field(it, TbTransformComponent, 1);
  tb_auto *hulls = ecs_field(it, ThsHullComponent, 2);

  for (int32_t i = 0; i < it->count; ++i) {
    tb_auto *transform = &transforms[i];
    tb_auto *hull = &hulls[i];

    tb_auto boat = ecs_get_parent(ecs, it->entities[i]);
    tb_auto boat_transform = ecs_get_mut(ecs, boat, TbTransformComponent);

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

    TbQuaternion boat_rot = boat_transform->transform.rotation;
    float3 forward =
        tb_qrotf3(boat_rot, tb_transform_get_forward(&transform->transform));
    float3 right =
        tb_qrotf3(boat_rot, tb_transform_get_right(&transform->transform));

    const float3 sample_points[SAMPLE_COUNT] = {
        hull_pos,
        hull_pos + (forward * half_depth), // bow
        hull_pos - (right * half_width),   // left center
        hull_pos + (right * half_width),   // right center
        hull_pos - (right * half_width) - (forward * half_depth), // left stern
        hull_pos + (right * half_width) - (forward * half_depth), // right stern
    };
    TbOceanSample average_sample = {.pos = {0}};
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i) {
      const float3 point = sample_points[i];
      tb_vlog_location(vlog, tb_f3(point.x, 10.0f, point.z), 0.4f,
                       tb_normf3(tb_f3(point.x, 0, point.z)));

      TbOceanSample sample = tb_sample_ocean(ocean, ecs, ocean_ent, point.xz);
      average_sample.pos += sample.pos;
      average_sample.tangent += sample.tangent;
      average_sample.binormal += sample.binormal;
    }
    average_sample.pos /= SAMPLE_COUNT;
    average_sample.tangent /= SAMPLE_COUNT;
    average_sample.tangent = tb_normf3(average_sample.tangent);
    average_sample.binormal /= SAMPLE_COUNT;
    average_sample.binormal = tb_normf3(average_sample.binormal);

    transform->transform.position[1] =
        tb_lerpf(average_sample.pos[1], transform->transform.position[1],
                 tb_clampf(it->delta_time, 0.0f, 1.0f));

    float3 normal =
        tb_normf3(tb_crossf3(average_sample.tangent, average_sample.binormal));
    TbQuaternion rot =
        tb_look_at_quat((float3){0}, average_sample.binormal, normal);

    transform->transform.rotation =
        tb_slerp(transform->transform.rotation, rot,
                 tb_clampf(it->delta_time, 0.0f, 1.0f));
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
      if (!rotating && input->gamepad_count > 0) {
        float a = -input->gamepad_states[0].left_stick.x;
        float deadzone = 0.15f;
        if (a > -deadzone && a < deadzone) {
          a = 0.0f;
        }
        rotation_alpha = tb_clampf(a, -1.0f, 1.0f);
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
          tb_mulq(boat_transform->transform.rotation,
                  tb_angle_axis_to_quat((float4){
                      0, 1, 0, hull->heading_velocity * it->delta_time}));
      tb_transform_mark_dirty(ecs, boat);
    }

    // Move boat forward based on angle compared to the wind direction
    {
      // Project forward onto the XZ plane to get the forward we want to use
      // for movement
      float3 mov_forward = tb_transform_get_forward(&boat_transform->transform);
      mov_forward = tb_normf3((float3){mov_forward.x, 0.0f, mov_forward.z});

      float movement_axis = 0.0f;
      if (input->keyboard.key_W > 0) {
        movement_axis = 1.0f;
      } else if (input->gamepad_count > 0) {
        const TbGameControllerState *state = &input->gamepad_states[0];
        movement_axis = tb_clampf(state->left_trigger, -1.0f, 1.0f);
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
      if (tb_magsqf3(velocity) > speed_sq) {
        hull->speed = hull->max_speed;
        velocity = tb_normf3(velocity) * hull->max_speed;
      }

      boat_transform->transform.position += velocity * it->delta_time;
      tb_transform_mark_dirty(ecs, boat);
    }
  }

  TracyCZoneEnd(ctx);
}

void ths_register_boat_movement_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, ThsBoatMovementSystem);
  ECS_COMPONENT(ecs, TbTransformComponent);
  ECS_COMPONENT(ecs, TbOceanComponent);
  ECS_COMPONENT(ecs, ThsHullComponent);

  ThsBoatMovementSystem sys = {
      .tmp_alloc = world->tmp_alloc,
      .ocean_query = ecs_query(ecs, {.filter.terms = {
                                         {.id = ecs_id(TbOceanComponent)},
                                     }})};
  ecs_set_ptr(ecs, ecs_id(ThsBoatMovementSystem), ThsBoatMovementSystem, &sys);

  ECS_SYSTEM(ecs, boat_movement_update_tick, EcsOnUpdate, TbTransformComponent,
             ThsHullComponent)

  ths_register_sailing_components(world);
}

void ths_unregister_boat_movement_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, ThsBoatMovementSystem);
  ThsBoatMovementSystem *sys =
      ecs_singleton_get_mut(ecs, ThsBoatMovementSystem);
  ecs_query_fini(sys->ocean_query);
  ecs_singleton_remove(ecs, ThsBoatMovementSystem);
}
