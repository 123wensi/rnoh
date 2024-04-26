#pragma once
#include <react/renderer/graphics/Point.h>
#include "ArkUINode.h"
#include "NativeNodeApi.h"

namespace rnoh {
class ScrollNodeDelegate {
 public:
  virtual ~ScrollNodeDelegate() = default;
  virtual void onScroll(){};
  virtual void onScrollStart(){};
  virtual void onScrollStop(){};
  virtual float onScrollFrameBegin(float offset, int32_t scrollState) {
    return offset;
  };
};

class ScrollNode : public ArkUINode {
 protected:
  using Point = facebook::react::Point;

  ArkUI_NodeHandle m_childArkUINodeHandle;
  ScrollNodeDelegate* m_scrollNodeDelegate;

 public:
  ScrollNode();
  ~ScrollNode() override;

  void insertChild(ArkUINode& child);
  void removeChild(ArkUINode& child);
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;

  Point getScrollOffset() const;
  void setScrollNodeDelegate(ScrollNodeDelegate* scrollNodeDelegate);
  ScrollNode& setHorizontal(bool horizontal);
  ScrollNode& setEnableScrollInteraction(bool enableScrollInteraction);
  ScrollNode& setFriction(float friction);
  ScrollNode& setEdgeEffect(bool bounces, bool alwaysBounces);
  ScrollNode& setScrollBarDisplayMode(
      ArkUI_ScrollBarDisplayMode scrollBarDisplayMode);
  ScrollNode& setScrollBarColor(uint32_t scrollBarColor);
  ScrollNode& setScrollSnap(
      ArkUI_ScrollSnapAlign scrollSnapAlign,
      bool snapToStart,
      bool snapToEnd,
      const std::vector<facebook::react::Float>& snapPoints);
  ScrollNode& setEnablePaging(bool enablePaging);
  ScrollNode& setCenterContent(bool centerContent);
  void scrollTo(float x, float y, bool animated);
  void setNestedScroll(ArkUI_ScrollNestedMode scrollNestedMode);
};

} // namespace rnoh