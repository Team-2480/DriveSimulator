#pragma once

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "raylib.h"

// Jolt includes
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstdlib>
#include <iostream>

using namespace JPH::literals;

namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
 public:
  virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
                             JPH::ObjectLayer inObject2) const override {
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
};

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint NUM_LAYERS(2);
};  // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
 public:
  BPLayerInterfaceImpl() {
    mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
  }

  virtual uint GetNumBroadPhaseLayers() const override {
    return BroadPhaseLayers::NUM_LAYERS;
  }

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(
      JPH::ObjectLayer inLayer) const override {
    JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
    return mObjectToBroadPhase[inLayer];
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(
      JPH::BroadPhaseLayer inLayer) const override {
    switch ((JPH::BroadPhaseLayer::Type)inLayer) {
      case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
        return "NON_MOVING";
      case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
        return "MOVING";
      default:
        JPH_ASSERT(false);
        return "INVALID";
    }
  }
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

 private:
  JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl
    : public JPH::ObjectVsBroadPhaseLayerFilter {
 public:
  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
                             JPH::BroadPhaseLayer inLayer2) const override {
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
};

// An example contact listener
class MyContactListener : public JPH::ContactListener {
 public:
  // See: ContactListener
  virtual JPH::ValidateResult OnContactValidate(
      const JPH::Body& inBody1, const JPH::Body& inBody2,
      JPH::RVec3Arg inBaseOffset,
      const JPH::CollideShapeResult& inCollisionResult) override {
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
  }

  virtual void OnContactAdded(const JPH::Body& inBody1,
                              const JPH::Body& inBody2,
                              const JPH::ContactManifold& inManifold,
                              JPH::ContactSettings& ioSettings) override {}

  virtual void OnContactPersisted(const JPH::Body& inBody1,
                                  const JPH::Body& inBody2,
                                  const JPH::ContactManifold& inManifold,
                                  JPH::ContactSettings& ioSettings) override {}

  virtual void OnContactRemoved(
      const JPH::SubShapeIDPair& inSubShapePair) override {}
};

class MyBodyActivationListener : public JPH::BodyActivationListener {
 public:
  virtual void OnBodyActivated(const JPH::BodyID& inBodyID,
                               JPH::uint64 inBodyUserData) override {}

  virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID,
                                 JPH::uint64 inBodyUserData) override {}
};

class JoltWrapper {
 public:
  JPH::PhysicsSystem physics_system;
  JPH::TempAllocatorImpl temp_allocator;
  JPH::JobSystemThreadPool job_system;

  const uint max_bodies = 1024 * 10;
  const uint num_body_mutexes = 0;
  const uint max_body_pairs = 1024 * 10;
  const uint max_contact_constraints = 1024 * 10;
  BPLayerInterfaceImpl broad_phase_layer_interface;
  JPH::ObjectVsBroadPhaseLayerFilter object_vs_broadphase_layer_filter;
  ObjectLayerPairFilterImpl object_vs_object_layer_filter;
  MyBodyActivationListener body_activation_listener;
  MyContactListener contact_listener;
  Model convex_model;

  static void init() {
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
  }

  JoltWrapper(Shader shader)
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

    convex_model = LoadModel("../release/convexhull.obj");
    for (size_t i = 0; i < convex_model.materialCount; i++) {
      convex_model.materials[i].shader = shader;
    }

    JPH::TriangleList tri_list;
    int triangle_count = convex_model.meshes[0].vertexCount / 3;

    // Test against all triangles in mesh
    for (size_t mesh = 0; mesh < convex_model.meshCount; mesh++) {
      for (int i = 0; i < triangle_count; i++) {
        Vector3 a, b, c;
        Vector2 u1, u2, u3;
        Vector3* vertdata = (Vector3*)convex_model.meshes[mesh].vertices;
        Vector2* uvdata = (Vector2*)convex_model.meshes[mesh].texcoords;

        if (convex_model.meshes[mesh].indices) {
          a = vertdata[convex_model.meshes[mesh].indices[i * 3 + 0]];
          b = vertdata[convex_model.meshes[mesh].indices[i * 3 + 1]];
          c = vertdata[convex_model.meshes[mesh].indices[i * 3 + 2]];

          // additional uv data
          u1 = uvdata[convex_model.meshes[mesh].indices[i * 3 + 0]];
          u2 = uvdata[convex_model.meshes[mesh].indices[i * 3 + 1]];
          u3 = uvdata[convex_model.meshes[mesh].indices[i * 3 + 2]];
        } else {
          a = vertdata[i * 3 + 0];
          b = vertdata[i * 3 + 1];
          c = vertdata[i * 3 + 2];

          // additional uv data
          u1 = uvdata[i * 3 + 0];
          u2 = uvdata[i * 3 + 1];
          u3 = uvdata[i * 3 + 2];
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

  JPH::BodyInterface& get_interface() {
    return physics_system.GetBodyInterface();
  }
  void update() {
    const float delta_time = 1.0f / 60.0f;

    auto errors =
        physics_system.Update(delta_time, 1, &temp_allocator, &job_system);
  }

  std::vector<JPH::BodyID> balls;

  void make_ball() {
    JPH::BodyCreationSettings sphere_settings(
        new JPH::SphereShape(0.14986f),
        JPH::RVec3((float)(rand() % 10000) / 10000 * 16 - 8.0_r, 2.0_r,
                   (float)(rand() % 10000) / 10000 * 8 - 4.0_r),
        JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    JPH::BodyID sphere_id = get_interface().CreateAndAddBody(
        sphere_settings, JPH::EActivation::Activate);
    get_interface().SetLinearAndAngularVelocity(
        sphere_id, JPH::Vec3(0.0f, 0.0f, 0.0f), JPH::RVec3(0.0_r, 0.0f, 0.0f));
    balls.push_back(sphere_id);
  }

  std::vector<Vector3> get_ball_positions() {
    std::vector<Vector3> ret;
    for (auto ball : balls) {
      JPH::RVec3 position = get_interface().GetCenterOfMassPosition(ball);
      ret.push_back({position.GetX(), position.GetY(), position.GetZ()});
    }
    return ret;
  }
};
