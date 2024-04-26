#include "ModalHostViewComponentInstance.h"

#include <RNOH/Assert.h>
#include <RNOH/arkui/NativeNodeApi.h>
#include <RNOH/arkui/TouchEventDispatcher.h>
#include <glog/logging.h>
#include <react/renderer/components/rncore/Props.h>
#include "RNOH/ArkTSBridge.h"
#include "RNOH/arkui/ArkUINodeRegistry.h"

namespace rnoh {

static constexpr int32_t ANIMATION_DURATION = 300;

class ModalHostTouchHandler : public TouchEventHandler {
 private:
  ModalHostViewComponentInstance* m_rootView;
  TouchEventDispatcher m_touchEventDispatcher;

 public:
  ModalHostTouchHandler(ModalHostTouchHandler const& other) = delete;
  ModalHostTouchHandler& operator=(ModalHostTouchHandler const& other) = delete;
  ModalHostTouchHandler(ModalHostTouchHandler&& other) = delete;
  ModalHostTouchHandler& operator=(ModalHostTouchHandler&& other) = delete;

  ModalHostTouchHandler(ModalHostViewComponentInstance* rootView)
      : m_rootView(rootView) {
    auto* rootNode = &rootView->m_rootStackNode;
    ArkUINodeRegistry::getInstance().registerTouchHandler(rootNode, this);
    NativeNodeApi::getInstance()->registerNodeEvent(
        rootNode->getArkUINodeHandle(),
        NODE_TOUCH_EVENT,
        NODE_TOUCH_EVENT,
        this);
  }

  ~ModalHostTouchHandler() override {
    auto* rootNode = &m_rootView->m_rootStackNode;
    NativeNodeApi::getInstance()->unregisterNodeEvent(
        rootNode->getArkUINodeHandle(), NODE_TOUCH_EVENT);
    ArkUINodeRegistry::getInstance().unregisterTouchHandler(rootNode);
  }

  void onTouchEvent(ArkUI_UIInputEvent* event) override {
    m_touchEventDispatcher.dispatchTouchEvent(
        event, m_rootView->shared_from_this());
  }
};

void ModalHostViewComponentInstance::updateDisplaySize(
    DisplayMetrics const& displayMetrics,
    SharedConcreteState const& state) {
  auto windowMetrics = displayMetrics.windowPhysicalPixels;
  auto screenOrientation = (displayMetrics.windowPhysicalPixels.height >=
                            displayMetrics.windowPhysicalPixels.width)
      ? ScreenOrientation::Portrait
      : ScreenOrientation::Landscape;

  if (m_screenOrientation != screenOrientation) {
    m_screenOrientation = screenOrientation;
    m_eventEmitter->onOrientationChange({.orientation = screenOrientation});
  }
  facebook::react::Size screenSize = {
      .width = windowMetrics.width / windowMetrics.scale,
      .height = windowMetrics.height / windowMetrics.scale};
  state->updateState({screenSize});
  if (m_props != nullptr &&
      m_props->animationType ==
          facebook::react::ModalHostViewAnimationType::Slide) {
    updateSlideTransition(displayMetrics);
  }
}

void ModalHostViewComponentInstance::updateSlideTransition(
    DisplayMetrics const& displayMetrics) {
  auto screenSize = displayMetrics.windowPhysicalPixels;
  m_rootStackNode.setTranslateTransition(
      0, float(screenSize.height / screenSize.scale), ANIMATION_DURATION);
}

ModalHostViewComponentInstance::ModalHostViewComponentInstance(Context context)
    : CppComponentInstance(std::move(context)),
      ArkTSMessageHub::Observer(m_deps->arkTSMessageHub),
      m_touchHandler(std::make_unique<ModalHostTouchHandler>(this)) {
  m_virtualNode.setSize(facebook::react::Size{0, 0});
  m_dialogHandler.setDialogDelegate(this);
  m_rootStackNode.setPosition({0, 0});
}

void ModalHostViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  using AnimationType = facebook::react::ModalHostViewAnimationType;
  CppComponentInstance::onPropsChanged(props);
  if (!props) {
    return;
  }
  if (!m_props || props->animationType != m_props->animationType) {
    if (props->animationType == AnimationType::Slide) {
      m_rootStackNode.resetOpacityTransition();
      auto screenSize = ArkTSBridge::getInstance()->getDisplayMetrics();
      updateSlideTransition(screenSize);
    } else if (props->animationType == AnimationType::Fade) {
      m_rootStackNode.setTranslateTransition(0, 0, 0);
      m_rootStackNode.setOpacityTransition(ANIMATION_DURATION);
    } else {
      m_rootStackNode.setTranslateTransition(0, 0, 0);
      m_rootStackNode.resetOpacityTransition();
    }
  }
}

void ModalHostViewComponentInstance::onStateChanged(
    SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);
  if (!m_state) {
    // set screen size the first time the component is initialized
    auto displayMetrics = ArkTSBridge::getInstance()->getDisplayMetrics();
    updateDisplaySize(displayMetrics, state);
  }
}

void ModalHostViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_rootStackNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void ModalHostViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_rootStackNode.removeChild(childComponentInstance->getLocalRootArkUINode());
};

void ModalHostViewComponentInstance::finalizeUpdates() {
  // only show modal after the screen size has been set and processed by RN
  auto isScreenSizeSet = m_state && m_state->getData().screenSize.height != 0 &&
      m_state->getData().screenSize.width != 0;
  auto shouldShowDialog = !m_dialogHandler.isShow() && isScreenSizeSet;
  if (shouldShowDialog) {
    showDialog();
  }
  CppComponentInstance::finalizeUpdates();
}

void ModalHostViewComponentInstance::showDialog() {
  m_dialogHandler.setContent(m_rootStackNode);
  m_dialogHandler.show();
}

void ModalHostViewComponentInstance::onShow() {
  if (m_eventEmitter != nullptr) {
    m_eventEmitter->onShow({});
  }
}

void ModalHostViewComponentInstance::onRequestClose() {
  if (m_eventEmitter != nullptr) {
    m_eventEmitter->onRequestClose({});
  }
}

StackNode& ModalHostViewComponentInstance::getLocalRootArkUINode() {
  return m_virtualNode;
}

void ModalHostViewComponentInstance::onMessageReceived(
    ArkTSMessage const& message) {
  if (message.name == "WINDOW_SIZE_CHANGE") {
    auto displayMetrics = ArkTSBridge::getInstance()->getDisplayMetrics();
    updateDisplaySize(displayMetrics, m_state);
  }
}

} // namespace rnoh
