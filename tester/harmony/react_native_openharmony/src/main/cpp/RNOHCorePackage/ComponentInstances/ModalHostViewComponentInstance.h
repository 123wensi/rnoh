#pragma once

#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/ArkUIDialogHandler.h"
#include "RNOH/arkui/ArkUINodeRegistry.h"
#include "RNOH/arkui/StackNode.h"
#include "react/renderer/components/modal/ModalHostViewShadowNode.h"

namespace rnoh {
class ModalHostViewComponentInstance
    : public CppComponentInstance<facebook::react::ModalHostViewShadowNode>,
      public ArkUIDialogDelegate,
      public ArkTSMessageHub::Observer {
 private:
  using ScreenOrientation = facebook::react::ModalHostViewEventEmitter::
      OnOrientationChangeOrientation;
  StackNode m_virtualNode;
  StackNode m_rootStackNode;
  ArkUIDialogHandler m_dialogHandler;
  std::unique_ptr<TouchEventHandler> m_touchHandler;
  std::shared_ptr<ArkTSMessageHandler> m_displaySizeChangeHandler;
  std::optional<ScreenOrientation> m_screenOrientation;

 private:
  void updateDisplaySize(
      DisplayMetrics const& displayMetrics,
      SharedConcreteState const& state);

  void updateSlideTransition(DisplayMetrics const& displayMetrics);

 public:
  ModalHostViewComponentInstance(Context context);

  void onPropsChanged(SharedConcreteProps const& props) override;
  void onStateChanged(SharedConcreteState const& state) override;

  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;

  void finalizeUpdates() override;

  void showDialog();

  void onShow() override;

  void onRequestClose() override;

  StackNode& getLocalRootArkUINode() override;

  // ArkTSMessageHub::Observer
  void onMessageReceived(ArkTSMessage const& message) override;

  friend class ModalHostTouchHandler;
};
} // namespace rnoh