#pragma once

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyInterface.h"
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

using namespace JPH::literals;

namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
 public:
  virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
                             JPH::ObjectLayer inObject2) const override;
};

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS(2);
};  // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
 public:
  BPLayerInterfaceImpl();

  virtual uint32_t GetNumBroadPhaseLayers() const override;

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(
      JPH::ObjectLayer inLayer) const override;

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
                             JPH::BroadPhaseLayer inLayer2) const override;
};

// An example contact listener
class MyContactListener : public JPH::ContactListener {
 public:
  // See: ContactListener
  virtual JPH::ValidateResult OnContactValidate(
      const JPH::Body&, const JPH::Body&, JPH::RVec3Arg,
      const JPH::CollideShapeResult&) override;

  virtual void OnContactAdded(const JPH::Body&, const JPH::Body&,
                              const JPH::ContactManifold&,
                              JPH::ContactSettings&) override {}

  virtual void OnContactPersisted(const JPH::Body&, const JPH::Body&,
                                  const JPH::ContactManifold&,
                                  JPH::ContactSettings&) override {}

  virtual void OnContactRemoved(const JPH::SubShapeIDPair&) override {}
};

class MyBodyActivationListener : public JPH::BodyActivationListener {
 public:
  virtual void OnBodyActivated(const JPH::BodyID&, JPH::uint64) override {}

  virtual void OnBodyDeactivated(const JPH::BodyID&, JPH::uint64) override {}
};

class JoltWrapper final {
 public:
  JPH::PhysicsSystem physics_system;
  JPH::TempAllocatorImpl temp_allocator;
  JPH::JobSystemThreadPool job_system;

  const uint32_t max_bodies = 1000;
  const uint32_t num_body_mutexes = 0;
  const uint32_t max_body_pairs = 1000;
  const uint32_t max_contact_constraints = 1000;
  BPLayerInterfaceImpl broad_phase_layer_interface;
  JPH::ObjectVsBroadPhaseLayerFilter object_vs_broadphase_layer_filter;
  ObjectLayerPairFilterImpl object_vs_object_layer_filter;
  MyBodyActivationListener body_activation_listener;
  MyContactListener contact_listener;

  Model convex_model;

  static void init();
  static void free();

  JoltWrapper(Shader& shader);
  ~JoltWrapper() { UnloadModel(convex_model); }

  JPH::BodyInterface& get_interface();
  void update();

  std::vector<JPH::BodyID> balls;

  void make_ball();

  std::vector<Vector3> get_ball_positions();
};
