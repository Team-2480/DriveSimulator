#include "jolt.h"

#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include "Jolt/RegisterTypes.h"
#include "config.h"

void JoltWrapper::init() {
  JPH::RegisterDefaultAllocator();
  JPH::Factory::sInstance = new JPH::Factory();
  JPH::RegisterTypes();
}
void JoltWrapper::free() { JPH::UnregisterTypes(); }

JoltWrapper::JoltWrapper(Shader& shader)
    : temp_allocator(10 * 1024 * 1024),
      job_system(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                 JPH::thread::hardware_concurrency() - 1) {
  physics_system.Init(max_bodies, num_body_mutexes, max_body_pairs,
                      max_contact_constraints, broad_phase_layer_interface,
                      object_vs_broadphase_layer_filter,
                      object_vs_object_layer_filter);

  physics_system.SetBodyActivationListener(&body_activation_listener);

  physics_system.SetContactListener(&contact_listener);

  JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

  convex_model = LoadModel(RELEASE_FOLDER("hull.glb"));

  for (int i = 0; i < convex_model.materialCount; i++) {
    convex_model.materials[i].shader = shader;
  }

  JPH::TriangleList tri_list;

  // Test against all triangles in mesh
  for (int mesh = 0; mesh < convex_model.meshCount; mesh++) {
    int triangle_count = convex_model.meshes[mesh].triangleCount;
    for (int i = 0; i < triangle_count; i++) {
      Vector3 a, b, c;
      Vector3* vertdata = (Vector3*)convex_model.meshes[mesh].vertices;

      if (convex_model.meshes[mesh].indices) {
        a = vertdata[convex_model.meshes[mesh].indices[i * 3 + 0]];
        b = vertdata[convex_model.meshes[mesh].indices[i * 3 + 1]];
        c = vertdata[convex_model.meshes[mesh].indices[i * 3 + 2]];

      } else {
        a = vertdata[i * 3 + 0];
        b = vertdata[i * 3 + 1];
        c = vertdata[i * 3 + 2];
      }
      tri_list.push_back(JPH::Triangle(JPH::Float3{a.x, a.y + 1, a.z},
                                       {b.x, b.y + 1, b.z},
                                       {c.x, c.y + 1, c.z}));
    }
  }

  JPH::MeshShapeSettings floor_shape_settings(tri_list);
  floor_shape_settings.SetEmbedded();

  JPH::BoxShapeSettings::ShapeResult floor_shape_result =
      floor_shape_settings.Create();
  JPH::ShapeRefC floor_shape = floor_shape_result.Get();

  JPH::BodyCreationSettings floor_settings(
      floor_shape, JPH::RVec3(0.0_r, -1.0_r, 0.0_r), JPH::Quat::sIdentity(),
      JPH::EMotionType::Static, Layers::NON_MOVING);

  JPH::Body* floor = body_interface.CreateBody(floor_settings);
  body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
}

void JoltWrapper::update() {
  const float delta_time = 1.0f / 30.0f;

  auto errors =
      physics_system.Update(delta_time, 5, &temp_allocator, &job_system);

  if (errors != JPH::EPhysicsUpdateError::None) {
    // whoops ig
  }
}
void JoltWrapper::make_ball() {
  JPH::EOverrideMassProperties::MassAndInertiaProvided;
  JPH::BodyCreationSettings sphere_settings(
      new JPH::SphereShape(0.15f / 2),
      JPH::RVec3((float)(rand() % 10000) / 10000 * 8 - 4.0_r, 2.0_r,
                 (float)(rand() % 10000) / 10000 * 4 - 2.0_r),
      JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);

  JPH::MassProperties msp;
  msp.mMass = .02;
  msp.ScaleToMass(0.5);

  sphere_settings.mMassPropertiesOverride = msp;
  sphere_settings.mOverrideMassProperties =
      JPH::EOverrideMassProperties::MassAndInertiaProvided;

  JPH::BodyID sphere_id = get_interface().CreateAndAddBody(
      sphere_settings, JPH::EActivation::Activate);
  get_interface().SetLinearAndAngularVelocity(
      sphere_id, JPH::Vec3(0.0f, 0.0f, 0.0f), JPH::RVec3(0.0_r, 0.0f, 0.0f));
  get_interface().SetFriction(sphere_id, 0.3);
  balls.push_back(sphere_id);
}
std::vector<Vector3> JoltWrapper::get_ball_positions() {
  std::vector<Vector3> ret;
  for (auto ball : balls) {
    JPH::RVec3 position = get_interface().GetCenterOfMassPosition(ball);
    ret.push_back({position.GetX(), position.GetY(), position.GetZ()});
  }
  return ret;
}

JPH::BodyInterface& JoltWrapper::get_interface() {
  return physics_system.GetBodyInterface();
}
bool ObjectLayerPairFilterImpl::ShouldCollide(
    JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const {
  switch (inObject1) {
    case Layers::NON_MOVING:
      return inObject2 == Layers::MOVING;
    case Layers::MOVING:
      return true;
    default:
      JPH_ASSERT(false);
      return false;
  }
}
BPLayerInterfaceImpl::BPLayerInterfaceImpl() {
  mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
  mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
}
uint32_t BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
  return BroadPhaseLayers::NUM_LAYERS;
}
JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(
    JPH::ObjectLayer inLayer) const {
  JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
  return mObjectToBroadPhase[inLayer];
}
bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(
    JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
  switch (inLayer1) {
    case Layers::NON_MOVING:
      return inLayer2 == BroadPhaseLayers::MOVING;
    case Layers::MOVING:
      return true;
    default:
      JPH_ASSERT(false);
      return false;
  }
}
JPH::ValidateResult MyContactListener::OnContactValidate(
    const JPH::Body&, const JPH::Body&, JPH::RVec3Arg,
    const JPH::CollideShapeResult&) {
  return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}
