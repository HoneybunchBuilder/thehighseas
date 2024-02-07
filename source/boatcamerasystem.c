#include "cameracomponent.h"
#include "inputsystem.h"
#include "tbcommon.h"
#include "transformcomponent.h"
#include "world.h"

#include "sailingcomponents.h"

#include <SDL3/SDL_log.h>
#include <flecs.h>

void boat_camera_update_tick(ecs_iter_t *it) {
  TracyCZoneN(ctx, "Boat Camera Update System", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  tb_auto *ecs = it->world;
  ECS_COMPONENT(ecs, TbInputSystem);

  const tb_auto *input = ecs_singleton_get(ecs, TbInputSystem);

  tb_auto *transforms = ecs_field(it, TbTransformComponent, 1);
  tb_auto *boat_cameras = ecs_field(it, TbBoatCameraComponent, 2);

  for (int32_t i = 0; i < it->count; ++i) {
    // Get parent transform to determine where the parent boat hull is that we
    // want to focus on
    tb_auto entity = it->entities[i];
    tb_auto *transform_comp = &transforms[i];
    tb_auto *boat_cam = &boat_cameras[i];

    tb_auto hull = ecs_get_parent(ecs, entity);
    tb_auto hull_transform = ecs_get_mut(ecs, hull, TbTransformComponent);
    tb_auto hull_pos = hull_transform->transform.position;

    // A target distance of 0 makes no sense; interpret as initialization
    // and set a variety of parameters to whatever is stored on the transform
    tb_auto target_dist = boat_cam->target_dist;
    tb_auto hull_to_camera = boat_cam->target_hull_to_camera;

    bool init = (target_dist == 0.0f);
    if (init) {
      hull_to_camera = tb_normf3(transform_comp->transform.position - hull_pos);
    }

    // Move the boat along the look axis based on mouse wheel input
    {
      float3 pos_hull_diff = transform_comp->transform.position - hull_pos;

      if (init) {
        target_dist = tb_magf3(pos_hull_diff);
      }

      target_dist += input->mouse.wheel[1] * boat_cam->zoom_speed;
      target_dist =
          tb_clampf(target_dist, boat_cam->min_dist, boat_cam->max_dist);
    }

    // Arcball the camera around the boat
    {
      float look_speed = 5.0f;
      float look_yaw = 0.0f;
      float look_pitch = 0.0f;
      if (input->mouse.left || input->mouse.right || input->mouse.middle) {
        float2 look_axis = input->mouse.axis;
        look_yaw = look_axis.x * it->delta_time * look_speed;
        look_pitch = look_axis.y * it->delta_time * look_speed;
      } else if (input->gamepad_count > 0) {
        const TbGameControllerState *ctl_state = &input->gamepad_states[0];
        float2 look_axis = ctl_state->right_stick;
        float deadzone = 0.15f;
        if (look_axis.x > -deadzone && look_axis.x < deadzone) {
          look_axis.x = 0.0f;
        }
        if (look_axis.y > -deadzone && look_axis.y < deadzone) {
          look_axis.y = 0.0f;
        }
        look_yaw = look_axis.x * it->delta_time;
        look_pitch = look_axis.y * it->delta_time;
      }

      tb_auto yaw_quat = tb_angle_axis_to_quat((float4){0, 1, 0, look_yaw});
      hull_to_camera = tb_normf3(tb_qrotf3(yaw_quat, hull_to_camera));
      tb_auto right = tb_normf3(tb_crossf3(TB_UP, hull_to_camera));
      tb_auto pitch_quat = tb_angle_axis_to_quat(tb_f3tof4(right, look_pitch));
      hull_to_camera = tb_normf3(tb_qrotf3(pitch_quat, hull_to_camera));
    }

    boat_cam->target_dist = target_dist;
    boat_cam->target_hull_to_camera = hull_to_camera;

    float3 camera_pos = (hull_to_camera * target_dist);

    // Make sure the camera looks at the hull
    transform_comp->transform =
        tb_look_forward_transform(camera_pos, -hull_to_camera, TB_UP);
    tb_transform_mark_dirty(ecs, entity);
  }

  TracyCZoneEnd(ctx);
}

void ths_register_boat_camera_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, TbBoatCameraComponent);

  ECS_SYSTEM(ecs, boat_camera_update_tick, EcsOnUpdate, TbTransformComponent,
             TbBoatCameraComponent);
}

void ths_unregister_boat_camera_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  (void)ecs;
}

TB_REGISTER_SYS(ths, boat_camera, TB_SYSTEM_NORMAL)
