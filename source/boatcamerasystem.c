#include "boatcamerasystem.h"

#include "cameracomponent.h"
#include "inputcomponent.h"
#include "profiling.h"
#include "sailingcomponents.h"
#include "transformcomponent.h"
#include "world.h"

bool create_boat_camera_system(BoatCameraSystem *self,
                               const BoatCameraSystemDescriptor *desc,
                               uint32_t system_dep_count,
                               System *const *system_deps) {
  (void)system_dep_count;
  (void)system_deps;
  *self = (BoatCameraSystem){
      .tmp_alloc = desc->tmp_alloc,
  };
  return true;
}

void destroy_boat_camera_system(BoatCameraSystem *self) {
  *self = (BoatCameraSystem){0};
}

void tick_boat_camera_system(BoatCameraSystem *self, const SystemInput *input,
                             SystemOutput *output, float delta_seconds) {
  TracyCZoneN(ctx, "Boat Camera System Tick", true);
  TracyCZoneColor(ctx, TracyCategoryColorGame);

  (void)self;
  (void)input;
  (void)output;
  (void)delta_seconds;

  EntityId *entities = tb_get_column_entity_ids(input, 0);
  uint32_t entity_count = tb_get_column_component_count(input, 0);
  if (entity_count == 0) {
    TracyCZoneEnd(ctx);
    return;
  }

  const PackedComponentStore *transform_store =
      tb_get_column_check_id(input, 0, 0, TransformComponentId);
  const PackedComponentStore *boat_cam_store =
      tb_get_column_check_id(input, 0, 1, BoatCameraComponentId);

  const PackedComponentStore *input_store =
      tb_get_column_check_id(input, 1, 0, InputComponentId);

  const InputComponent *input_comp =
      tb_get_component(input_store, 0, InputComponent);

  // Copy the boat camera component for output
  BoatCameraComponent *out_boat_cams =
      tb_alloc_nm_tp(self->tmp_alloc, entity_count, BoatCameraComponent);
  SDL_memcpy(out_boat_cams, boat_cam_store->components,
             entity_count * sizeof(BoatCameraComponent));

  // Make a copy of the transform input as the output
  TransformComponent *out_transforms =
      tb_alloc_nm_tp(self->tmp_alloc, entity_count, TransformComponent);
  SDL_memcpy(out_transforms, transform_store->components,
             entity_count * sizeof(TransformComponent));

  for (uint32_t i = 0; i < entity_count; ++i) {
    // Get parent transform to determine where the parent boat hull is that we
    // want to focus on
    TransformComponent *transform_comp = &out_transforms[i];
    BoatCameraComponent *boat_cam = &out_boat_cams[i];

    TransformComponent *hull_transform_comp =
        tb_transform_get_parent(transform_comp);
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

      target_dist += input_comp->mouse.wheel[1] * boat_cam->zoom_speed;
      target_dist = clampf(target_dist, boat_cam->min_dist, boat_cam->max_dist);
    }

    // Arcball the camera around the boat
    {
      float2 look_axis = input_comp->mouse.axis;

      float look_yaw = 0.0f;
      float look_pitch = 0.0f;
      if (input_comp->mouse.left || input_comp->mouse.right ||
          input_comp->mouse.middle) {
        look_yaw = look_axis[0] * delta_seconds * 5;
        look_pitch = look_axis[1] * delta_seconds * 5;
      };

      Quaternion yaw_quat = angle_axis_to_quat((float4){0, 1, 0, look_yaw});
      hull_to_camera = normf3(qrotf3(yaw_quat, hull_to_camera));
      float3 right = normf3(crossf3((float3){0, 1, 0}, hull_to_camera));
      Quaternion pitch_quat = angle_axis_to_quat(f3tof4(right, look_pitch));
      hull_to_camera = normf3(qrotf3(pitch_quat, hull_to_camera));
    }

    boat_cam->target_dist = target_dist;
    boat_cam->target_center = target_center;
    boat_cam->target_hull_to_camera = hull_to_camera;

    transform_comp->transform.position =
        target_center + (hull_to_camera * target_dist);

    // Make sure the camera looks at the hull
    transform_comp->transform.rotation =
        look_at_quat(hull_pos, transform_comp->transform.position,
                     (float3){0.0f, 1.0f, 0.0f});
  }

  (void)input_store;

  // Report output
  output->set_count = 2;
  output->write_sets[0] = (SystemWriteSet){
      .id = BoatCameraComponentId,
      .count = entity_count,
      .components = (uint8_t *)out_boat_cams,
      .entities = entities,
  };
  output->write_sets[1] = (SystemWriteSet){
      .id = TransformComponentId,
      .count = entity_count,
      .components = (uint8_t *)out_transforms,
      .entities = entities,
  };

  TracyCZoneEnd(ctx);
}

TB_DEFINE_SYSTEM(boat_camera, BoatCameraSystem, BoatCameraSystemDescriptor)

void tb_boat_camera_system_descriptor(
    SystemDescriptor *desc, const BoatCameraSystemDescriptor *cam_desc) {
  *desc = (SystemDescriptor){
      .name = "BoatCamera",
      .size = sizeof(BoatCameraSystem),
      .id = BoatCameraSystemId,
      .desc = (InternalDescriptor)cam_desc,
      .dep_count = 2,
      .deps[0] = {3,
                  {TransformComponentId, BoatCameraComponentId,
                   CameraComponentId}},
      .deps[1] = {1, {InputComponentId}},
      .create = tb_create_boat_camera_system,
      .destroy = tb_destroy_boat_camera_system,
      .tick = tb_tick_boat_camera_system,
  };
}
