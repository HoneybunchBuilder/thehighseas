#include "boatcamerasystem.h"

#include "cameracomponent.h"
#include "inputsystem.h"
#include "profiling.h"
#include "sailingcomponents.h"
#include "transformcomponent.h"
#include "world.h"

#include <SDL2/SDL_log.h>

#include <flecs.h>

void boat_camera_update_tick(ecs_iter_t *it) {
  TracyCZoneN(ctx, "Boat Camera Update System", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  ecs_world_t *ecs = it->world;
  ECS_COMPONENT(ecs, InputSystem);

  const InputSystem *input = ecs_singleton_get(ecs, InputSystem);

  TransformComponent *transforms = ecs_field(it, TransformComponent, 1);
  BoatCameraComponent *boat_cameras = ecs_field(it, BoatCameraComponent, 2);

  for (int32_t i = 0; i < it->count; ++i) {
    // Get parent transform to determine where the parent boat hull is that we
    // want to focus on
    TransformComponent *transform_comp = &transforms[i];
    BoatCameraComponent *boat_cam = &boat_cameras[i];

    const TransformComponent *hull_transform_comp =
        tb_transform_get_parent(ecs, transform_comp);
    float3 hull_pos = hull_transform_comp->transform.position;

    boat_cam->target_center = hull_pos;

    // A target distance of 0 makes no sense; interpret as initialization
    // and set a variety of parameters to whatever is stored on the transform
    float target_dist = boat_cam->target_dist;
    float3 target_center = boat_cam->target_center;
    float3 hull_to_camera = boat_cam->target_hull_to_camera;

    bool init = target_dist == 0.0f;
    if (init) {
      target_center = hull_pos;
      hull_to_camera = normf3(transform_comp->transform.position - hull_pos);
    }

    // Move the boat along the look axis based on mouse wheel input
    {
      float3 pos_hull_diff = transform_comp->transform.position - hull_pos;

      if (init) {
        target_dist = magf3(pos_hull_diff);
      }

      target_dist += input->mouse.wheel[1] * boat_cam->zoom_speed;
      target_dist = clampf(target_dist, boat_cam->min_dist, boat_cam->max_dist);
    }

    // Arcball the camera around the boat
    {

      float look_yaw = 0.0f;
      float look_pitch = 0.0f;
      if (input->mouse.left || input->mouse.right || input->mouse.middle) {
        float2 look_axis = input->mouse.axis;
        look_yaw = look_axis[0] * it->delta_time * 5;
        look_pitch = look_axis[1] * it->delta_time * 5;
      } else if (input->controller_count > 0) {
        const TBGameControllerState *ctl_state = &input->controller_states[0];
        float2 look_axis = ctl_state->right_stick;
        float deadzone = 0.15f;
        if (look_axis[0] > -deadzone && look_axis[0] < deadzone) {
          look_axis[0] = 0.0f;
        }
        if (look_axis[1] > -deadzone && look_axis[1] < deadzone) {
          look_axis[1] = 0.0f;
        }
        look_yaw = look_axis[0] * it->delta_time;
        look_pitch = look_axis[1] * it->delta_time;
      }

      Quaternion yaw_quat = angle_axis_to_quat((float4){0, 1, 0, look_yaw});
      hull_to_camera = normf3(qrotf3(yaw_quat, hull_to_camera));
      float3 right = normf3(crossf3(TB_UP, hull_to_camera));
      Quaternion pitch_quat = angle_axis_to_quat(f3tof4(right, look_pitch));
      hull_to_camera = normf3(qrotf3(pitch_quat, hull_to_camera));
    }

    boat_cam->target_dist = target_dist;
    boat_cam->target_center = target_center;
    boat_cam->target_hull_to_camera = hull_to_camera;

    transform_comp->transform.position =
        target_center + (hull_to_camera * target_dist);

    // Make sure the camera looks at the hull
    transform_comp->transform.rotation = mf33_to_quat(
        m44tom33(look_at(transform_comp->transform.position, hull_pos, TB_UP)));
  }

  TracyCZoneEnd(ctx);
}

void ths_register_boat_camera_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, BoatCameraSystem);
  ECS_COMPONENT(ecs, TransformComponent);
  ECS_COMPONENT(ecs, BoatCameraComponent);

  BoatCameraSystem sys = {
      .tmp_alloc = world->tmp_alloc,
  };
  ecs_set_ptr(ecs, ecs_id(BoatCameraSystem), BoatCameraSystem, &sys);

  ECS_SYSTEM(ecs, boat_camera_update_tick, EcsOnUpdate, TransformComponent,
             BoatCameraComponent)
}

void ths_unregister_boat_camera_sys(TbWorld *world) {
  ecs_world_t *ecs = world->ecs;
  ECS_COMPONENT(ecs, BoatCameraSystem);
  BoatCameraSystem *sys = ecs_singleton_get_mut(ecs, BoatCameraSystem);
  *sys = (BoatCameraSystem){0};
  ecs_singleton_remove(ecs, BoatCameraSystem);
}
