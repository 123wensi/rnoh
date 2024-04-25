#include "TextInputNode.h"
#include "NativeNodeApi.h"
#include "RNOH/arkui/conversions.h"

static constexpr std::array TEXT_INPUT_NODE_EVENT_TYPES = {
    NODE_TEXT_INPUT_ON_PASTE,
    NODE_TEXT_INPUT_ON_CUT,
    NODE_TEXT_INPUT_ON_CHANGE,
    NODE_TEXT_INPUT_ON_SUBMIT,
    NODE_ON_FOCUS,
    NODE_ON_BLUR,
    NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE};

namespace rnoh {

TextInputNode::TextInputNode()
    : TextInputNodeBase(ArkUI_NodeType::ARKUI_NODE_TEXT_INPUT),
      m_textInputNodeDelegate(nullptr) {
  for (auto eventType : TEXT_INPUT_NODE_EVENT_TYPES) {
    maybeThrow(NativeNodeApi::getInstance()->registerNodeEvent(
        m_nodeHandle, eventType, eventType, this));
  }

  ArkUI_NumberValue value = {.i32 = 1};
  ArkUI_AttributeItem item = {&value, 1};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_ENABLE_KEYBOARD_ON_FOCUS, &item));
}

TextInputNode::~TextInputNode() {
  for (auto eventType : TEXT_INPUT_NODE_EVENT_TYPES) {
    NativeNodeApi::getInstance()->unregisterNodeEvent(m_nodeHandle, eventType);
  }
}

void TextInputNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_SUBMIT) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onSubmit();
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_ON_FOCUS) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onFocus();
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_ON_BLUR) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onBlur();
    }
  } else if (
      eventType ==
      ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE) {
    if (m_textInputNodeDelegate != nullptr) {
      int32_t selectionLocation = eventArgs[0].i32;
      int32_t selectionLength = eventArgs[1].i32 - eventArgs[0].i32;
      m_textInputNodeDelegate->onTextSelectionChange(
          selectionLocation, selectionLength);
    }
  }
}

void TextInputNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    std::string_view eventString) {
  if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CHANGE) {
    if (m_textInputNodeDelegate != nullptr) {
      std::string text(eventString);
      m_textInputNodeDelegate->onChange(std::move(text));
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_PASTE) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onPasteOrCut();
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CUT) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onPasteOrCut();
    }
  }
}

void TextInputNode::setTextInputNodeDelegate(
    TextInputNodeDelegate* textInputNodeDelegate) {
  m_textInputNodeDelegate = textInputNodeDelegate;
}

facebook::react::Point TextInputNode::getTextInputOffset() const {
  auto value = NativeNodeApi::getInstance()
                   ->getAttribute(m_nodeHandle, NODE_OFFSET)
                   ->value;
  facebook::react::Float x = value->i32;
  value++;
  facebook::react::Float y = value->i32;
  return facebook::react::Point{x, y};
}

void TextInputNode::setTextContent(std::string const& textContent) {
  ArkUI_AttributeItem item = {.string = textContent.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_TEXT, &item));
}

void TextInputNode::setSelectedBackgroundColor(
    facebook::react::SharedColor const& color) {
  ArkUI_NumberValue selectedBackgroundColor = {
      .u32 = rnoh::convertColorToTranslucent(color)};
  ArkUI_AttributeItem colorItem = {
      &selectedBackgroundColor, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR, &colorItem));
}

void TextInputNode::setCaretHidden(bool hidden) {
  if (hidden) {
    ArkUI_NumberValue value = {.f32 = 0};
    ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_TEXT_INPUT_CARET_STYLE, &item));
  } else {
    maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
        m_nodeHandle, NODE_TEXT_INPUT_CARET_STYLE));
  }
}

void TextInputNode::setInputType(ArkUI_TextInputType keyboardType) {
  ArkUI_NumberValue value = {.i32 = keyboardType};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_TYPE, &item));
}

void TextInputNode::setPasswordIconVisibility(bool isVisible) {
  ArkUI_NumberValue value = {.i32 = isVisible ? 1 : 0};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SHOW_PASSWORD_ICON, &item));
}

void TextInputNode::setEnterKeyType(
    facebook::react::ReturnKeyType returnKeyType) {
  ArkUI_NumberValue value = {.i32 = rnoh::convertEnterKeyType(returnKeyType)};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_ENTER_KEY_TYPE, &item));
}

void TextInputNode::setCancelButtonMode(
    facebook::react::TextInputAccessoryVisibilityMode mode) {
  int32_t cancelButtonStyle =
      static_cast<int32_t>(ARKUI_CANCELBUTTON_STYLE_INVISIBLE);
  if (mode == facebook::react::TextInputAccessoryVisibilityMode::Always) {
    cancelButtonStyle = static_cast<int32_t>(ARKUI_CANCELBUTTON_STYLE_CONSTANT);
  } else if (
      mode == facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
    cancelButtonStyle = static_cast<int32_t>(ARKUI_CANCELBUTTON_STYLE_INPUT);
  }

  ArkUI_NumberValue value[] = {{.i32 = cancelButtonStyle}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};

  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_CANCEL_BUTTON, &item));
}

void TextInputNode::setFont(
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
      m_nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER_FONT, &item));
}

void TextInputNode::setCaretColor(facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  ArkUI_NumberValue value = {.u32 = colorValue};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_CARET_COLOR, &item));
}

void TextInputNode::setMaxLength(int32_t maxLength) {
  ArkUI_NumberValue value = {.i32 = maxLength};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_MAX_LENGTH, &item));
}

void TextInputNode::setPlaceholder(std::string const& placeholder) {
  ArkUI_AttributeItem item = {.string = placeholder.c_str()};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER, &item));
}

void TextInputNode::setPlaceholderColor(
    facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  ArkUI_NumberValue value = {.u32 = colorValue};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER_COLOR, &item));
}

void TextInputNode::resetSelectedBackgroundColor() {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR));
}

std::string TextInputNode::getTextContent() {
  auto item = NativeNodeApi::getInstance()->getAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_TEXT);
  return item->string;
}

} // namespace rnoh
