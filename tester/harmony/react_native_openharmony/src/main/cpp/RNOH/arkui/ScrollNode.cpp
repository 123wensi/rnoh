#include "ScrollNode.h"
#include <glog/logging.h>

static constexpr std::array SCROLL_NODE_EVENT_TYPES{
    NODE_SCROLL_EVENT_ON_SCROLL,
    NODE_SCROLL_EVENT_ON_SCROLL_START,
    NODE_SCROLL_EVENT_ON_SCROLL_STOP,
    NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN};

namespace rnoh {
ScrollNode::ScrollNode()
    : ArkUINode(NativeNodeApi::getInstance()->createNode(
          ArkUI_NodeType::ARKUI_NODE_SCROLL)),
      m_childArkUINodeHandle(nullptr),
      m_scrollNodeDelegate(nullptr) {
  for (auto eventType : SCROLL_NODE_EVENT_TYPES) {
    maybeThrow(NativeNodeApi::getInstance()->registerNodeEvent(
        m_nodeHandle, eventType, eventType, this));
  }
}

ScrollNode::~ScrollNode() {
  for (auto eventType : SCROLL_NODE_EVENT_TYPES) {
    NativeNodeApi::getInstance()->unregisterNodeEvent(m_nodeHandle, eventType);
  }
}

void ScrollNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  if (eventType == ArkUI_NodeEventType::NODE_SCROLL_EVENT_ON_SCROLL) {
    if (m_scrollNodeDelegate != nullptr) {
      m_scrollNodeDelegate->onScroll();
    }
  } else if (
      eventType == ArkUI_NodeEventType::NODE_SCROLL_EVENT_ON_SCROLL_START) {
    if (m_scrollNodeDelegate != nullptr) {
      m_scrollNodeDelegate->onScrollStart();
    }
  } else if (
      eventType == ArkUI_NodeEventType::NODE_SCROLL_EVENT_ON_SCROLL_STOP) {
    if (m_scrollNodeDelegate != nullptr) {
      m_scrollNodeDelegate->onScrollStop();
    }
  } else if (
      eventType ==
      ArkUI_NodeEventType::NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN) {
    if (m_scrollNodeDelegate != nullptr) {
      auto remainingOffset = m_scrollNodeDelegate->onScrollFrameBegin(
          eventArgs[0].f32, eventArgs[1].i32);
      eventArgs[0].f32 = remainingOffset;
    }
  }
}

facebook::react::Point ScrollNode::getScrollOffset() const {
  auto item = NativeNodeApi::getInstance()->getAttribute(
      m_nodeHandle, NODE_SCROLL_OFFSET);
  facebook::react::Float x = item->value[0].i32;
  facebook::react::Float y = item->value[1].i32;
  return Point{x, y};
}

void ScrollNode::insertChild(ArkUINode& child) {
  if (m_childArkUINodeHandle != nullptr) {
    LOG(FATAL) << "ScrollNode can only have one child";
  }
  maybeThrow(NativeNodeApi::getInstance()->addChild(
      m_nodeHandle, child.getArkUINodeHandle()));
  m_childArkUINodeHandle = child.getArkUINodeHandle();
}

void ScrollNode::removeChild(ArkUINode& child) {
  if (m_childArkUINodeHandle == nullptr) {
    LOG(FATAL) << "Tried to remove child before inserting one.";
  }
  NativeNodeApi::getInstance()->removeChild(
      m_nodeHandle, m_childArkUINodeHandle);
  m_childArkUINodeHandle = nullptr;
}

void ScrollNode::setScrollNodeDelegate(ScrollNodeDelegate* scrollNodeDelegate) {
  m_scrollNodeDelegate = scrollNodeDelegate;
}

void ScrollNode::setNestedScroll(ArkUI_ScrollNestedMode scrollNestedMode) {
  std::array<ArkUI_NumberValue, 2> value{
      {{.i32 = scrollNestedMode}, {.i32 = scrollNestedMode}}};
  ArkUI_AttributeItem item = {
      value.data(), sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_NESTED_SCROLL, &item));
}

ScrollNode& ScrollNode::setHorizontal(bool horizontal) {
  auto scrollDirection = horizontal
      ? ArkUI_ScrollDirection::ARKUI_SCROLL_DIRECTION_HORIZONTAL
      : ArkUI_ScrollDirection::ARKUI_SCROLL_DIRECTION_VERTICAL;
  ArkUI_NumberValue value[] = {{.i32 = scrollDirection}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_SCROLL_DIRECTION, &item));
  return *this;
}

ScrollNode& ScrollNode::setEnableScrollInteraction(
    bool enableScrollInteraction) {
  ArkUI_NumberValue value[] = {{.i32 = enableScrollInteraction}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_ENABLE_SCROLL_INTERACTION, &item));
  return *this;
}

ScrollNode& ScrollNode::setFriction(float friction) {
  ArkUI_NumberValue value[] = {{.f32 = friction}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_FRICTION, &item));
  return *this;
}

ScrollNode& ScrollNode::setEdgeEffect(bool bounces, bool alwaysBounce) {
  ArkUI_EdgeEffect edgeEffect = bounces
      ? ArkUI_EdgeEffect::ARKUI_EDGE_EFFECT_SPRING
      : ArkUI_EdgeEffect::ARKUI_EDGE_EFFECT_NONE;
  ArkUI_NumberValue value[] = {{.i32 = edgeEffect}, {.i32 = alwaysBounce}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_EDGE_EFFECT, &item));
  return *this;
}
void ScrollNode::scrollTo(float x, float y, bool animated) {
  ArkUI_AttributeItem item;
  ArkUI_NumberValue value[] = {
      {.f32 = x},
      {.f32 = y},
      {.i32 = animated ? 1000 : 0},
      {.i32 = ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH},
      {.i32 = 0}};
  item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_OFFSET, &item));
}
ScrollNode& ScrollNode::setScrollBarDisplayMode(
    ArkUI_ScrollBarDisplayMode scrollBarDisplayMode) {
  ArkUI_NumberValue value[] = {{.i32 = scrollBarDisplayMode}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_BAR_DISPLAY_MODE, &item));
  return *this;
}
ScrollNode& ScrollNode::setScrollBarColor(uint32_t scrollBarColor) {
  ArkUI_NumberValue value[] = {{.u32 = scrollBarColor}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_BAR_COLOR, &item));
  return *this;
}

ScrollNode& ScrollNode::setScrollSnap(
    ArkUI_ScrollSnapAlign scrollSnapAlign,
    bool snapToStart,
    bool snapToEnd,
    const std::vector<facebook::react::Float>& snapPoints) {
  std::vector<ArkUI_NumberValue> value = {
      {.i32 = scrollSnapAlign}, {.i32 = snapToStart}, {.i32 = snapToEnd}};
  for (auto snapPoint : snapPoints) {
    value.push_back({.f32 = static_cast<float>(snapPoint)});
  }
  ArkUI_AttributeItem item = {value.data(), static_cast<int32_t>(value.size())};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_SNAP, &item));
  return *this;
}

ScrollNode& ScrollNode::setEnablePaging(bool enablePaging) {
  ArkUI_NumberValue value[] = {{.i32 = enablePaging}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_SCROLL_ENABLE_PAGING, &item));
  return *this;
}

ScrollNode& ScrollNode::setCenterContent(bool centerContent) {
  ArkUI_Alignment alignmentMode = centerContent ? 
    ArkUI_Alignment::ARKUI_ALIGNMENT_CENTER : ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START;
  std::array<ArkUI_NumberValue, 1> value = {{{.i32 = alignmentMode}}};
  ArkUI_AttributeItem item = {value.data(), value.size()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_STACK_ALIGN_CONTENT, &item));
  return *this;
}
} // namespace rnoh