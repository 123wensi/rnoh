#pragma once

#include "ArkUINode.h"
#include "react/renderer/attributedstring/primitives.h"

namespace rnoh {

class TextNode : public ArkUINode {
 private:
  enum {
    FLAG_PADDING = 0,
    FLAG_MINFONTSIZE,
    FLAG_MAXFONTSIZE,
    FLAG_COPYOPTION,
    FLAG_ENABLE,
    FLAG_MAX
  };
  bool m_initFlag[FLAG_MAX] = {0};

  float m_minFontSize = 0.0;
  float m_maxFontSize = 0.0;
  int32_t m_testCopyOption = 0;
  bool m_enableFlag = false;
  float m_top = 0.0;
  float m_right = 0.0;
  float m_bottom = 0.0;
  float m_left = 0.0;

 public:
  TextNode();

  void insertChild(ArkUINode& child, std::size_t index);
  void removeChild(ArkUINode& child);

  TextNode& setTextContent(const std::string& text);
  TextNode& setFontColor(uint32_t fontColor);
  TextNode& resetFontColor();
  TextNode& setFontSize(float fontSize);
  TextNode& setFontStyle(int32_t fontStyle);
  TextNode& setFontWeight(int32_t fontWeight);
  TextNode& setTextLineHeight(float textLineHeight);
  TextNode& setTextDecoration(
      int32_t decorationStyle,
      uint32_t decorationColor = 0xFFFF0000);
  TextNode& setTextCase(int32_t textCase);
  TextNode& setTextLetterSpacing(float textLetterSpacing);
  TextNode& setTextMaxLines(int32_t textMaxLines);
  TextNode& resetTextMaxLines();
  TextNode& setTextAlign(int32_t align);
  TextNode& setTextEllipsisMode(int32_t ellipsisMode);
  TextNode& setTextOverflow(int32_t textOverflow);
  TextNode& setFontFamily(const std::string& fontFamily);
  TextNode& setTextCopyOption(int32_t testCopyOption);
  TextNode& setTextBaselineOffset(float textBaselineOffset);
  TextNode& setTextShadow(
      float textShadowRadius,
      int32_t textShadowType,
      uint32_t textShadowColor,
      float textShadowOffsetX,
      float textShadowOffsetY);
  TextNode& setMinFontSize(float minFontSize);
  TextNode& setMaxFontSize(float maxFontSize);
  TextNode& setTextFont(
      float fontSize,
      int32_t fontWeight = ARKUI_FONT_WEIGHT_NORMAL,
      int32_t fontStyle = ARKUI_FONT_STYLE_NORMAL,
      const std::string& fontFamily = std::string());
  TextNode& setTextHeightAdaptivePolicy(int32_t policyType);
  TextNode& setTextIndent(float textIndent);
  TextNode& setTextEnable(bool enableFlag);
  TextNode& setPadding(float top, float right, float bottom, float left);
  TextNode& setWordBreak(ArkUI_WordBreak wordBreak);
};

} // namespace rnoh