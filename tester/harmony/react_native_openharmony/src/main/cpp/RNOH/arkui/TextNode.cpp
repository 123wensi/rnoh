#include "TextNode.h"
#include <react/renderer/attributedstring/TextAttributes.h>
#include "NativeNodeApi.h"
#include "react/renderer/attributedstring/primitives.h"

namespace rnoh {

TextNode::TextNode()
    : ArkUINode(NativeNodeApi::getInstance()->createNode(
          ArkUI_NodeType::ARKUI_NODE_TEXT)) {}

void TextNode::insertChild(ArkUINode& child, std::size_t index) {
  maybeThrow(NativeNodeApi::getInstance()->insertChildAt(
      m_nodeHandle, child.getArkUINodeHandle(), static_cast<int32_t>(index)));
}

void TextNode::removeChild(ArkUINode& child) {
  maybeThrow(NativeNodeApi::getInstance()->removeChild(
      m_nodeHandle, child.getArkUINodeHandle()));
}

TextNode& TextNode::setTextContent(const std::string& text) {
  ArkUI_AttributeItem item = {.string = text.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_CONTENT, &item));
  return *this;
}

TextNode& TextNode::setFontColor(uint32_t color) {
  ArkUI_NumberValue value[] = {{.u32 = color}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_FONT_COLOR, &item));
  return *this;
}

TextNode& TextNode::resetFontColor() {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
      m_nodeHandle, NODE_FONT_COLOR));
  return *this;
}

TextNode& TextNode::setFontSize(float fontSize) {
  ArkUI_NumberValue value[] = {{.f32 = fontSize}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_FONT_SIZE, &item));
  return *this;
}

TextNode& TextNode::setFontStyle(int32_t fontStyle) {
  ArkUI_NumberValue value[] = {{.i32 = fontStyle}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_FONT_STYLE, &item));
  return *this;
}

TextNode& TextNode::setFontWeight(int32_t fontWeight) {
  ArkUI_NumberValue value[] = {{.i32 = fontWeight}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_FONT_WEIGHT, &item));
  return *this;
}

TextNode& TextNode::setTextLineHeight(float textLineHeight) {
  ArkUI_NumberValue value[] = {{.f32 = textLineHeight}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_LINE_HEIGHT, &item));
  return *this;
}

TextNode& TextNode::setTextDecoration(
    int32_t decorationStyle,
    uint32_t decorationColor /*= 0xFFFF0000*/) {
  ArkUI_NumberValue value[] = {
      {.i32 = decorationStyle}, {.u32 = decorationColor}};
  ArkUI_AttributeItem item = {
      .value = value, .size = sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_DECORATION, &item));
  return *this;
}

TextNode& TextNode::setTextCase(int32_t textCase) {
  ArkUI_NumberValue value[] = {{.i32 = textCase}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_CASE, &item));
  return *this;
}

TextNode& TextNode::setTextLetterSpacing(float textLetterSpacing) {
  ArkUI_NumberValue value[] = {{.f32 = textLetterSpacing}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_LETTER_SPACING, &item));
  return *this;
}

TextNode& TextNode::setTextMaxLines(int32_t textMaxLines) {
  ArkUI_NumberValue value[] = {{.i32 = textMaxLines}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_MAX_LINES, &item));
  return *this;
}

TextNode& TextNode::resetTextMaxLines() {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
      m_nodeHandle, NODE_TEXT_MAX_LINES));
  return *this;
}

TextNode& TextNode::setTextAlign(int32_t align) {
  ArkUI_NumberValue value[] = {{.i32 = align}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_ALIGN, &item));
  return *this;
}

TextNode& TextNode::setTextEllipsisMode(int32_t ellipsisMode) {
  ArkUI_NumberValue value[] = {{.i32 = ellipsisMode}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_ELLIPSIS_MODE, &item));
  return *this;
}

TextNode& TextNode::setTextOverflow(int32_t textOverflow) {
  ArkUI_NumberValue value[] = {{.i32 = textOverflow}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_OVERFLOW, &item));
  return *this;
}

TextNode& TextNode::setFontFamily(const std::string& fontFamily) {
  ArkUI_AttributeItem item = {.string = fontFamily.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_FONT_FAMILY, &item));
  return *this;
}

TextNode& TextNode::setTextCopyOption(int32_t testCopyOption) {
  if (!m_initFlag[FLAG_COPYOPTION] || testCopyOption != m_testCopyOption) {
    VLOG(3) << "[text-debug] setTextCopyOption flag="
            << m_initFlag[FLAG_COPYOPTION];
    ArkUI_NumberValue value[] = {{.i32 = testCopyOption}};
    ArkUI_AttributeItem item = {.value = value, .size = 1};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_TEXT_COPY_OPTION, &item));
    m_initFlag[FLAG_COPYOPTION] = true;
    m_testCopyOption = testCopyOption;
  }
  return *this;
}

TextNode& TextNode::setTextBaselineOffset(float textBaselineOffset) {
  ArkUI_NumberValue value[] = {{.f32 = textBaselineOffset}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_BASELINE_OFFSET, &item));
  return *this;
}

TextNode& TextNode::setTextShadow(
    float textShadowRadius,
    int32_t textShadowType,
    uint32_t textShadowColor,
    float textShadowOffsetX,
    float textShadowOffsetY) {
  ArkUI_NumberValue value[] = {
      {.f32 = textShadowRadius},
      {.i32 = textShadowType},
      {.u32 = textShadowColor},
      {.f32 = textShadowOffsetX},
      {.f32 = textShadowOffsetY}};
  ArkUI_AttributeItem item = {
      .value = value, .size = sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_TEXT_SHADOW, &item));
  return *this;
}

TextNode& TextNode::setMinFontSize(float minFontSize) {
  if (!m_initFlag[FLAG_MINFONTSIZE] || minFontSize != m_minFontSize) {
    VLOG(3) << "[text-debug] setMinFontSize flag="
            << m_initFlag[FLAG_MINFONTSIZE];
    ArkUI_NumberValue value[] = {{.f32 = minFontSize}};
    ArkUI_AttributeItem item = {.value = value, .size = 1};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_TEXT_MIN_FONT_SIZE, &item));
    m_initFlag[FLAG_MINFONTSIZE] = true;
    m_minFontSize = minFontSize;
  }
  return *this;
}

TextNode& TextNode::setMaxFontSize(float maxFontSize) {
  if (!m_initFlag[FLAG_MAXFONTSIZE] || maxFontSize != m_maxFontSize) {
    VLOG(3) << "[text-debug] setMaxFontSize flag="
            << m_initFlag[FLAG_MAXFONTSIZE];
    ArkUI_NumberValue value[] = {{.f32 = maxFontSize}};
    ArkUI_AttributeItem item = {.value = value, .size = 1};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_TEXT_MAX_FONT_SIZE, &item));
    m_initFlag[FLAG_MAXFONTSIZE] = true;
    m_maxFontSize = maxFontSize;
  }
  return *this;
}

TextNode& TextNode::setTextFont(
    float fontSize,
    int32_t fontWeight /*= ARKUI_FONT_WEIGHT_NORMAL*/,
    int32_t fontStyle /*= ARKUI_FONT_STYLE_NORMAL*/,
    const std::string& fontFamily /*= std::string()*/) {
  ArkUI_NumberValue value[] = {
      {.f32 = fontSize}, {.i32 = fontWeight}, {.i32 = fontStyle}};
  ArkUI_AttributeItem item = {
      .value = value, .size = sizeof(value) / sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_FONT, &item));
  return *this;
}

TextNode& TextNode::setTextHeightAdaptivePolicy(int32_t policyType) {
  ArkUI_NumberValue value[] = {{.i32 = policyType}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_HEIGHT_ADAPTIVE_POLICY, &item));
  return *this;
}

TextNode& TextNode::setTextIndent(float textIndent) {
  ArkUI_NumberValue value[] = {{.f32 = textIndent}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INDENT, &item));
  return *this;
}

TextNode& TextNode::setTextEnable(bool enableFlag) {
  if (!m_initFlag[FLAG_ENABLE] || enableFlag != m_enableFlag) {
    VLOG(3) << "[text-debug] setTextEnable flag=" << m_initFlag[FLAG_ENABLE];
    ArkUI_NumberValue value[] = {{.i32 = enableFlag}};
    ArkUI_AttributeItem item = {value, 1};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_ENABLED, &item));
    m_initFlag[FLAG_ENABLE] = true;
    m_enableFlag = enableFlag;
  }
  return *this;
}

TextNode&
TextNode::setPadding(float top, float right, float bottom, float left) {
  if (!m_initFlag[FLAG_PADDING] ||
      !(top == m_top && right == m_right && bottom == m_bottom &&
        left == m_left)) {
    VLOG(3) << "[text-debug] setPadding flag=" << m_initFlag[FLAG_PADDING];
    ArkUI_NumberValue value[] = {
        {.f32 = top}, {.f32 = right}, {.f32 = bottom}, {.f32 = left}};
    ArkUI_AttributeItem item = {.value = value, .size = 4};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_PADDING, &item));
    m_initFlag[FLAG_PADDING] = true;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
    m_left = left;
  }
  return *this;
}

TextNode& TextNode::setWordBreak(ArkUI_WordBreak wordBreak) {
  ArkUI_NumberValue value = {.i32 = wordBreak};
  ArkUI_AttributeItem item = {&value, 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_WORD_BREAK, &item));

  return *this;
}

} // namespace rnoh
