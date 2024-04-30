#include "StackNode.h"

#include <glog/logging.h>
#include <memory>
#include "NativeNodeApi.h"

namespace rnoh {

StackNode::StackNode()
    : ArkUINode(NativeNodeApi::getInstance()->createNode(
          ArkUI_NodeType::ARKUI_NODE_STACK)),
      m_stackNodeDelegate(nullptr) {
  maybeThrow(NativeNodeApi::getInstance()->registerNodeEvent(
      m_nodeHandle, NODE_ON_CLICK, 0, this));
}

void StackNode::insertChild(ArkUINode& child, std::size_t index) {
  maybeThrow(NativeNodeApi::getInstance()->insertChildAt(
      m_nodeHandle, child.getArkUINodeHandle(), static_cast<int32_t>(index)));
}

void StackNode::removeChild(ArkUINode& child) {
  maybeThrow(NativeNodeApi::getInstance()->removeChild(
      m_nodeHandle, child.getArkUINodeHandle()));
}

void StackNode::setStackNodeDelegate(StackNodeDelegate* stackNodeDelegate) {
  m_stackNodeDelegate = stackNodeDelegate;
}

void StackNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  if (eventType == ArkUI_NodeEventType::NODE_ON_CLICK &&
      eventArgs[3].i32 != 2) {
    onClick();
  }
}

void StackNode::onClick() {
  if (m_stackNodeDelegate != nullptr) {
    m_stackNodeDelegate->onClick();
  }
}

StackNode::~StackNode() {
  NativeNodeApi::getInstance()->unregisterNodeEvent(
      m_nodeHandle, NODE_ON_CLICK);
}

StackNode& StackNode::setAlign(int32_t align) {
  ArkUI_NumberValue value[] = {{.i32 = align}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_STACK_ALIGN_CONTENT, &item));
  return *this;
}
} // namespace rnoh
