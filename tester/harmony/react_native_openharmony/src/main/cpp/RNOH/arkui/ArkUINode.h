/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <arkui/native_node.h>
#include <folly/dynamic.h>
#include <react/renderer/components/rncore/Props.h>
#include <react/renderer/components/view/AccessibilityPrimitives.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/Rect.h>
#include <react/renderer/graphics/Transform.h>
#include <stdexcept>
#include "ArkUINodeRegistry.h"
#include "glog/logging.h"
#include "react/renderer/components/view/primitives.h"

namespace rnoh {

namespace {
enum class ArkuiHitTestMode : int32_t {
  DEFAULT,
  BLOCK,
  TRANSPARENT,
  NONE,
};
}

class ArkUINode {
 protected:
  ArkUINode(const ArkUINode& other) = delete;
  ArkUINode& operator=(const ArkUINode& other) = delete;

  ArkUINode& operator=(ArkUINode&& other) noexcept;
  ArkUINode(ArkUINode&& other) noexcept;

 public:
  using Alignment = ArkUI_Alignment;

  ArkUI_NodeHandle getArkUINodeHandle();
  ArkUINode(ArkUI_NodeHandle nodeHandle);

  void markDirty();

  virtual ArkUINode& setPosition(facebook::react::Point const& position);
  virtual ArkUINode& setSize(facebook::react::Size const& size);
  virtual ArkUINode& setBorderWidth(
      facebook::react::BorderWidths const& borderWidths);
  virtual ArkUINode& setBorderColor(
      facebook::react::BorderColors const& borderColors);
  virtual ArkUINode& setBorderRadius(
      facebook::react::BorderRadii const& borderRadius);
  virtual ArkUINode& setBorderStyle(
      facebook::react::BorderStyles const& borderStyles);
  virtual ArkUINode& setBackgroundColor(
      facebook::react::SharedColor const& color);
  virtual ArkUINode& setTransform(
      facebook::react::Transform const& transform,
      facebook::react::Float pointScaleFactor);
  virtual ArkUINode& setShadow(
      facebook::react::SharedColor const& shadowColor,
      facebook::react::Size const& shadowOffset,
      float const shadowOpacity,
      float const shadowRadius);
  virtual ArkUINode& setHitTestMode(
      facebook::react::PointerEventsMode const& pointerEvents);
  virtual ArkUINode& setAccessibilityDescription(
      std::string const& accessibilityDescription);
  virtual ArkUINode& setAccessibilityLevel(
      facebook::react::ImportantForAccessibility importance);
  virtual ArkUINode& setAccessibilityText(
      std::string const& accessibilityLabel);
  virtual ArkUINode& setAccessibilityGroup(bool accessible);
  virtual ArkUINode& setId(std::string const& id);
  virtual ArkUINode& setOpacity(facebook::react::Float opacity);
  virtual ArkUINode& setClip(bool clip);
  virtual ArkUINode& setAlignment(Alignment alignment);
  virtual ArkUINode& setTranslateTransition(
      float translateX,
      float translateY,
      int32_t animationDurationMillis);
  virtual ArkUINode& resetTranslateTransition();
  virtual ArkUINode& setOpacityTransition(int32_t animationDurationMillis);
  virtual ArkUINode& resetOpacityTransition();
  virtual ArkUINode& setOffset(float x, float y);
  virtual ArkUINode& setEnabled(bool enabled);
  virtual ArkUINode& setFocusStatus(int32_t focus);

  virtual ArkUINode& resetAccessibilityText();

  // method for handling node events with ArkUI_NodeComponentEvent payload type
  virtual void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs);
  // method for handling node events with ArkUI_StringAsyncEvent payload type
  virtual void onNodeEvent(
      ArkUI_NodeEventType eventType,
      std::string_view eventString);

  virtual ~ArkUINode();

 protected:
  void maybeThrow(int32_t status) {
    // TODO: map status to error message, maybe add a new error type
    if (status != 0) {
      auto message = std::string("ArkUINode operation failed with status: ") +
          std::to_string(status);
      LOG(ERROR) << message;
      throw std::runtime_error(std::move(message));
    }
  }

  ArkUI_NodeHandle m_nodeHandle;
};
} // namespace rnoh
