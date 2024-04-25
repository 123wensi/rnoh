//
// Created on 2024/3/19.
//
// Node APIs are not fully supported. To solve the compilation error of the
// interface cannot be found, please include "napi/native_api.h".

#include "TextConversions.h"
#include "react/renderer/components/view/conversions.h"

using namespace rnoh;

int32_t TextConversions::getArkUIFontWeight(int32_t fontWeight) {
  int32_t data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL;
  switch (fontWeight) {
    case (int)facebook::react::FontWeight::Weight100:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W100;
      break;
    case (int)facebook::react::FontWeight::Weight200:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W200;
      break;
    case (int)facebook::react::FontWeight::Weight300:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W300;
      break;
    case (int)facebook::react::FontWeight::Weight400:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W400;
      break;
    case (int)facebook::react::FontWeight::Weight500:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W500;
      break;
    case (int)facebook::react::FontWeight::Weight600:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W600;
      break;
    case (int)facebook::react::FontWeight::Weight700:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W700;
      break;
    case (int)facebook::react::FontWeight::Weight800:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W800;
      break;
    case (int)facebook::react::FontWeight::Weight900:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W900;
      break;
    default:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL;
  }
  return data;
}

int32_t TextConversions::getArkUITextCase(
    const facebook::react::TextTransform& textTransform) {
  int textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL;
  switch (textTransform) {
    case facebook::react::TextTransform::None:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL;
      break;
    case facebook::react::TextTransform::Uppercase:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_UPPER;
      break;
    case facebook::react::TextTransform::Lowercase:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_LOWER;
      break;
    default:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL;
      break;
  }
  return textCase;
}

int32_t TextConversions::getArkUIEllipsizeMode(
    const facebook::react::EllipsizeMode& ellipsizeMode) {
  int arkEllipsizeMode = -1;
  switch (ellipsizeMode) {
    case facebook::react::EllipsizeMode::Head:
      arkEllipsizeMode = ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_START;
      break;
    case facebook::react::EllipsizeMode::Tail:
      arkEllipsizeMode = ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_END;
      break;
    case facebook::react::EllipsizeMode::Middle:
      arkEllipsizeMode = ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_CENTER;
      break;
    default:
      arkEllipsizeMode = -1;
      break;
  }
  return arkEllipsizeMode;
}

std::pair<std::optional<float>, std::optional<float>>
TextConversions::calcHorizontalPadding(
    YGStyle::Edges const& yogaPadding,
    bool isRTL) {
  std::optional<float> left = std::nullopt;
  std::optional<float> right = std::nullopt;
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeLeft])
          .has_value()) {
    left = facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeLeft])
               .value();
  }
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeRight])
          .has_value()) {
    right =
        facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeRight])
            .value();
  }
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeStart])
          .has_value()) {
    float paddingStart =
        facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeStart])
            .value();
    isRTL ? (right = paddingStart) : (left = paddingStart);
  }
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeEnd])
          .has_value()) {
    float paddingEnd =
        facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeEnd])
            .value();
    isRTL ? (left = paddingEnd) : (right = paddingEnd);
  }
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeHorizontal])
          .has_value()) {
    float paddingHorizontal = facebook::react::optionalFloatFromYogaValue(
                                  yogaPadding[YGEdgeHorizontal])
                                  .value();
    left = right = paddingHorizontal;
  }
  return std::make_pair(left, right);
};

std::pair<std::optional<float>, std::optional<float>>
TextConversions::calcVerticalPadding(YGStyle::Edges const& yogaPadding) {
  std::optional<float> top = std::nullopt;
  std::optional<float> bottom = std::nullopt;
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeTop])
          .has_value()) {
    top = facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeTop])
              .value();
  }
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeBottom])
          .has_value()) {
    bottom =
        facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeBottom])
            .value();
  }
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeVertical])
          .has_value()) {
    float paddingVertical =
        facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeVertical])
            .value();
    top = bottom = paddingVertical;
  }
  return std::make_pair(top, bottom);
};

TextPaddingInfo TextConversions::getArkUIPadding(
    std::shared_ptr<const facebook::react::ParagraphProps> props) {
  TextPaddingInfo info;
  auto yogaPadding = props->yogaStyle.padding();
  bool isRTL = false; // 标识布局是否为从右到左，当前无法获取。
  if (facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeAll])
          .has_value()) {
    float padding =
        facebook::react::optionalFloatFromYogaValue(yogaPadding[YGEdgeAll])
            .value();
    info.top = info.right = info.bottom = info.left = padding;
  }
  auto horizonPadding = calcHorizontalPadding(yogaPadding, isRTL);
  auto verticalPadding = calcVerticalPadding(yogaPadding);
  info.left = horizonPadding.first.has_value() ? horizonPadding.first.value()
                                               : info.left;
  info.right = horizonPadding.second.has_value() ? horizonPadding.second.value()
                                                 : info.right;
  info.top = verticalPadding.first.has_value() ? verticalPadding.first.value()
                                               : info.top;
  info.bottom = verticalPadding.second.has_value()
      ? verticalPadding.second.value()
      : info.bottom;
  return info;
}

int32_t TextConversions::getArkUITextAlignment(
    const facebook::react::TextAlignment& textAlign) {
  int32_t align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_START;
  switch (textAlign) {
    case facebook::react::TextAlignment::Natural:
    case facebook::react::TextAlignment::Left:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_START;
      break;
    case facebook::react::TextAlignment::Right:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_END;
      break;
    case facebook::react::TextAlignment::Center:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_CENTER;
      break;
    case facebook::react::TextAlignment::Justified:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_JUSTIFY;
      break;
    default:
      break;
  }
  return align;
}

std::optional<float> TextConversions::getMinFontSize(
    std::shared_ptr<const facebook::react::ParagraphProps> props) {
  if (props->rawProps == nullptr ||
      props->rawProps.count("adjustsFontSizeToFit") == 0 ||
      props->rawProps.count("minimumFontScale") == 0) {
    return std::nullopt;
  }
  bool adjustsFontSizeToFit = props->rawProps["adjustsFontSizeToFit"].asBool();
  double minimumFontScale = props->rawProps["minimumFontScale"].asDouble();
  float fontSize = isnan(props->textAttributes.fontSize)
      ? DEFAULT_FONT_SIZE
      : (float)props->textAttributes.fontSize;
  if (fabs((float)minimumFontScale) < EPSILON_THRESHOLD ||
      fabs(fontSize) < EPSILON_THRESHOLD || !adjustsFontSizeToFit) {
    return std::nullopt;
  }
  return fontSize * minimumFontScale;
}

std::optional<float> TextConversions::getMaxFontSize(
    std::shared_ptr<const facebook::react::ParagraphProps> props) {
  float fontSize = isnan(props->textAttributes.fontSize)
      ? DEFAULT_FONT_SIZE
      : (float)props->textAttributes.fontSize;
  if (fabs(fontSize) < EPSILON_THRESHOLD) {
    return std::nullopt;
  }
  return fontSize;
}

int32_t TextConversions::getStackAlign(
    const facebook::react::TextAlignment& alignHorizon,
    const std::string& alignVertical) {
  const static std::map<
      std::string,
      std::map<facebook::react::TextAlignment, ArkUI_Alignment>>
      convertMap{
          {"auto",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
           }},
          {"top",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
           }},
          {"center",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_CENTER},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_START},
           }},
          {"bottom",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START},
           }}};
  if (convertMap.find(alignVertical) == convertMap.end()) {
    return ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START;
  }
  return convertMap.at(alignVertical).at(alignHorizon);
}

ArkUI_WordBreak TextConversions::getArkUIWordBreakStrategy(
    facebook::react::TextBreakStrategy textBreakStrategy) {
  switch (textBreakStrategy) {
    case facebook::react::TextBreakStrategy::Simple:
      return ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_ALL;
    case facebook::react::TextBreakStrategy::HighQuality:
      return ArkUI_WordBreak::ARKUI_WORD_BREAK_NORMAL;
    case facebook::react::TextBreakStrategy::Balanced:
      return ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_WORD;
  }
}