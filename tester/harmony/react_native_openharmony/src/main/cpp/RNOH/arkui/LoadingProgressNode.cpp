#include "LoadingProgressNode.h"
#include "NativeNodeApi.h"

// LoadingProgressNode structure
// -> Column <- parentNode
//   -> Stack <- loadingBackgroundNode
//     -> LoadingProgress <- loadingSpinnerNode
//   -> End of Stack
//   -> Text <- loadingTitleNode - added only when title prop is provided
// -> End of Column

namespace rnoh {
LoadingProgressNode::LoadingProgressNode()
    : ArkUINode(this->buildLoadingProgressNode()) {}

ArkUI_NodeHandle LoadingProgressNode::createLoadingSpinnerNode() {
  ArkUI_NodeHandle loadingSpinnerNode =
      NativeNodeApi::getInstance()->createNode(
          ArkUI_NodeType::ARKUI_NODE_LOADING_PROGRESS);

  ArkUI_NumberValue spinnerSize[] = {LOADING_SPINNER_NODE_SIZE};
  ArkUI_AttributeItem spinnerSizeItem = {
      spinnerSize, sizeof(spinnerSize) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingSpinnerNode, NODE_WIDTH, &spinnerSizeItem));
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingSpinnerNode, NODE_HEIGHT, &spinnerSizeItem));

  return loadingSpinnerNode;
}

void LoadingProgressNode::updateLoadingTitleNode(
    ArkUI_NodeHandle loadingTitleNode,
    const std::string& title,
    facebook::react::SharedColor const& color) {
  ArkUI_AttributeItem loadingTitleItem = {.string = title.c_str()};

  uint32_t loadingTitleColor = color ? *color : *facebook::react::blackColor();
  ArkUI_NumberValue nLoadingTitleColor[] = {{.u32 = loadingTitleColor}};
  ArkUI_AttributeItem loadingTitleColorItem = {
      nLoadingTitleColor,
      sizeof(nLoadingTitleColor) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingTitleNode, NODE_TEXT_CONTENT, &loadingTitleItem));
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingTitleNode, NODE_FONT_COLOR, &loadingTitleColorItem));
}

ArkUI_NodeHandle LoadingProgressNode::createLoadingTitleNode(
    const std::string& title,
    facebook::react::SharedColor const& color) {
  ArkUI_NodeHandle loadingTitleNode =
      NativeNodeApi::getInstance()->createNode(ArkUI_NodeType::ARKUI_NODE_TEXT);

  this->updateLoadingTitleNode(loadingTitleNode, title, color);

  return loadingTitleNode;
}

ArkUI_NodeHandle LoadingProgressNode::createLoadingBackgroundNode() {
  ArkUI_NodeHandle loadingBackgroundNode =
      NativeNodeApi::getInstance()->createNode(
          ArkUI_NodeType::ARKUI_NODE_STACK);
  uint32_t loadingBackgroundColor = *facebook::react::clearColor();
  ArkUI_NumberValue nLoadingBackgroundColor[] = {
      {.u32 = loadingBackgroundColor}};
  ArkUI_AttributeItem loadingBackgroundColorItem = {
      nLoadingBackgroundColor,
      sizeof(nLoadingBackgroundColor) / sizeof(ArkUI_NumberValue)};

  ArkUI_NumberValue borderRadiusValue[] = {1000, 1000, 1000, 1000};
  ArkUI_AttributeItem loadingBackgroundRadiusItem = {
      borderRadiusValue, sizeof(borderRadiusValue) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingBackgroundNode, NODE_BORDER_RADIUS, &loadingBackgroundRadiusItem));

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingBackgroundNode,
      NODE_BACKGROUND_COLOR,
      &loadingBackgroundColorItem));

  return loadingBackgroundNode;
}

void LoadingProgressNode::updateLoadingBackgroundNode(
    ArkUI_NodeHandle loadingBackgroundNode,
    const facebook::react::SharedColor& color) {
  uint32_t loadingBackgroundColor = *color;
  ArkUI_NumberValue nLoadingBackgroundColor[] = {
      {.u32 = loadingBackgroundColor}};
  ArkUI_AttributeItem loadingBackgroundColorItem = {
      nLoadingBackgroundColor,
      sizeof(nLoadingBackgroundColor) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingBackgroundNode,
      NODE_BACKGROUND_COLOR,
      &loadingBackgroundColorItem));
}

ArkUI_NodeHandle LoadingProgressNode::buildLoadingProgressNode() {
  ArkUI_NodeHandle parentNode = NativeNodeApi::getInstance()->createNode(
      ArkUI_NodeType::ARKUI_NODE_COLUMN);

  ArkUI_NodeHandle loadingBackgroundNode = this->createLoadingBackgroundNode();
  ArkUI_NodeHandle loadingSpinnerNode = this->createLoadingSpinnerNode();

  maybeThrow(NativeNodeApi::getInstance()->addChild(
      loadingBackgroundNode, loadingSpinnerNode));
  maybeThrow(NativeNodeApi::getInstance()->addChild(
      parentNode, loadingBackgroundNode));

  return parentNode;
}

void LoadingProgressNode::setLoadingSpinnerNodeAnimating(const bool& enable) {
  uint32_t typeValue = 1;
  if (!enable) {
    typeValue = 0;
  }
  ArkUI_NodeHandle loadingBackgroundNode =
      NativeNodeApi::getInstance()->getChildAt(m_nodeHandle, 0);
  ArkUI_NodeHandle loadingSpinnderNode =
      NativeNodeApi::getInstance()->getChildAt(loadingBackgroundNode, 0);

  ArkUI_NumberValue preparedTypeValue[] = {{.u32 = typeValue}};
  ArkUI_AttributeItem typeItem = {
      preparedTypeValue, sizeof(preparedTypeValue) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingSpinnderNode, NODE_LOADING_PROGRESS_ENABLE_LOADING, &typeItem));
}

void LoadingProgressNode::createOrUpdateLoadingProgressNodeTitle(
    const std::string& title,
    facebook::react::SharedColor const& color) {
  ArkUI_NodeHandle existingLoadingTitleNode =
      NativeNodeApi::getInstance()->getChildAt(m_nodeHandle, 1);

  if (!existingLoadingTitleNode) {
    ArkUI_NodeHandle loadingTitleNode =
        this->createLoadingTitleNode(title, color);
    maybeThrow(
        NativeNodeApi::getInstance()->addChild(m_nodeHandle, loadingTitleNode));
    return;
  };

  updateLoadingTitleNode(existingLoadingTitleNode, title, color);
};

void LoadingProgressNode::updateLoadingProgressBackgroundColor(
    const facebook::react::SharedColor& color) {
  ArkUI_NodeHandle loadingBackgroundNode =
      NativeNodeApi::getInstance()->getChildAt(m_nodeHandle, 0);

  if (loadingBackgroundNode) {
    this->updateLoadingBackgroundNode(loadingBackgroundNode, color);
  };
}

void LoadingProgressNode::updateLoadingSpinnerNodeColor(
    facebook::react::SharedColor const& color) {
  ArkUI_NodeHandle loadingBackgroundNode =
      NativeNodeApi::getInstance()->getChildAt(m_nodeHandle, 0);
  ArkUI_NodeHandle loadingSpinnderNode =
      NativeNodeApi::getInstance()->getChildAt(loadingBackgroundNode, 0);

  uint32_t loadingSpinnerColor = *color;
  ArkUI_NumberValue nLoadingSpinnerColor[] = {{.u32 = loadingSpinnerColor}};
  ArkUI_AttributeItem colorItem = {
      nLoadingSpinnerColor,
      sizeof(nLoadingSpinnerColor) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      loadingSpinnderNode, NODE_LOADING_PROGRESS_COLOR, &colorItem));
}

} // namespace rnoh
