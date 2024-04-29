#pragma once

#include "ArkUINode.h"

namespace rnoh {

class LoadingProgressNode : public ArkUINode {
 protected:
  static constexpr float LOADING_SPINNER_NODE_SIZE = 29;
  ArkUI_NodeHandle buildLoadingProgressNode();
  ArkUI_NodeHandle createLoadingBackgroundNode();
  ArkUI_NodeHandle createLoadingSpinnerNode();
  ArkUI_NodeHandle createLoadingTitleNode(
      std::string const& title,
      facebook::react::SharedColor const& color);
  void updateLoadingBackgroundNode(
      ArkUI_NodeHandle loadingBackgroundNode,
      facebook::react::SharedColor const& color);
  void updateLoadingTitleNode(
      ArkUI_NodeHandle loadingTitleNode,
      std::string const& title,
      facebook::react::SharedColor const& color);

 public:
  LoadingProgressNode();
  void setLoadingSpinnerNodeAnimating(const bool& enable);
  void createOrUpdateLoadingProgressNodeTitle(
      std::string const& title,
      facebook::react::SharedColor const& color);
  void updateLoadingSpinnerNodeColor(facebook::react::SharedColor const& color);
  void updateLoadingProgressBackgroundColor(
      facebook::react::SharedColor const& color);
  void setLoadingProgressNodeTitleColor(
      facebook::react::SharedColor const& color);
};

} // namespace rnoh