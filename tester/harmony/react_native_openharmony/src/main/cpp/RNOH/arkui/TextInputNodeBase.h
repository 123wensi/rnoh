#pragma once

#include "ArkUINode.h"
#include "react/renderer/components/textinput/TextInputProps.h"
#include "react/renderer/graphics/Color.h"

namespace rnoh {

class TextInputNodeBase : public ArkUINode {
 protected:
  TextInputNodeBase(ArkUI_NodeType nodeType);
  void setCommonFontAttributes(
      facebook::react::TextAttributes const& textAttributes);

 public:
  void setPadding(
      facebook::react::RectangleEdges<facebook::react::Float> padding);

  void setFocusable(bool const& focusable);

  void setAutoFocus(bool autoFocus);

  void setResponseRegion(
      facebook::react::Point const& position,
      facebook::react::Size const& size);

  void setTextContent(std::string const& textContent);

  void setTextSelection(int32_t start, int32_t end);

  void setFontColor(facebook::react::SharedColor const& color);

  virtual void setFont(
      facebook::react::TextAttributes const& textAttributes) = 0;

  virtual void setCaretColor(facebook::react::SharedColor const& color) = 0;

  virtual void setMaxLength(int32_t maxLength) = 0;

  virtual void setPlaceholder(std::string const& placeholder) = 0;

  virtual void setPlaceholderColor(
      facebook::react::SharedColor const& color) = 0;

  void setTextAlign(
      std::optional<facebook::react::TextAlignment> const& textAlign);

  std::string virtual getTextContent() = 0;
};

} // namespace rnoh