#pragma once

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/renderer/scheduler/SurfaceHandler.h>
#include "ArkUINodeRegistry.h"
#include "RNOH/ComponentInstance.h"
#include "RNOH/ComponentInstanceFactory.h"
#include "RNOH/ComponentInstanceRegistry.h"

namespace rnoh {

class XComponentSurface {
 public:
  XComponentSurface(
      std::shared_ptr<facebook::react::Scheduler> scheduler,
      ComponentInstanceRegistry::Shared componentInstanceRegistry,
      ComponentInstanceFactory::Shared const& componentInstanceFactory,
      facebook::react::SurfaceId surfaceId,
      std::string const& appKey);

  XComponentSurface(XComponentSurface const& other) = delete;
  XComponentSurface& operator=(XComponentSurface const& other) = delete;

  XComponentSurface(XComponentSurface&& other) noexcept;
  XComponentSurface& operator=(XComponentSurface&& other) noexcept;

  ~XComponentSurface() noexcept;

  void attachNativeXComponent(OH_NativeXComponent* nativeXComponent);

  void updateConstraints(
      float width,
      float height,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      bool isRTL);
  void start(
      float width,
      float height,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      bool isRTL,
      folly::dynamic const& initialProps,
      std::shared_ptr<facebook::react::LayoutAnimationDriver> const&
          animationDriver);
  void setProps(folly::dynamic const& props);
  void stop();
  void setDisplayMode(facebook::react::DisplayMode displayMode);

 private:
  facebook::react::SurfaceId m_surfaceId;
  std::shared_ptr<facebook::react::Scheduler> m_scheduler;
  OH_NativeXComponent* m_nativeXComponent;
  ComponentInstance::Shared m_rootView;
  ComponentInstanceRegistry::Shared m_componentInstanceRegistry;
  facebook::react::SurfaceHandler m_surfaceHandler;
  std::unique_ptr<TouchEventHandler> m_touchEventHandler;
};

} // namespace rnoh