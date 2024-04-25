#pragma once

#include <react/renderer/components/text/ParagraphShadowNode.h>
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/SpanNode.h"
#include "RNOH/arkui/StackNode.h"
#include "RNOH/arkui/TextNode.h"

namespace rnoh {
class TextComponentInstance
    : public CppComponentInstance<facebook::react::ParagraphShadowNode> {
 private:
  using FragmentTouchTargetByTag =
      std::unordered_map<facebook::react::Tag, std::shared_ptr<TouchTarget>>;

  TextNode m_textNode{};
  StackNode m_stackNode{};
  std::vector<std::shared_ptr<ArkUINode>> m_childNodes{};
  FragmentTouchTargetByTag m_fragmentTouchTargetByTag{};
  bool m_touchTargetChildrenNeedUpdate = false;

 public:
  TextComponentInstance(Context context);
  ~TextComponentInstance() override;
  StackNode& getLocalRootArkUINode() override;
  std::vector<TouchTarget::Shared> getTouchTargetChildren() override;

 protected:
  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;
  void onPropsChanged(SharedConcreteProps const& props) override;
  void onStateChanged(SharedConcreteState const& textState) override;

 private:
  void setTextAttributes(const facebook::react::TextAttributes& textAttributes);
  void setFragment(
      const facebook::react::AttributedString::Fragment& fragment,
      std::shared_ptr<SpanNode> spanNode,
      uint32_t index);
  void setImageSpanSize(
      const facebook::react::Size& imageSize,
      std::shared_ptr<ImageSpanNode> imageSpanNode);
  void setParagraphAttributes(
      const facebook::react::ParagraphAttributes& paragraphAttributes);
  std::string stringCapitalize(const std::string& strInput);
  void updateFragmentTouchTargets(
      facebook::react::ParagraphState const& newState);
};
} // namespace rnoh