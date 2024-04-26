#include "XComponentSurface.h"
#include <glog/logging.h>
#include <react/renderer/components/root/RootComponentDescriptor.h>
#include "ArkUINodeRegistry.h"
#include "NativeNodeApi.h"
#include "TouchEventDispatcher.h"

namespace rnoh {

using facebook::react::Scheduler;
using facebook::react::SurfaceHandler;
using facebook::react::SurfaceId;

void maybeAttachRootNode(
    OH_NativeXComponent* nativeXComponent,
    ComponentInstance& rootView) {
  if (nativeXComponent != nullptr) {
    LOG(INFO)
        << "Attaching native root node to nativeXComponent for surface with id: "
        << rootView.getTag();
#ifdef C_API_ARCH
    OH_NativeXComponent_AttachNativeRootNode(
        nativeXComponent,
        rootView.getLocalRootArkUINode().getArkUINodeHandle());
#endif
  }
}

void maybeDetachRootNode(
    OH_NativeXComponent* nativeXComponent,
    ComponentInstance& /* rootView */) {
  if (nativeXComponent != nullptr) {
#ifdef C_API_ARCH
    // NOTE: this is noop, because detaching a destroyed XComponent is
    // incorrect, and we're not notified when it's destroyed. We may want to
    // detach it from `RNSurface` in the future.
#endif
  }
}

class SurfaceTouchEventHandler : public TouchEventHandler {
 private:
  ComponentInstance::Shared m_rootView;
  TouchEventDispatcher m_touchEventDispatcher;

 public:
  SurfaceTouchEventHandler(ComponentInstance::Shared rootView)
      : m_rootView(std::move(rootView)) {
    ArkUINodeRegistry::getInstance().registerTouchHandler(
        &m_rootView->getLocalRootArkUINode(), this);
    NativeNodeApi::getInstance()->registerNodeEvent(
        m_rootView->getLocalRootArkUINode().getArkUINodeHandle(),
        NODE_TOUCH_EVENT,
        NODE_TOUCH_EVENT,
        this);
  }

  SurfaceTouchEventHandler(SurfaceTouchEventHandler const& other) = delete;
  SurfaceTouchEventHandler& operator=(SurfaceTouchEventHandler const& other) =
      delete;

  SurfaceTouchEventHandler(SurfaceTouchEventHandler&& other) noexcept = delete;
  SurfaceTouchEventHandler& operator=(
      SurfaceTouchEventHandler&& other) noexcept = delete;

  ~SurfaceTouchEventHandler() override {
    NativeNodeApi::getInstance()->unregisterNodeEvent(
        m_rootView->getLocalRootArkUINode().getArkUINodeHandle(),
        NODE_TOUCH_EVENT);
    ArkUINodeRegistry::getInstance().unregisterTouchHandler(
        &m_rootView->getLocalRootArkUINode());
  }

  void onTouchEvent(ArkUI_UIInputEvent* event) override {
    m_touchEventDispatcher.dispatchTouchEvent(event, m_rootView);
  }
};

XComponentSurface::XComponentSurface(
    std::shared_ptr<Scheduler> scheduler,
    ComponentInstanceRegistry::Shared componentInstanceRegistry,
    ComponentInstanceFactory::Shared const& componentInstanceFactory,
    SurfaceId surfaceId,
    std::string const& appKey)
    : m_surfaceId(surfaceId),
      m_scheduler(std::move(scheduler)),
      m_nativeXComponent(nullptr),
      m_rootView(nullptr),
      m_componentInstanceRegistry(std::move(componentInstanceRegistry)),
      m_surfaceHandler(SurfaceHandler(appKey, surfaceId)) {
  m_scheduler->registerSurface(m_surfaceHandler);
  m_rootView = componentInstanceFactory->create(
      surfaceId, facebook::react::RootShadowNode::Handle(), "RootView");
  if (m_rootView == nullptr) {
    LOG(ERROR) << "Couldn't create rootInstanceComponent for surface with id: "
               << surfaceId;
    return;
  }
  m_componentInstanceRegistry->insert(m_rootView);
  m_touchEventHandler = std::make_unique<SurfaceTouchEventHandler>(m_rootView);
}

XComponentSurface::XComponentSurface(XComponentSurface&& other) noexcept
    : m_surfaceId(other.m_surfaceId),
      m_scheduler(std::move(other.m_scheduler)),
      m_nativeXComponent(other.m_nativeXComponent),
      m_rootView(std::move(other.m_rootView)),
      m_componentInstanceRegistry(std::move(other.m_componentInstanceRegistry)),
      m_surfaceHandler(std::move(other.m_surfaceHandler)),
      m_touchEventHandler(std::move(other.m_touchEventHandler)) {
  other.m_nativeXComponent = nullptr;
}

XComponentSurface& XComponentSurface::operator=(
    XComponentSurface&& other) noexcept {
  std::swap(m_surfaceId, other.m_surfaceId);
  std::swap(m_scheduler, other.m_scheduler);
  std::swap(m_nativeXComponent, other.m_nativeXComponent);
  std::swap(m_rootView, other.m_rootView);
  std::swap(m_componentInstanceRegistry, other.m_componentInstanceRegistry);
  std::swap(m_surfaceHandler, other.m_surfaceHandler);
  std::swap(m_touchEventHandler, other.m_touchEventHandler);
  return *this;
}

XComponentSurface::~XComponentSurface() noexcept {
  if (m_surfaceHandler.getStatus() == SurfaceHandler::Status::Running) {
    LOG(WARNING) << "Tried to unregister a running surface with id "
                 << m_surfaceId;
    this->stop();
  }
  m_scheduler->unregisterSurface(m_surfaceHandler);
  if (m_componentInstanceRegistry != nullptr && m_rootView != nullptr) {
    // NOTE: we don't detach the view from XComponent here,
    // since the Surface must already be `.stop()`ed before it's `unregistered`
    m_componentInstanceRegistry->deleteByTag(m_rootView->getTag());
  }
}

void XComponentSurface::attachNativeXComponent(
    OH_NativeXComponent* nativeXComponent) {
  if (nativeXComponent == m_nativeXComponent) {
    return;
  }
  maybeDetachRootNode(m_nativeXComponent, *m_rootView);
  m_nativeXComponent = nativeXComponent;
  maybeAttachRootNode(nativeXComponent, *m_rootView);
}

void XComponentSurface::updateConstraints(
    float width,
    float height,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio) {
  auto layoutConstraints = m_surfaceHandler.getLayoutConstraints();
  layoutConstraints.layoutDirection =
      facebook::react::LayoutDirection::LeftToRight;
  layoutConstraints.minimumSize =
      layoutConstraints.maximumSize = {.width = width, .height = height};
  auto layoutContext = m_surfaceHandler.getLayoutContext();
  layoutContext.viewportOffset = {viewportOffsetX, viewportOffsetY};
  layoutContext.pointScaleFactor = pixelRatio;
  m_surfaceHandler.constraintLayout(layoutConstraints, layoutContext);
}

void XComponentSurface::start(
    float width,
    float height,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    folly::dynamic const& initialProps,
    std::shared_ptr<facebook::react::LayoutAnimationDriver> const&
        animationDriver) {
  this->setProps(initialProps);
  this->updateConstraints(
      width, height, viewportOffsetX, viewportOffsetY, pixelRatio);
  m_surfaceHandler.start();
  auto mountingCoordinator = m_surfaceHandler.getMountingCoordinator();
  mountingCoordinator->setMountingOverrideDelegate(animationDriver);
}

void XComponentSurface::setProps(folly::dynamic const& props) {
  m_surfaceHandler.setProps(props);
}

void XComponentSurface::stop() {
  m_surfaceHandler.stop();
}

void XComponentSurface::setDisplayMode(
    facebook::react::DisplayMode displayMode) {
  m_surfaceHandler.setDisplayMode(displayMode);
}

} // namespace rnoh