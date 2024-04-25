#include "TextAreaNode.h"
#include <bits/alltypes.h>
#include "NativeNodeApi.h"
#include "RNOH/arkui/conversions.h"

static constexpr std::array TEXT_AREA_NODE_EVENT_TYPES = {
    NODE_TEXT_AREA_ON_PASTE,
    NODE_TEXT_AREA_ON_CHANGE,
    NODE_TEXT_INPUT_ON_CUT,
    NODE_ON_FOCUS,
    NODE_ON_BLUR,
    NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE};

namespace rnoh {

TextAreaNode::TextAreaNode()
    : TextInputNodeBase(ArkUI_NodeType::ARKUI_NODE_TEXT_AREA),
      m_textAreaNodeDelegate(nullptr) {
  for (auto eventType : TEXT_AREA_NODE_EVENT_TYPES) {
    maybeThrow(NativeNodeApi::getInstance()->registerNodeEvent(
        m_nodeHandle, eventType, eventType, this));
    // NODE_TEXT_AREA_ENABLE_KEYBOARD_ON_FOCUS missing in C-API
  }
}

TextAreaNode::~TextAreaNode() {
  for (auto eventType : TEXT_AREA_NODE_EVENT_TYPES) {
    NativeNodeApi::getInstance()->unregisterNodeEvent(m_nodeHandle, eventType);
  }
}

void TextAreaNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  if (eventType == ArkUI_NodeEventType::NODE_ON_FOCUS) {
    if (m_textAreaNodeDelegate != nullptr) {
      m_textAreaNodeDelegate->onFocus();
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_ON_BLUR) {
    if (m_textAreaNodeDelegate != nullptr) {
      m_textAreaNodeDelegate->onBlur();
    }
  } else if (
      eventType ==
      ArkUI_NodeEventType::NODE_TEXT_AREA_ON_TEXT_SELECTION_CHANGE) {
    if (m_textAreaNodeDelegate != nullptr) {
      int32_t selectionLocation = eventArgs[0].i32;
      int32_t selectionLength = eventArgs[1].i32 - eventArgs[0].i32;
      m_textAreaNodeDelegate->onTextSelectionChange(
          selectionLocation, selectionLength);
    }
  }
}

void TextAreaNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    std::string_view eventString) {
  if (eventType == ArkUI_NodeEventType::NODE_TEXT_AREA_ON_CHANGE) {
    if (m_textAreaNodeDelegate != nullptr) {
      std::string text(eventString);
      m_textAreaNodeDelegate->onChange(std::move(text));
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_TEXT_AREA_ON_PASTE) {
    if (m_textAreaNodeDelegate != nullptr) {
      m_textAreaNodeDelegate->onPasteOrCut();
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CUT) {
    if (m_textAreaNodeDelegate != nullptr) {
      m_textAreaNodeDelegate->onPasteOrCut();
    }
  }
  // NODE_TEXT_AREA_ON_CUT missing
}

void TextAreaNode::setTextAreaNodeDelegate(
    TextAreaNodeDelegate* textAreaNodeDelegate) {
  m_textAreaNodeDelegate = textAreaNodeDelegate;
}

facebook::react::Point TextAreaNode::getTextAreaOffset() const {
  auto value = NativeNodeApi::getInstance()
                   ->getAttribute(m_nodeHandle, NODE_OFFSET)
                   ->value;
  facebook::react::Float x = value[0].i32;
  facebook::react::Float y = value[1].i32;

  return facebook::react::Point{x, y};
}

void TextAreaNode::setTextContent(std::string const& textContent) {
  ArkUI_AttributeItem item = {.string = textContent.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_TEXT, &item));
}

void TextAreaNode::setInputType(ArkUI_TextInputType keyboardType) {
  ArkUI_NumberValue value = {.i32 = keyboardType};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_TYPE, &item));
}

void TextAreaNode::setFont(
    facebook::react::TextAttributes const& textAttributes) {
  TextInputNodeBase::setCommonFontAttributes(textAttributes);

  std::string fontFamily = "HarmonyOS Sans";
  if (!textAttributes.fontFamily.empty()) {
    fontFamily = textAttributes.fontFamily;
  }

  ArkUI_NumberValue fontWeight = {.i32 = ARKUI_FONT_WEIGHT_NORMAL};
  if (textAttributes.fontWeight) {
    fontWeight =
        rnoh::convertFontWeight((int32_t)textAttributes.fontWeight.value());
  }

  ArkUI_NumberValue tempStyle = {.i32 = ARKUI_FONT_STYLE_NORMAL};
  if (textAttributes.fontStyle == facebook::react::FontStyle::Italic) {
    tempStyle = {.i32 = ARKUI_FONT_STYLE_ITALIC};
  }

  std::array<ArkUI_NumberValue, 3> value = {
      {{.f32 = static_cast<float>(textAttributes.fontSize)},
       tempStyle,
       fontWeight}};
  ArkUI_AttributeItem item = {value.data(), value.size(), fontFamily.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_PLACEHOLDER_FONT, &item));
}

void TextAreaNode::setCaretColor(facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  ArkUI_NumberValue value = {.u32 = colorValue};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_CARET_COLOR, &item));
}

void TextAreaNode::setMaxLength(int32_t maxLength) {
  ArkUI_NumberValue value = {.i32 = maxLength};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_MAX_LENGTH, &item));
}

void TextAreaNode::setPlaceholder(std::string const& placeholder) {
  ArkUI_AttributeItem item = {.string = placeholder.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_PLACEHOLDER, &item));
}

void TextAreaNode::setPlaceholderColor(
    facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  ArkUI_NumberValue value = {.u32 = colorValue};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_AREA_PLACEHOLDER_COLOR, &item));
}

void TextAreaNode::defaultSetPadding() {
  ArkUI_NumberValue value = {.f32 = 0.f};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_PADDING, &item));
}

std::string TextAreaNode::getTextContent() {
  auto item = NativeNodeApi::getInstance()->getAttribute(
      m_nodeHandle, NODE_TEXT_AREA_TEXT);
  return item->string;
}
} // namespace rnoh
