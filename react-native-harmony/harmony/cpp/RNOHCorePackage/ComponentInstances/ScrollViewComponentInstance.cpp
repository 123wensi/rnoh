#include "ScrollViewComponentInstance.h"
#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>
#include <react/renderer/components/scrollview/ScrollViewState.h>
#include <react/renderer/core/ConcreteState.h>
#include <cmath>
#include "PullToRefreshViewComponentInstance.h"
#include "conversions.h"

namespace rnoh {

ScrollViewComponentInstance::ScrollViewComponentInstance(Context context)
    : CppComponentInstance(std::move(context)) {
  m_scrollContainerNode.insertChild(m_scrollNode, 0);
  m_scrollNode.insertChild(m_contentContainerNode);
  // NOTE: perhaps this needs to take rtl into account?
  m_scrollNode.setAlignment(ARKUI_ALIGNMENT_TOP_START);
  m_scrollNode.setScrollNodeDelegate(this);
}

StackNode& ScrollViewComponentInstance::getLocalRootArkUINode() {
  return m_scrollContainerNode;
}

void ScrollViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_contentContainerNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void ScrollViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_contentContainerNode.removeChild(
      childComponentInstance->getLocalRootArkUINode());
}

void ScrollViewComponentInstance::setLayout(
    facebook::react::LayoutMetrics layoutMetrics) {
  m_scrollContainerNode.setSize(layoutMetrics.frame.size);
  m_scrollNode.setSize(layoutMetrics.frame.size);
  m_layoutMetrics = layoutMetrics;
  m_containerSize = layoutMetrics.frame.size;
}

void rnoh::ScrollViewComponentInstance::onStateChanged(
    SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);
  auto stateData = state->getData();
  m_contentContainerNode.setSize(stateData.getContentSize());
  m_contentSize = stateData.getContentSize();
}

void rnoh::ScrollViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  CppComponentInstance::onPropsChanged(props);
  auto horizontal = props->alwaysBounceHorizontal ||
      m_contentSize.width > m_containerSize.width;
  if (props->rawProps.count("persistentScrollbar") > 0) {
    m_persistentScrollbar = props->rawProps["persistentScrollbar"].asBool();
  }
  m_scrollEventThrottle = props->scrollEventThrottle;
  m_scrollNode.setHorizontal(horizontal)
      .setEnableScrollInteraction(
          !m_isNativeResponderBlocked && props->scrollEnabled)
      .setFriction(getFrictionFromDecelerationRate(props->decelerationRate))
      .setEdgeEffect(
          props->bounces,
          horizontal ? props->alwaysBounceHorizontal
                     : props->alwaysBounceVertical)
      .setScrollBarDisplayMode(getScrollBarDisplayMode(
          horizontal,
          m_persistentScrollbar,
          props->showsVerticalScrollIndicator,
          props->showsHorizontalScrollIndicator))
      .setScrollBarColor(
          props->indicatorStyle ==
                  facebook::react::ScrollViewIndicatorStyle::White
              ? 0xFFFFFFFF
              : 0xFF000000)
      .setEnablePaging(props->pagingEnabled);

  setScrollSnap(
      props->snapToStart,
      props->snapToEnd,
      props->snapToOffsets,
      props->snapToInterval,
      props->snapToAlignment);

  auto borderMetrics = props->resolveBorderMetrics(m_layoutMetrics);
  m_contentContainerNode.setMargin(
      -borderMetrics.borderWidths.left,
      -borderMetrics.borderWidths.top,
      0.f,
      0.f);
}

void ScrollViewComponentInstance::handleCommand(
    std::string const& commandName,
    folly::dynamic const& args) {
  if (commandName == "scrollTo") {
    m_scrollNode.scrollTo(
        args[0].asDouble(), args[1].asDouble(), args[2].asBool());
  } else if (commandName == "scrollToEnd") {
    scrollToEnd(args[0].asBool());
  }
}

void rnoh::ScrollViewComponentInstance::setNativeResponderBlocked(
    bool blocked) {
  m_isNativeResponderBlocked = blocked;
  if (blocked) {
    m_scrollNode.setEnableScrollInteraction(false);
  } else {
    m_scrollNode.setEnableScrollInteraction(m_props->scrollEnabled);
  }
}

facebook::react::Point rnoh::ScrollViewComponentInstance::computeChildPoint(
    facebook::react::Point const& point,
    TouchTarget::Shared const& child) const {
  auto offset = m_scrollNode.getScrollOffset();
  return CppComponentInstance::computeChildPoint(point + offset, child);
}

void rnoh::ScrollViewComponentInstance::updateStateWithContentOffset(
    facebook::react::Point contentOffset) {
  if (!m_state) {
    return;
  }
  m_state->updateState([contentOffset](auto const& stateData) {
    auto newData = stateData;
    newData.contentOffset = contentOffset;
    return std::make_shared<
        facebook::react::ScrollViewShadowNode::ConcreteState::Data const>(
        newData);
  });
}

facebook::react::ScrollViewMetrics
ScrollViewComponentInstance::getScrollViewMetrics() {
  auto scrollViewMetrics = facebook::react::ScrollViewMetrics();
  scrollViewMetrics.responderIgnoreScroll = true;
  scrollViewMetrics.zoomScale = 1;
  scrollViewMetrics.contentSize = m_contentSize;
  scrollViewMetrics.contentOffset = m_scrollNode.getScrollOffset();
  scrollViewMetrics.containerSize = m_containerSize;
  return scrollViewMetrics;
}

bool ScrollViewComponentInstance::isHandlingTouches() const {
  return m_scrollState != IDLE;
}

void ScrollViewComponentInstance::onScroll() {
  auto scrollViewMetrics = getScrollViewMetrics();
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now().time_since_epoch())
                 .count();
  if (m_allowNextScrollEvent ||
      (m_scrollEventThrottle < now - m_lastScrollDispatchTime &&
       scrollMovedBySignificantOffset(scrollViewMetrics.contentOffset))) {
    m_lastScrollDispatchTime = now;
    VLOG(2) << "onScroll (contentOffset: " << scrollViewMetrics.contentOffset.x
            << ", " << scrollViewMetrics.contentOffset.y
            << "; contentSize: " << scrollViewMetrics.contentSize.width << ", "
            << scrollViewMetrics.contentSize.height
            << "; containerSize: " << scrollViewMetrics.containerSize.width
            << ", " << scrollViewMetrics.containerSize.height << ")";
    m_eventEmitter->onScroll(scrollViewMetrics);
    sendEventForNativeAnimations(scrollViewMetrics);
    m_currentOffset = scrollViewMetrics.contentOffset;
  };
}

void ScrollViewComponentInstance::onScrollStart() {
  m_allowNextScrollEvent = false;
}

void ScrollViewComponentInstance::onScrollStop() {
  m_allowNextScrollEvent = true;

  if (m_scrollState == ScrollState::FLING) {
    emitOnMomentumScrollEndEvent();
  } else if (m_scrollState == ScrollState::SCROLL) {
    emitOnScrollEndDragEvent();
  }
  m_scrollState = ScrollState::IDLE;
}

float ScrollViewComponentInstance::onScrollFrameBegin(
    float offset,
    int32_t scrollState) {
  auto newScrollState = static_cast<ScrollState>(scrollState);
  if (m_scrollState != newScrollState) {
    if (m_scrollState == ScrollState::SCROLL) {
      emitOnScrollEndDragEvent();
    } else if (m_scrollState == ScrollState::FLING) {
      emitOnMomentumScrollEndEvent();
    }
    auto scrollViewMetrics = getScrollViewMetrics();
    if (scrollState == ScrollState::SCROLL) {
      m_eventEmitter->onScrollBeginDrag(scrollViewMetrics);
    } else if (scrollState == ScrollState::FLING) {
      m_eventEmitter->onMomentumScrollBegin(scrollViewMetrics);
    }
  }
  m_scrollState = newScrollState;
  return offset;
}

void ScrollViewComponentInstance::emitOnScrollEndDragEvent() {
  auto scrollViewMetrics = getScrollViewMetrics();
  m_eventEmitter->onScrollEndDrag(scrollViewMetrics);
  updateStateWithContentOffset(scrollViewMetrics.contentOffset);
}
void ScrollViewComponentInstance::emitOnMomentumScrollEndEvent() {
  auto scrollViewMetrics = getScrollViewMetrics();
  m_eventEmitter->onMomentumScrollEnd(scrollViewMetrics);
  updateStateWithContentOffset(scrollViewMetrics.contentOffset);
}

facebook::react::Float
ScrollViewComponentInstance::getFrictionFromDecelerationRate(
    facebook::react::Float decelerationRate) {
  // default deceleration rate and friction values differ between ArkUI and RN
  // so we adapt the decelerationRate accordingly to resemble iOS behaviour
  // iOS's UIScrollView supports only two values of decelerationRate
  // called 'normal' and 'fast' and maps all other to the nearest of those two
  facebook::react::Float IOS_NORMAL = 0.998;
  facebook::react::Float IOS_FAST = 0.99;
  facebook::react::Float ARKUI_FAST = 2;
  facebook::react::Float ARKUI_NORMAL = 0.6;
  if (decelerationRate < (IOS_NORMAL + IOS_FAST) / 2) {
    return ARKUI_FAST;
  } else {
    return ARKUI_NORMAL;
  }
}

void ScrollViewComponentInstance::scrollToEnd(bool animated) {
  auto horizontal = m_props->alwaysBounceHorizontal ||
      m_contentSize.width > m_containerSize.width;
  auto x = horizontal ? m_contentSize.width : 0.0;
  auto y = horizontal ? 0.0 : m_contentSize.height;
  m_scrollNode.scrollTo(x, y, animated);
}

ArkUI_ScrollBarDisplayMode ScrollViewComponentInstance::getScrollBarDisplayMode(
    bool horizontal,
    bool persistentScrollBar,
    bool showsVerticalScrollIndicator,
    bool showsHorizontalScrollIndicator) {
  if (horizontal && !showsHorizontalScrollIndicator ||
      !horizontal && !showsVerticalScrollIndicator) {
    return ArkUI_ScrollBarDisplayMode::ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF;
  }
  return persistentScrollBar
      ? ArkUI_ScrollBarDisplayMode::ARKUI_SCROLL_BAR_DISPLAY_MODE_ON
      : ArkUI_ScrollBarDisplayMode::ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO;
}

void ScrollViewComponentInstance::setScrollSnap(
    bool snapToStart,
    bool snapToEnd,
    const std::vector<facebook::react::Float>& snapToOffsets,
    facebook::react::Float snapToInterval,
    facebook::react::ScrollViewSnapToAlignment snapToAlignment) {
  if (!snapToOffsets.empty()) {
    m_scrollNode.setScrollSnap(
        ArkUI_ScrollSnapAlign::ARKUI_SCROLL_SNAP_ALIGN_START,
        snapToStart,
        snapToEnd,
        snapToOffsets);
  }
  if (snapToInterval > 0) {
    const std::vector<facebook::react::Float> snapPoints = {snapToInterval};
    m_scrollNode.setScrollSnap(
        getArkUI_ScrollSnapAlign(snapToAlignment),
        snapToStart,
        snapToEnd,
        snapPoints);
  }
}
bool ScrollViewComponentInstance::scrollMovedBySignificantOffset(
    facebook::react::Point newOffset) {
  return std::abs(newOffset.x - m_currentOffset.x) >= 1 ||
      std::abs(newOffset.y - m_currentOffset.y) >= 1;
}

void ScrollViewComponentInstance::finalizeUpdates() {
  ComponentInstance::finalizeUpdates();

  // when parent isn't refresh node, set the position
  auto parent = this->getParent().lock();
  bool isRefresh =
      std::dynamic_pointer_cast<const PullToRefreshViewComponentInstance>(
          this->getParent().lock()) != nullptr;
  if (parent && !isRefresh) {
    this->getLocalRootArkUINode().setPosition(m_layoutMetrics.frame.origin);
  }
}

folly::dynamic ScrollViewComponentInstance::getScrollEventPayload(
    facebook::react::ScrollViewMetrics const& scrollViewMetrics) {
  using folly::dynamic;

  dynamic contentSize =
      dynamic::object("width", scrollViewMetrics.contentSize.width)(
          "height", scrollViewMetrics.contentSize.height);
  dynamic contentOffset =
      dynamic::object("x", scrollViewMetrics.contentOffset.x)(
          "y", scrollViewMetrics.contentOffset.y);
  dynamic contentInset =
      dynamic::object("left", scrollViewMetrics.contentInset.left)(
          "top", scrollViewMetrics.contentInset.top)(
          "right", scrollViewMetrics.contentInset.right)(
          "bottom", scrollViewMetrics.contentInset.bottom);
  dynamic containerSize =
      dynamic::object("width", scrollViewMetrics.containerSize.width)(
          "height", scrollViewMetrics.containerSize.height);
  dynamic payload = dynamic::object("contentSize", contentSize)(
      "contentOffset", contentOffset)("contentInset", contentInset)(
      "containerSize", containerSize)("zoomScale", scrollViewMetrics.zoomScale)(
      "responderIgnoreScroll", scrollViewMetrics.responderIgnoreScroll);
  return payload;
}

void rnoh::ScrollViewComponentInstance::sendEventForNativeAnimations(
    facebook::react::ScrollViewMetrics const& scrollViewMetrics) {
  auto nativeAnimatedTurboModule = m_nativeAnimatedTurboModule.lock();
  if (nativeAnimatedTurboModule == nullptr) {
    auto instance = m_deps->rnInstance.lock();
    if (instance == nullptr) {
      return;
    }
    nativeAnimatedTurboModule =
        instance->getTurboModule<NativeAnimatedTurboModule>(
            "NativeAnimatedTurboModule");
    m_nativeAnimatedTurboModule = nativeAnimatedTurboModule;
  }
  if (nativeAnimatedTurboModule != nullptr) {
    nativeAnimatedTurboModule->handleComponentEvent(
        m_tag, "onScroll", getScrollEventPayload(scrollViewMetrics));
  }
}

} // namespace rnoh
