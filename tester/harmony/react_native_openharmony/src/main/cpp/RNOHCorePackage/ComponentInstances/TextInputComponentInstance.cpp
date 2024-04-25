#include "TextInputComponentInstance.h"
#include "RNOH/arkui/conversions.h"
#include "conversions.h"

#include <folly/dynamic.h>
#include <glog/logging.h>
#include <react/renderer/components/textinput/TextInputProps.h>
#include <react/renderer/components/textinput/TextInputState.h>
#include <react/renderer/core/ConcreteState.h>
#include <sstream>
#include <utility>

namespace rnoh {

TextInputComponentInstance::TextInputComponentInstance(Context context)
    : CppComponentInstance(std::move(context)) {
  m_textInputNode.setTextInputNodeDelegate(this);
  m_textAreaNode.setTextAreaNodeDelegate(this);
}

void TextInputComponentInstance::onChange(std::string text) {
  m_valueChanged = true;
  m_content = std::move(text);
}

void TextInputComponentInstance::onSubmit() {
  m_eventEmitter->onSubmitEditing(getTextInputMetrics());
}

void TextInputComponentInstance::onBlur() {
  this->m_focused = false;
  if (m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
    m_textInputNode.setCancelButtonMode(
        facebook::react::TextInputAccessoryVisibilityMode::Never);
  } else if (
      m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
    m_textInputNode.setCancelButtonMode(
        facebook::react::TextInputAccessoryVisibilityMode::Always);
  }
  m_eventEmitter->onBlur(getTextInputMetrics());
  m_eventEmitter->onEndEditing(getTextInputMetrics());
}

void TextInputComponentInstance::onFocus() {
  this->m_focused = true;
  if (this->m_clearTextOnFocus) {
    m_textAreaNode.setTextContent("");
    m_textInputNode.setTextContent("");
  }
  if (m_props->traits.selectTextOnFocus) {
    m_textInputNode.setTextSelection(0, m_content.size());
    m_textAreaNode.setTextSelection(0, m_content.size());
  }
  if (m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
    m_textInputNode.setCancelButtonMode(m_props->traits.clearButtonMode);
  } else if (
      m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
    m_textInputNode.setCancelButtonMode(
        facebook::react::TextInputAccessoryVisibilityMode::Never);
  }
  m_eventEmitter->onFocus(getTextInputMetrics());
}

void TextInputComponentInstance::onPasteOrCut() {
  m_textWasPastedOrCut = true;
}

void TextInputComponentInstance::onTextSelectionChange(
    int32_t location,
    int32_t length) {
  if (m_textWasPastedOrCut) {
    m_textWasPastedOrCut = false;
  } else if (m_valueChanged) {
    std::string key;
    bool noPreviousSelection = m_selectionLength == 0;
    bool cursorDidNotMove = location == m_selectionLocation;
    bool cursorMovedBackwardsOrAtBeginningOfInput =
        (location < m_selectionLocation) || location <= 0;
    if (!cursorMovedBackwardsOrAtBeginningOfInput &&
        (noPreviousSelection || !cursorDidNotMove)) {
      key = m_content.at(location - 1);
    }
    auto keyPressMetrics = facebook::react::KeyPressMetrics();
    keyPressMetrics.text = key;
    keyPressMetrics.eventCount = m_nativeEventCount;
    m_eventEmitter->onKeyPress(keyPressMetrics);
  }
  if (m_valueChanged) {
    m_valueChanged = false;
    m_nativeEventCount++;
    m_eventEmitter->onChange(getTextInputMetrics());
  }

  m_selectionLocation = location;
  m_selectionLength = length;
  m_eventEmitter->onSelectionChange(getTextInputMetrics());
}

facebook::react::TextInputMetrics
TextInputComponentInstance::getTextInputMetrics() {
  auto textInputMetrics = facebook::react::TextInputMetrics();
  textInputMetrics.contentOffset = m_multiline
      ? m_textAreaNode.getTextAreaOffset()
      : m_textInputNode.getTextInputOffset();
  textInputMetrics.containerSize = m_layoutMetrics.frame.size;

  textInputMetrics.eventCount = this->m_nativeEventCount;
  textInputMetrics.selectionRange.location = this->m_selectionLocation;
  textInputMetrics.selectionRange.length = this->m_selectionLength;
  textInputMetrics.zoomScale = 1;
  textInputMetrics.text = this->m_content;
  return textInputMetrics;
}

void TextInputComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  m_multiline = props->traits.multiline;
  CppComponentInstance::onPropsChanged(props);
  m_clearTextOnFocus = props->traits.clearTextOnFocus;

  if (!m_props ||
      *(props->textAttributes.foregroundColor) !=
          *(m_props->textAttributes.foregroundColor)) {
    if (props->textAttributes.foregroundColor) {
      m_textAreaNode.setFontColor(props->textAttributes.foregroundColor);
      m_textInputNode.setFontColor(props->textAttributes.foregroundColor);
    } else {
      m_textAreaNode.setFontColor(facebook::react::blackColor());
      m_textInputNode.setFontColor(facebook::react::blackColor());
    }
  }
  if (!m_props || props->textAttributes != m_props->textAttributes) {
    m_textAreaNode.setFont(props->textAttributes);
    m_textInputNode.setFont(props->textAttributes);
  }
  if (!m_props || *(props->backgroundColor) != *(m_props->backgroundColor)) {
    if (props->backgroundColor) {
      m_textAreaNode.setBackgroundColor(props->backgroundColor);
      m_textInputNode.setBackgroundColor(props->backgroundColor);
    } else {
      m_textAreaNode.setBackgroundColor(facebook::react::clearColor());
      m_textInputNode.setBackgroundColor(facebook::react::clearColor());
    }
  }
  if (props->textAttributes.alignment) {
    if (!m_props ||
        *(props->textAttributes.alignment) !=
            *(m_props->textAttributes.alignment)) {
      m_textAreaNode.setTextAlign(props->textAttributes.alignment);
      m_textInputNode.setTextAlign(props->textAttributes.alignment);
    }
  }
  if (!m_props || *(props->cursorColor) != *(m_props->cursorColor)) {
    if (props->cursorColor) {
      m_textAreaNode.setCaretColor(props->cursorColor);
      m_textInputNode.setCaretColor(props->cursorColor);
    } else {
      m_textAreaNode.setCaretColor(facebook::react::blackColor());
      m_textInputNode.setCaretColor(facebook::react::blackColor());
    }
  }
  if (!m_props || props->traits.editable != m_props->traits.editable) {
    m_textAreaNode.setEnabled(props->traits.editable);
    m_textInputNode.setEnabled(props->traits.editable);
  }
  if (!m_props || props->traits.keyboardType != m_props->traits.keyboardType) {
    m_textInputNode.setInputType(
        props->traits.secureTextEntry
            ? ARKUI_TEXTINPUT_TYPE_PASSWORD
            : rnoh::convertInputType(props->traits.keyboardType));
  }
  if (props->maxLength != 0) {
    if (!m_props || props->maxLength != m_props->maxLength) {
      m_textAreaNode.setMaxLength(props->maxLength);
      m_textInputNode.setMaxLength(props->maxLength);
    }
  }
  if (!m_props || props->placeholder != m_props->placeholder) {
    m_textAreaNode.setPlaceholder(props->placeholder);
    m_textInputNode.setPlaceholder(props->placeholder);
  }
  if (props->placeholderTextColor) {
    if (!m_props ||
        *(props->placeholderTextColor) != *(m_props->placeholderTextColor)) {
      m_textAreaNode.setPlaceholderColor(props->placeholderTextColor);
      m_textInputNode.setPlaceholderColor(props->placeholderTextColor);
    }
  }
  if (props->rawProps.count("focusable") > 0) {
    if (!m_props ||
        props->rawProps["focusable"].asBool() !=
            m_props->rawProps["focusable"].asBool()) {
      m_textAreaNode.setFocusable(props->rawProps["focusable"].asBool());
      m_textInputNode.setFocusable(props->rawProps["focusable"].asBool());
    }
  }
  m_textAreaNode.setId(getIdFromProps(props));
  m_textInputNode.setId(getIdFromProps(props));

  if (!m_props || props->autoFocus != m_props->autoFocus) {
    m_textAreaNode.setAutoFocus(props->autoFocus);
    m_textInputNode.setAutoFocus(props->autoFocus);
  }
  if (!m_props || *(props->selectionColor) != *(m_props->selectionColor)) {
    if (props->selectionColor) {
      m_textInputNode.setSelectedBackgroundColor(props->selectionColor);
      if (!props->cursorColor) {
        m_textInputNode.setCaretColor(props->selectionColor);
      }
    } else {
      m_textInputNode.resetSelectedBackgroundColor();
    }
  }
  if (!m_props ||
      props->traits.secureTextEntry != m_props->traits.secureTextEntry ||
      props->traits.keyboardType != m_props->traits.keyboardType) {
    m_textInputNode.setInputType(
        props->traits.secureTextEntry
            ? ARKUI_TEXTINPUT_TYPE_PASSWORD
            : rnoh::convertInputType(props->traits.keyboardType));
  }
  if (!m_props || props->traits.caretHidden != m_props->traits.caretHidden) {
    m_textInputNode.setCaretHidden(props->traits.caretHidden);
  }
  if (!m_props ||
      props->traits.returnKeyType != m_props->traits.returnKeyType) {
    m_textInputNode.setEnterKeyType(props->traits.returnKeyType);
  }
  if (!m_props ||
      props->traits.clearButtonMode != m_props->traits.clearButtonMode) {
    if (m_focused) {
      if (props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
        m_textInputNode.setCancelButtonMode(props->traits.clearButtonMode);
      } else if (
          props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
        m_textInputNode.setCancelButtonMode(
            facebook::react::TextInputAccessoryVisibilityMode::Never);
      }
    } else {
      if (props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
        m_textInputNode.setCancelButtonMode(
            facebook::react::TextInputAccessoryVisibilityMode::Never);
      } else if (
          props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
        m_textInputNode.setCancelButtonMode(
            facebook::react::TextInputAccessoryVisibilityMode::Always);
      }
    }

    if (props->traits.clearButtonMode ==
            facebook::react::TextInputAccessoryVisibilityMode::Always ||
        props->traits.clearButtonMode ==
            facebook::react::TextInputAccessoryVisibilityMode::Never) {
      m_textInputNode.setCancelButtonMode(props->traits.clearButtonMode);
    }
  }
  if (!m_props ||
      !(props->yogaStyle.padding() == m_props->yogaStyle.padding())) {
    m_textInputNode.setPadding(resolveEdges(props->yogaStyle.padding()));
    m_textAreaNode.setPadding(resolveEdges(props->yogaStyle.padding()));
  }
}

void TextInputComponentInstance::setLayout(
    facebook::react::LayoutMetrics layoutMetrics) {
  CppComponentInstance::setLayout(layoutMetrics);
  if (m_multiline) {
    m_textInputNode.setPosition(layoutMetrics.frame.origin);
    m_textInputNode.setSize(layoutMetrics.frame.size);
  } else {
    m_textAreaNode.setPosition(layoutMetrics.frame.origin);
    m_textAreaNode.setSize(layoutMetrics.frame.size);
  }
}

void TextInputComponentInstance::handleCommand(
    std::string const& commandName,
    folly::dynamic const& args) {
  if (commandName == "focus") {
    focus();
  } else if (commandName == "blur") {
    blur();
  } else if (
      commandName == "setTextAndSelection" && args.isArray() &&
      args.size() == 4 && args[0].asInt() >= m_nativeEventCount) {
    m_textInputNode.setTextContent(args[1].asString());
    m_textAreaNode.setTextContent(args[1].asString());
    if (args[2].asInt() >= 0 && args[3].asInt() >= 0) {
      m_textInputNode.setTextSelection(args[2].asInt(), args[3].asInt());
      m_textAreaNode.setTextSelection(args[2].asInt(), args[3].asInt());
    }
  }
}

void TextInputComponentInstance::onStateChanged(
    SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);

  if (state->getData().mostRecentEventCount < this->m_nativeEventCount) {
    return;
  }

  std::ostringstream contentStream;
  for (auto const& fragment :
       state->getData().attributedStringBox.getValue().getFragments()) {
    contentStream << fragment.string;
  }
  auto content = contentStream.str();
  m_textAreaNode.setTextContent(content);
  m_textInputNode.setTextContent(content);
}

ArkUINode& TextInputComponentInstance::getLocalRootArkUINode() {
  if (m_multiline) {
    return m_textAreaNode;
  }
  return m_textInputNode;
}

void TextInputComponentInstance::focus() {
  getLocalRootArkUINode().setFocusStatus(1);
}

void TextInputComponentInstance::blur() {
  getLocalRootArkUINode().setFocusStatus(0);
}

} // namespace rnoh
