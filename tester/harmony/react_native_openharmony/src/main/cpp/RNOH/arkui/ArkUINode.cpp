#include "ArkUINode.h"
#include <bits/alltypes.h>
#include <algorithm>
#include "ArkUINodeRegistry.h"
#include "NativeNodeApi.h"
#include "conversions.h"

namespace rnoh {

ArkUINode::ArkUINode(ArkUI_NodeHandle nodeHandle) : m_nodeHandle(nodeHandle) {
  ArkUINodeRegistry::getInstance().registerNode(this);
}

ArkUINode::ArkUINode(ArkUINode&& other) noexcept
    : m_nodeHandle(std::move(other.m_nodeHandle)) {
  other.m_nodeHandle = nullptr;
}

ArkUINode& ArkUINode::operator=(ArkUINode&& other) noexcept {
  std::swap(m_nodeHandle, other.m_nodeHandle);
  return *this;
}

ArkUI_NodeHandle ArkUINode::getArkUINodeHandle() {
  return m_nodeHandle;
}

void ArkUINode::markDirty() {
  // TODO: maybe this can be passed as an arg here,
  // and Component Instance can decide which flag to set in each mutation
  NativeNodeApi::getInstance()->markDirty(
      getArkUINodeHandle(), ArkUI_NodeDirtyFlag::NODE_NEED_RENDER);
  NativeNodeApi::getInstance()->markDirty(
      getArkUINodeHandle(), ArkUI_NodeDirtyFlag::NODE_NEED_LAYOUT);
  NativeNodeApi::getInstance()->markDirty(
      getArkUINodeHandle(), ArkUI_NodeDirtyFlag::NODE_NEED_MEASURE);
}

ArkUINode& ArkUINode::setPosition(facebook::react::Point const& position) {
  ArkUI_NumberValue value[] = {
      static_cast<float>(position.x), static_cast<float>(position.y)};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_POSITION, &item));
  return *this;
}

ArkUINode& ArkUINode::setSize(facebook::react::Size const& size) {
  ArkUI_NumberValue widthValue[] = {static_cast<float>(size.width)};
  ArkUI_AttributeItem widthItem = {
      widthValue, sizeof(widthValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_WIDTH, &widthItem));

  ArkUI_NumberValue heightValue[] = {static_cast<float>(size.height)};
  ArkUI_AttributeItem heightItem = {
      heightValue, sizeof(heightValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_HEIGHT, &heightItem));
  return *this;
}

ArkUINode& ArkUINode::setBorderWidth(
    facebook::react::BorderWidths const& borderWidths) {
  facebook::react::BorderWidths borderWidth;
  borderWidth.top = std::max(borderWidths.top, 0.0);
  borderWidth.right = std::max(borderWidths.right, 0.0);
  borderWidth.bottom = std::max(borderWidths.bottom, 0.0);
  borderWidth.left = std::max(borderWidths.left, 0.0);
  ArkUI_NumberValue borderWidthValue[] = {
      static_cast<float>(borderWidth.top),
      static_cast<float>(borderWidth.right),
      static_cast<float>(borderWidth.bottom),
      static_cast<float>(borderWidth.left)};

  ArkUI_AttributeItem borderWidthItem = {
      borderWidthValue, sizeof(borderWidthValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_BORDER_WIDTH, &borderWidthItem));
  return *this;
}

ArkUINode& ArkUINode::setBorderColor(
    facebook::react::BorderColors const& borderColors) {
  uint32_t borderTopColor = 0xff000000;
  uint32_t bordeRightColor = 0xff000000;
  uint32_t borderBottomColor = 0xff000000;
  uint32_t borderLeftColor = 0xff000000;
  if (borderColors.top) {
    borderTopColor = (uint32_t)*borderColors.top;
  }
  if (borderColors.right) {
    bordeRightColor = (uint32_t)*borderColors.right;
  }
  if (borderColors.bottom) {
    borderBottomColor = (uint32_t)*borderColors.bottom;
  }
  if (borderColors.left) {
    borderLeftColor = (uint32_t)*borderColors.left;
  }
  ArkUI_NumberValue borderColorValue[] = {
      {.u32 = borderTopColor},
      {.u32 = bordeRightColor},
      {.u32 = borderBottomColor},
      {.u32 = borderLeftColor}};

  ArkUI_AttributeItem borderColorItem = {
      borderColorValue, sizeof(borderColorValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_BORDER_COLOR, &borderColorItem));
  return *this;
}

ArkUINode& ArkUINode::setBorderRadius(
    facebook::react::BorderRadii const& borderRadius) {
  ArkUI_NumberValue borderRadiusValue[] = {
      static_cast<float>(borderRadius.topLeft),
      static_cast<float>(borderRadius.topRight),
      static_cast<float>(borderRadius.bottomLeft),
      static_cast<float>(borderRadius.bottomRight)};

  ArkUI_AttributeItem borderRadiusItem = {
      borderRadiusValue, sizeof(borderRadiusValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_BORDER_RADIUS, &borderRadiusItem));
  return *this;
}

ArkUINode& ArkUINode::setBorderStyle(
    facebook::react::BorderStyles const& borderStyles) {
  ArkUI_NumberValue borderStyleValue[] = {
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.top))},
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.right))},
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.bottom))},
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.left))}};

  ArkUI_AttributeItem borderStyleItem = {
      borderStyleValue, sizeof(borderStyleValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_BORDER_STYLE, &borderStyleItem));
  return *this;
}

ArkUINode& ArkUINode::setShadow(
    facebook::react::SharedColor const& shadowColor,
    facebook::react::Size const& shadowOffset,
    float const shadowOpacity,
    float const shadowRadius) {
  if (shadowOpacity <= 0.0 || shadowOpacity > 1.0) {
    return *this;
  }
  uint32_t shadowColorValue = 0xff000000;
  if (shadowColor) {
    shadowColorValue = *shadowColor;
  }
  uint32_t alpha = static_cast<uint32_t>(
      (float)((shadowColorValue >> 24) & (0xff)) * shadowOpacity);
  shadowColorValue = (alpha << 24) + (shadowColorValue & 0xffffff);
  ArkUI_NumberValue shadowValue[] = {
      {.f32 = shadowRadius},
      {.i32 = 0},
      {.f32 = static_cast<float>(shadowOffset.width)},
      {.f32 = static_cast<float>(shadowOffset.height)},
      {.i32 = 0},
      {.u32 = shadowColorValue},
      {.u32 = 0}};
  ArkUI_AttributeItem shadowItem = {
      .value = shadowValue,
      .size = sizeof(shadowValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_CUSTOM_SHADOW, &shadowItem));
  return *this;
}

ArkUINode& ArkUINode::setHitTestMode(
    facebook::react::PointerEventsMode const& pointerEvents) {
  ArkuiHitTestMode hitTestMode =
      (pointerEvents == facebook::react::PointerEventsMode::None ||
       pointerEvents == facebook::react::PointerEventsMode::BoxNone)
      ? ArkuiHitTestMode::NONE
      : ArkuiHitTestMode::DEFAULT;
  ArkUI_NumberValue hitTestModeValue[] = {
      {.i32 = static_cast<int32_t>(hitTestMode)}};
  ArkUI_AttributeItem hitTestModeItem = {
      .value = hitTestModeValue,
      .size = sizeof(hitTestModeValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_HIT_TEST_BEHAVIOR, &hitTestModeItem));
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityDescription(
    std::string const& accessibilityDescription) {
  ArkUI_AttributeItem descriptionItem = {
      .string = accessibilityDescription.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ACCESSIBILITY_DESCRIPTION, &descriptionItem));
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityLevel(
    facebook::react::ImportantForAccessibility importance) {
  ArkUI_NumberValue levelValue[] = {{.i32 = static_cast<int32_t>(importance)}};
  ArkUI_AttributeItem levelItem = {
      .value = levelValue,
      .size = sizeof(levelValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ACCESSIBILITY_MODE, &levelItem));
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityText(
    std::string const& accessibilityLabel) {
  ArkUI_AttributeItem textItem = {.string = accessibilityLabel.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ACCESSIBILITY_TEXT, &textItem));
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityGroup(bool accessible) {
  ArkUI_NumberValue groupValue[] = {{.i32 = static_cast<int32_t>(accessible)}};
  ArkUI_AttributeItem groupItem = {
      .value = groupValue,
      .size = sizeof(groupValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ACCESSIBILITY_GROUP, &groupItem));
  return *this;
}

ArkUINode& ArkUINode::setId(std::string const& id) {
  ArkUI_AttributeItem idItem = {.string = id.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ID, &idItem));
  return *this;
}

ArkUINode& ArkUINode::setBackgroundColor(
    facebook::react::SharedColor const& color) {
  // TODO: figure out if we need to update (to a clear value), or return early
  // here
  uint32_t colorValue = facebook::react::isColorMeaningful(color)
      ? *color
      : *facebook::react::clearColor();
  ArkUI_NumberValue preparedColorValue[] = {{.u32 = colorValue}};
  ArkUI_AttributeItem colorItem = {
      preparedColorValue,
      sizeof(preparedColorValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_BACKGROUND_COLOR, &colorItem));
  return *this;
}

ArkUINode& ArkUINode::setTransform(
    facebook::react::Transform const& transform,
    facebook::react::Float pointScaleFactor) {
  ArkUI_NumberValue transformCenterValue[] = {0, 0, 0, 0.5f, 0.5f};

  ArkUI_AttributeItem transformCenterItem = {
      transformCenterValue,
      sizeof(transformCenterValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TRANSFORM_CENTER, &transformCenterItem));

  // NOTE: ArkUI translation is in `px` units, while React Native uses `vp`
  // units, so we need to correct for the scale factor here
  auto matrix = transform.matrix;
  matrix[12] *= pointScaleFactor;
  matrix[13] *= pointScaleFactor;
  matrix[14] *= pointScaleFactor;

  std::array<ArkUI_NumberValue, 16> transformValue;
  for (int i = 0; i < 16; i++) {
    transformValue[i] = {.f32 = static_cast<float>(matrix[i])};
  }

  ArkUI_AttributeItem transformItem = {
      transformValue.data(), transformValue.size()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TRANSFORM, &transformItem));
  return *this;
}

ArkUINode& ArkUINode::setOpacity(facebook::react::Float opacity) {
  ArkUI_NumberValue opacityValue[] = {{.f32 = (float)opacity}};
  ArkUI_AttributeItem opacityItem = {
      opacityValue, sizeof(opacityValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_OPACITY, &opacityItem));
  return *this;
}

ArkUINode& ArkUINode::setClip(bool clip) {
  uint32_t isClip = static_cast<uint32_t>(clip);
  ArkUI_NumberValue clipValue[] = {{.u32 = isClip}};
  ArkUI_AttributeItem clipItem = {
      clipValue, sizeof(clipValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_CLIP, &clipItem));
  return *this;
}

ArkUINode& ArkUINode::setAlignment(Alignment alignment) {
  ArkUI_NumberValue alignmentValue[] = {
      {.i32 = static_cast<int32_t>(alignment)}};
  ArkUI_AttributeItem alignmentItem = {
      alignmentValue, sizeof(alignmentValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ALIGNMENT, &alignmentItem));
  return *this;
}

ArkUINode& ArkUINode::setTranslateTransition(
    float translateX,
    float translateY,
    int32_t animationDurationMillis) {
  std::array<ArkUI_NumberValue, 5> translateValue = {
      ArkUI_NumberValue{.f32 = translateX},
      {.f32 = translateY},
      {.f32 = 0},
      {.i32 = animationDurationMillis},
      {.i32 = static_cast<int32_t>(ARKUI_CURVE_LINEAR)}};
  ArkUI_AttributeItem translateItem = {
      translateValue.data(), translateValue.size()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TRANSLATE_TRANSITION, &translateItem));
  return *this;
}

ArkUINode& ArkUINode::resetTranslateTransition() {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
      m_nodeHandle, NODE_TRANSLATE_TRANSITION));
  return *this;
}

ArkUINode& ArkUINode::setOpacityTransition(int32_t animationDurationMillis) {
  std::array<ArkUI_NumberValue, 3> args = {
      ArkUI_NumberValue{.f32 = 0},
      {.i32 = animationDurationMillis},
      {.i32 = static_cast<int32_t>(ARKUI_CURVE_LINEAR)}};
  ArkUI_AttributeItem opacityItem = {args.data(), args.size()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_OPACITY_TRANSITION, &opacityItem));
  return *this;
}

ArkUINode& ArkUINode::resetOpacityTransition() {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
      m_nodeHandle, NODE_OPACITY_TRANSITION));
  return *this;
}

ArkUINode& ArkUINode::setOffset(float x, float y) {
  ArkUI_NumberValue offsetValue[] = {{.f32 = x}, {.f32 = y}};
  ArkUI_AttributeItem offsetItem = {
      offsetValue, sizeof(offsetValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_OFFSET, &offsetItem));
  return *this;
}

ArkUINode& ArkUINode::setEnabled(bool enabled) {
  ArkUI_NumberValue value = {.i32 = int32_t(enabled)};
  ArkUI_AttributeItem item = {&value, 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_ENABLED, &item));
  return *this;
}

ArkUINode& ArkUINode::resetAccessibilityText() {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
      m_nodeHandle, NODE_ACCESSIBILITY_TEXT));
  return *this;
}

void ArkUINode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {}

void ArkUINode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    std::string_view eventString) {}

ArkUINode& ArkUINode::setFocusStatus(int32_t focus) {
  std::array<ArkUI_NumberValue, 1> value = {{{.i32 = focus}}};
  ArkUI_AttributeItem item = {value.data(), value.size()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_FOCUS_STATUS, &item));
  return *this;
}

ArkUINode::~ArkUINode() {
  if (m_nodeHandle != nullptr) {
    ArkUINodeRegistry::getInstance().unregisterNode(this);
    NativeNodeApi::getInstance()->disposeNode(m_nodeHandle);
  }
}

} // namespace rnoh
