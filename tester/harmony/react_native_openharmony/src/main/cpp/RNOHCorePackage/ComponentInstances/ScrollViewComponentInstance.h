#pragma once
#include <react/renderer/components/scrollview/ScrollViewEventEmitter.h>
#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/ScrollNode.h"
#include "RNOH/arkui/StackNode.h"
#include "RNOHCorePackage/TurboModules/Animated/NativeAnimatedTurboModule.h"

namespace rnoh {

class ScrollViewComponentInstance
    : public CppComponentInstance<facebook::react::ScrollViewShadowNode>,
      public ScrollNodeDelegate {
 private:
  enum ScrollState : int32_t { IDLE, SCROLL, FLING };
  struct ChildTagWithOffset {
    facebook::react::Tag tag;
    facebook::react::Float offset;
  };
  ScrollNode m_scrollNode;
  StackNode m_contentContainerNode;
  StackNode m_scrollContainerNode;
  facebook::react::Size m_contentSize;
  facebook::react::Size m_containerSize;
  ScrollState m_scrollState = IDLE;
  facebook::react::Point m_currentOffset = {0, 0};
  bool m_persistentScrollbar = false;
  long long m_lastScrollDispatchTime = 0;
  bool m_allowNextScrollEvent = false;
  facebook::react::Float m_scrollEventThrottle = 0;
  bool m_isNativeResponderBlocked = false;
  std::weak_ptr<NativeAnimatedTurboModule> m_nativeAnimatedTurboModule{};
  bool m_allowScrollPropagation = false;
  bool m_disableIntervalMomentum = false;
  float m_recentScrollFrameOffset = 0;
  std::vector<facebook::react::Float> m_snapToOffsets = {};
  std::optional<ChildTagWithOffset> m_firstVisibleView = std::nullopt;

  facebook::react::Float getFrictionFromDecelerationRate(
      facebook::react::Float decelerationRate);
  void emitOnScrollEndDragEvent();
  void emitOnMomentumScrollEndEvent();
  void scrollToEnd(bool animated);
  ArkUI_ScrollBarDisplayMode getScrollBarDisplayMode(
      bool horizontal,
      bool persistentScrollBar,
      bool showsVerticalScrollIndicator,
      bool showsHorizontalScrollIndicator);
  void setScrollSnap(
      bool snapToStart,
      bool snapToEnd,
      const std::vector<facebook::react::Float>& snapToOffsets,
      facebook::react::Float snapToInterval,
      facebook::react::ScrollViewSnapToAlignment snapToAlignment);
  bool scrollMovedBySignificantOffset(facebook::react::Point newOffset);
  folly::dynamic getScrollEventPayload(
      facebook::react::ScrollViewMetrics const& scrollViewMetrics);

  void sendEventForNativeAnimations(
      facebook::react::ScrollViewMetrics const& scrollViewMetrics);
  bool isHorizontal(SharedConcreteProps const& props);
  void disableIntervalMomentum();
  std::optional<float> getNextSnapTarget();
  void adjustVisibleContentPosition(
      facebook::react::ScrollViewMaintainVisibleContentPosition const&
          scrollViewMaintainVisibleContentPosition);
  std::optional<ChildTagWithOffset> getFirstVisibleView(
      int32_t minIndexForVisible);

 public:
  ScrollViewComponentInstance(Context context);

  StackNode& getLocalRootArkUINode() override;

  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;

  void setLayout(facebook::react::LayoutMetrics layoutMetrics) override;

  void onStateChanged(SharedConcreteState const& state) override;
  void onPropsChanged(SharedConcreteProps const& props) override;

  void handleCommand(std::string const& commandName, folly::dynamic const& args)
      override;

  // ScrollNodeDelegate implementation
  void onScroll() override;
  void onScrollStart() override;
  void onScrollStop() override;
  float onScrollFrameBegin(float offset, int32_t scrollState) override;

  void finalizeUpdates() override;

  // TouchTarget implementation
  facebook::react::Point computeChildPoint(
      facebook::react::Point const& point,
      TouchTarget::Shared const& child) const override;

  facebook::react::ScrollViewMetrics getScrollViewMetrics();

  bool isHandlingTouches() const override;

 protected:
  void onNativeResponderBlockChange(bool isBlocked) override;

 private:
  void updateStateWithContentOffset(facebook::react::Point contentOffset);
  bool isContentSmallerThanContainer();
  bool isAtEnd(facebook::react::Point currentOffset);
  facebook::react::Point getContentViewOffset() const;
};
} // namespace rnoh