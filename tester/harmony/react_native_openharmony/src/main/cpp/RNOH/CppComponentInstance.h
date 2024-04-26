/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <react/renderer/components/view/TouchEventEmitter.h>
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/EventEmitter.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/core/State.h>
#include <vector>
#include "RNOH/ComponentInstance.h"

namespace rnoh {

inline facebook::react::Rect transformRectAroundPoint(
    const facebook::react::Rect& rect,
    const facebook::react::Point& point,
    const facebook::react::Transform& transform);
template <typename ShadowNodeT>
class CppComponentInstance : public ComponentInstance {
  static_assert(
      std::is_base_of_v<facebook::react::ShadowNode, ShadowNodeT>,
      "ShadowNodeT must be a subclass of facebook::react::ShadowNode");

 protected:
  using ConcreteProps = typename ShadowNodeT::ConcreteProps;
  using ConcreteState = typename ShadowNodeT::ConcreteState;
  using ConcreteEventEmitter = typename ShadowNodeT::ConcreteEventEmitter;
  using SharedConcreteProps = std::shared_ptr<const ConcreteProps>;
  using SharedConcreteState = std::shared_ptr<const ConcreteState>;
  using SharedConcreteEventEmitter =
      std::shared_ptr<const ConcreteEventEmitter>;

 public:
  using Shared = std::shared_ptr<CppComponentInstance>;

  CppComponentInstance(Context context)
      : ComponentInstance(std::move(context)) {}

  facebook::react::Tag getTag() const {
    return m_tag;
  }

  facebook::react::Props::Shared getProps() const override {
    return m_props;
  }

  void setProps(facebook::react::Props::Shared props) final {
    auto newProps = std::dynamic_pointer_cast<const ConcreteProps>(props);
    if (!newProps) {
      return;
    }
    this->onPropsChanged(newProps);
    m_props = newProps;
  }

  SharedConcreteState const& getState() const {
    return m_state;
  }

  void setState(facebook::react::State::Shared state) final {
    auto newState = std::dynamic_pointer_cast<const ConcreteState>(state);
    if (!newState) {
      return;
    }

    this->onStateChanged(newState);
    m_state = newState;
  }

  SharedConcreteEventEmitter const& getEventEmitter() const {
    return m_eventEmitter;
  }

  void setEventEmitter(facebook::react::SharedEventEmitter eventEmitter) final {
    auto newEventEmitter =
        std::dynamic_pointer_cast<const ConcreteEventEmitter>(eventEmitter);
    if (!newEventEmitter) {
      return;
    }
    this->onEventEmitterChanged(newEventEmitter);
    m_eventEmitter = newEventEmitter;
  }

  void setLayout(facebook::react::LayoutMetrics layoutMetrics) override {
    this->getLocalRootArkUINode().setPosition(layoutMetrics.frame.origin);
    this->getLocalRootArkUINode().setSize(layoutMetrics.frame.size);
    m_layoutMetrics = layoutMetrics;
    markBoundingBoxAsDirty();
  }

  // TouchTarget implementation
  facebook::react::LayoutMetrics getLayoutMetrics() const override {
    return m_layoutMetrics;
  }

  bool containsPoint(facebook::react::Point const& point) const override {
    auto hitRect = getHitRect();
    return hitRect.containsPoint(point);
  }

  bool containsPointInBoundingBox(
      facebook::react::Point const& point) override {
    auto boundingBox = this->getBoundingBox();
    return boundingBox.containsPoint(point);
  }

  bool canHandleTouch() const override {
    if (m_props != nullptr) {
      auto props =
          std::dynamic_pointer_cast<const facebook::react::ViewProps>(m_props);
      return props->pointerEvents == facebook::react::PointerEventsMode::Auto ||
          props->pointerEvents == facebook::react::PointerEventsMode::BoxOnly;
    }
    return true;
  };

  bool canChildrenHandleTouch() const override {
    if (m_props != nullptr) {
      auto props =
          std::dynamic_pointer_cast<const facebook::react::ViewProps>(m_props);
      return props->pointerEvents == facebook::react::PointerEventsMode::Auto ||
          props->pointerEvents == facebook::react::PointerEventsMode::BoxNone;
    }
    return true;
  };

  facebook::react::Tag getTouchTargetTag() const override {
    return getTag();
  }

  TouchTarget::Shared getTouchTargetParent() const override {
    return getParent().lock();
  }

  facebook::react::SharedTouchEventEmitter getTouchEventEmitter()
      const override {
    return m_eventEmitter;
  }

  std::vector<TouchTarget::Shared> getTouchTargetChildren() override {
    auto children = getChildren();
    return std::vector<TouchTarget::Shared>(children.begin(), children.end());
  }

  facebook::react::Transform getTransform() const override {
    if (m_props != nullptr) {
      return m_props->transform;
    }
    return facebook::react::Transform::Identity();
  }

  facebook::react::Rect getBoundingBox() override {
    if (!m_boundingBox.has_value()) {
      calculateBoundingBox();
    }
    return m_boundingBox.value();
  };

  bool isClippingSubviews() const override {
    return m_isClipping;
  }

  void markBoundingBoxAsDirty() override {
    if (m_boundingBox.has_value()) {
      m_boundingBox.reset();
      auto parent = getTouchTargetParent();
      while (parent != nullptr && !parent->isClippingSubviews()) {
        parent->markBoundingBoxAsDirty();
        parent = parent->getTouchTargetParent();
      }
    }
  };

 protected:
  virtual void onPropsChanged(SharedConcreteProps const& props) {
    auto isOpacityManagedByAnimated = getIgnoredPropKeys().count("opacity") > 0;
    auto isTransformManagedByAnimated =
        getIgnoredPropKeys().count("transform") > 0;
    auto old = m_props;
    if (!old || *(props->backgroundColor) != *(old->backgroundColor)) {
      this->getLocalRootArkUINode().setBackgroundColor(props->backgroundColor);
    }

    facebook::react::BorderMetrics borderMetrics =
        props->resolveBorderMetrics(this->m_layoutMetrics);
    facebook::react::BorderMetrics oldBorderMetrics;
    if (!old || borderMetrics.borderWidths != m_oldBorderMetrics.borderWidths) {
      this->getLocalRootArkUINode().setBorderWidth(borderMetrics.borderWidths);
    }
    if (!old || borderMetrics.borderColors != m_oldBorderMetrics.borderColors) {
      this->getLocalRootArkUINode().setBorderColor(borderMetrics.borderColors);
    }
    if (!old || borderMetrics.borderRadii != m_oldBorderMetrics.borderRadii ||
        !m_isRadiusSetValid) {
      if (this->m_layoutMetrics.frame.size != facebook::react::Size{0, 0}) {
        m_isRadiusSetValid = true;
      }
      this->getLocalRootArkUINode().setBorderRadius(borderMetrics.borderRadii);
    }
    if (!old || borderMetrics.borderStyles != m_oldBorderMetrics.borderStyles) {
      this->getLocalRootArkUINode().setBorderStyle(borderMetrics.borderStyles);
    }

    if (!old ||
        props->shadowColor != old->shadowColor &&
            props->shadowOffset != old->shadowOffset &&
            props->shadowOpacity != old->shadowOpacity &&
            props->shadowRadius != old->shadowRadius) {
      this->getLocalRootArkUINode().setShadow(
          props->shadowColor,
          props->shadowOffset,
          props->shadowOpacity,
          props->shadowRadius);
    }

    if (!isTransformManagedByAnimated &&
        (!old || props->transform != old->transform ||
         abs(m_oldPointScaleFactor - m_layoutMetrics.pointScaleFactor) >
             0.001f)) {
      m_oldPointScaleFactor = m_layoutMetrics.pointScaleFactor;
      this->getLocalRootArkUINode().setTransform(
          props->transform, m_layoutMetrics.pointScaleFactor);
      markBoundingBoxAsDirty();
    }

    if (!old || props->pointerEvents != old->pointerEvents) {
      this->getLocalRootArkUINode().setHitTestMode(props->pointerEvents);
    }

    if (!old || props->accessibilityHint != old->accessibilityHint) {
      this->getLocalRootArkUINode().setAccessibilityDescription(
          props->accessibilityHint);
    }

    if (!old ||
        props->importantForAccessibility != old->importantForAccessibility) {
      this->getLocalRootArkUINode().setAccessibilityLevel(
          props->importantForAccessibility);
    }

    if (!old || props->accessibilityLabel != old->accessibilityLabel) {
      this->getLocalRootArkUINode().setAccessibilityText(
          props->accessibilityLabel);
    }

    if (!old || props->accessible != old->accessible) {
      this->getLocalRootArkUINode().setAccessibilityGroup(props->accessible);
    }
    if (!isOpacityManagedByAnimated && !isTransformManagedByAnimated &&
        (!old || props->opacity != old->opacity ||
         props->transform != old->transform ||
         props->backfaceVisibility != old->backfaceVisibility)) {
      this->setOpacity(props);
    }

    auto newOverflow = props->getClipsContentToBounds();
    if (!old || (old->getClipsContentToBounds() != newOverflow)) {
      m_isClipping = newOverflow;
      this->getLocalRootArkUINode().setClip(newOverflow);
      markBoundingBoxAsDirty();
    }

    this->getLocalRootArkUINode().setId(getIdFromProps(props));

    m_oldBorderMetrics = props->resolveBorderMetrics(this->m_layoutMetrics);
  };

  virtual void onStateChanged(SharedConcreteState const& state){};

  virtual void onEventEmitterChanged(
      SharedConcreteEventEmitter const& eventEmitter){};

  void calculateBoundingBox() {
    auto newBoundingBox = getHitRect();
    if (!m_isClipping) {
      for (auto& child : m_children) {
        auto childBoundingBox = child->getBoundingBox();
        childBoundingBox.origin += child->getLayoutMetrics().frame.origin;

        auto childCenter = child->getLayoutMetrics().frame.getCenter();
        auto childTransform = child->getTransform();

        childBoundingBox = transformRectAroundPoint(
            childBoundingBox, childCenter, childTransform);
        newBoundingBox.unionInPlace(childBoundingBox);
      }
    }
    m_boundingBox = newBoundingBox;
  };

  facebook::react::Rect getHitRect() const {
    facebook::react::Point origin = {0, 0};
    auto size = m_layoutMetrics.frame.size;
    if (m_props) {
      origin -= {m_props->hitSlop.left, m_props->hitSlop.top};
      size +=
          {m_props->hitSlop.right + m_props->hitSlop.left,
           m_props->hitSlop.top + m_props->hitSlop.bottom};
    }
    return {origin, size};
  };

 private:
  void setOpacity(SharedConcreteProps const& props) {
    auto opacity = props->opacity;
    float validOpacity = std::max(0.0f, std::min((float)opacity, 1.0f));
    facebook::react::Transform transform = props->transform;
    if (props->backfaceVisibility ==
        facebook::react::BackfaceVisibility::Hidden) {
      facebook::react::Vector vec{0, 0, 1, 0};
      auto resVec = transform * vec;
      if (resVec.z < 0.0) {
        validOpacity = 0.0;
      }
    }
    this->getLocalRootArkUINode().setOpacity(validOpacity);
  }

 protected:
  std::string getIdFromProps(SharedConcreteProps const& props) const {
    if (props->testId != "") {
      return props->testId;
    } else if (props->nativeId != "") {
      return props->nativeId;
    } else {
      std::string id = ShadowNodeT::Name();
      id += "@";
      id += m_tag;
      return id;
    }
  }

  SharedConcreteProps m_props;
  SharedConcreteState m_state;
  SharedConcreteEventEmitter m_eventEmitter;
  std::optional<facebook::react::Rect> m_boundingBox;
  bool m_isClipping = false;
};

inline facebook::react::Rect transformRectAroundPoint(
    const facebook::react::Rect& rect,
    const facebook::react::Point& point,
    const facebook::react::Transform& transform) {
  using Point = facebook::react::Point;

  auto leftTopTransformed =
      (Point({rect.getMinX(), rect.getMinY()}) - point) * transform + point;
  auto leftBottomTransformed =
      (Point({rect.getMinX(), rect.getMaxY()}) - point) * transform + point;
  auto rightTopTransformed =
      (Point({rect.getMaxX(), rect.getMinY()}) - point) * transform + point;
  auto rightBottomTransformed =
      (Point({rect.getMaxX(), rect.getMaxY()}) - point) * transform + point;
  return facebook::react::Rect::boundingRect(
      leftTopTransformed,
      leftBottomTransformed,
      rightTopTransformed,
      rightBottomTransformed);
}
} // namespace rnoh
