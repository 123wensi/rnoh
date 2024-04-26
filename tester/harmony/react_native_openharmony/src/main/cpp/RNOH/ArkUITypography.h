#pragma once
#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_text_typography.h>
#include <react/renderer/graphics/Size.h>
#include <react/renderer/textlayoutmanager/TextLayoutManager.h>
#include <memory>

namespace rnoh {

using UniqueTypographyStyle = std::unique_ptr<
    OH_Drawing_TypographyStyle,
    decltype(&OH_Drawing_DestroyTypographyStyle)>;

class ArkUITypography final {
 public:
  facebook::react::TextMeasurement::Attachments getAttachments() const {
    facebook::react::TextMeasurement::Attachments result;
    result.reserve(m_attachmentCount);
    auto placeholderRects =
        OH_Drawing_TypographyGetRectsForPlaceholders(m_typography.get());

    // calculate attachment sizes and positions
    for (auto i = 0; i < m_attachmentCount; i++) {
      facebook::react::TextMeasurement::Attachment attachment;
      attachment.frame.origin.x =
          OH_Drawing_GetLeftFromTextBox(placeholderRects, i);
      attachment.frame.origin.y =
          OH_Drawing_GetTopFromTextBox(placeholderRects, i);
      attachment.frame.size.width =
          OH_Drawing_GetRightFromTextBox(placeholderRects, i) -
          OH_Drawing_GetLeftFromTextBox(placeholderRects, i);
      attachment.frame.size.height =
          OH_Drawing_GetBottomFromTextBox(placeholderRects, i) -
          OH_Drawing_GetTopFromTextBox(placeholderRects, i);
      result.push_back(std::move(attachment));
    }
    return result;
  }

  facebook::react::Float getHeight() const {
    return OH_Drawing_TypographyGetHeight(m_typography.get());
  }

  facebook::react::Float getLongestLineWidth() const {
    return OH_Drawing_TypographyGetLongestLine(m_typography.get());
  }

  using Rects = std::vector<facebook::react::Rect>;

  std::vector<Rects> getRectsForFragments() const {
    std::vector<Rects> result;
    result.reserve(m_fragmentLengths.size());
    size_t fragmentBegin = 0;
    for (size_t i = 0; i < m_fragmentLengths.size(); i++) {
      auto fragmentEnd = fragmentBegin + m_fragmentLengths[i];
      auto textBoxes = OH_Drawing_TypographyGetRectsForRange(
          m_typography.get(),
          fragmentBegin,
          fragmentEnd,
          RECT_HEIGHT_STYLE_MAX,
          RECT_WIDTH_STYLE_MAX);
      auto textBoxCount = OH_Drawing_GetSizeOfTextBox(textBoxes);
      Rects rects;
      rects.reserve(textBoxCount);
      for (size_t j = 0; j < textBoxCount; j++) {
        facebook::react::Rect rect;
        rect.origin.x = OH_Drawing_GetLeftFromTextBox(textBoxes, j);
        rect.origin.y = OH_Drawing_GetTopFromTextBox(textBoxes, j);
        rect.size.width = OH_Drawing_GetRightFromTextBox(textBoxes, j) -
            OH_Drawing_GetLeftFromTextBox(textBoxes, j);
        rect.size.height = OH_Drawing_GetBottomFromTextBox(textBoxes, j) -
            OH_Drawing_GetTopFromTextBox(textBoxes, j);
        rects.emplace_back(std::move(rect));
      }
      result.emplace_back(std::move(rects));
      fragmentBegin = fragmentEnd;
    }
    return result;
  }

 private:
  ArkUITypography(
      OH_Drawing_TypographyCreate* typographyHandler,
      size_t attachmentCount,
      std::vector<size_t> fragmentLengths,
      facebook::react::Float maxWidth)
      : m_typography(
            OH_Drawing_CreateTypography(typographyHandler),
            OH_Drawing_DestroyTypography),
        m_attachmentCount(attachmentCount),
        m_fragmentLengths(std::move(fragmentLengths)) {
    OH_Drawing_TypographyLayout(m_typography.get(), maxWidth);
  }

  std::
      unique_ptr<OH_Drawing_Typography, decltype(&OH_Drawing_DestroyTypography)>
          m_typography;
  size_t m_attachmentCount;
  std::vector<size_t> m_fragmentLengths;

  friend class ArkUITypographyBuilder;
};

class ArkUITypographyBuilder final {
 public:
  ArkUITypographyBuilder(OH_Drawing_TypographyStyle* typographyStyle)
      : m_typographyHandler(
            OH_Drawing_CreateTypographyHandler(
                typographyStyle,
                OH_Drawing_CreateFontCollection()),
            OH_Drawing_DestroyTypographyHandler) {}

  void setMaximumWidth(facebook::react::Float maximumWidth) {
    if (!isnan(maximumWidth) && maximumWidth > 0) {
      m_maximumWidth = maximumWidth;
    } else {
      m_maximumWidth = std::numeric_limits<facebook::react::Float>::max();
    }
  }

  void addFragment(
      const facebook::react::AttributedString::Fragment& fragment) {
    if (!fragment.isAttachment()) {
      addTextFragment(fragment);
    } else {
      addAttachment(fragment);
    }
  }

  ArkUITypography build() const {
    return ArkUITypography(
        m_typographyHandler.get(),
        m_attachmentCount,
        m_fragmentLengths,
        m_maximumWidth);
  }

 private:
  size_t utf8Length(const std::string& str) {
    size_t length = 0;
    for (auto c : str) {
      if ((c & 0x80) == 0 || (c & 0xc0) == 0xc0) {
        length++;
      }
    }
    return length;
  }

  void addTextFragment(
      const facebook::react::AttributedString::Fragment& fragment) {
    auto textStyle = OH_Drawing_CreateTextStyle();
    auto fontSize = fragment.textAttributes.fontSize;
    if (fontSize <= 0) {
      // set fontSize to default for negative values(same as iOS)
      fontSize = 14;
    }
    OH_Drawing_SetTextStyleFontSize(textStyle, fontSize);
    // new NDK for setting letterSpacing
    if (!isnan(fragment.textAttributes.letterSpacing)) {
      OH_Drawing_SetTextStyleLetterSpacing(
          textStyle, fragment.textAttributes.letterSpacing);
    }
    if (!isnan(fragment.textAttributes.lineHeight) &&
        fragment.textAttributes.lineHeight > 0) {
      // fontSize * fontHeight = lineHeight, no direct ndk for setting
      // lineHeight so do it in this weird way
      double fontHeight = fragment.textAttributes.lineHeight / fontSize;
      OH_Drawing_SetTextStyleFontHeight(textStyle, fontHeight);
    }
    if (fragment.textAttributes.fontWeight.has_value()) {
      OH_Drawing_SetTextStyleFontWeight(
          textStyle,
          mapValueToFontWeight(
              int(fragment.textAttributes.fontWeight.value())));
    }
    if (!fragment.textAttributes.fontFamily.empty()) {
      const char* fontFamilies[] = {fragment.textAttributes.fontFamily.c_str()};
      OH_Drawing_SetTextStyleFontFamilies(textStyle, 1, fontFamilies);
    }
    // push text and corresponding textStyle to handler
    OH_Drawing_TypographyHandlerPushTextStyle(
        m_typographyHandler.get(), textStyle);
    OH_Drawing_TypographyHandlerAddText(
        m_typographyHandler.get(), fragment.string.c_str());
    OH_Drawing_DestroyTextStyle(textStyle);
    m_fragmentLengths.push_back(utf8Length(fragment.string));
  }

  void addAttachment(
      const facebook::react::AttributedString::Fragment& fragment) {
    // using placeholder span for inline views
    OH_Drawing_PlaceholderSpan inlineView = {
        fragment.parentShadowView.layoutMetrics.frame.size.width,
        fragment.parentShadowView.layoutMetrics.frame.size.height,
        // align to the line's baseline
        ALIGNMENT_ABOVE_BASELINE,
        TEXT_BASELINE_ALPHABETIC,
        0,
    };
    // push placeholder to handler
    OH_Drawing_TypographyHandlerAddPlaceholder(
        m_typographyHandler.get(), &inlineView);
    m_attachmentCount++;
  }

  OH_Drawing_FontWeight mapValueToFontWeight(int value) {
    switch (value) {
      case 100:
        return OH_Drawing_FontWeight::FONT_WEIGHT_100;
      case 200:
        return OH_Drawing_FontWeight::FONT_WEIGHT_200;
      case 300:
        return OH_Drawing_FontWeight::FONT_WEIGHT_300;
      case 400:
        return OH_Drawing_FontWeight::FONT_WEIGHT_400;
      case 500:
        return OH_Drawing_FontWeight::FONT_WEIGHT_500;
      case 600:
        return OH_Drawing_FontWeight::FONT_WEIGHT_600;
      case 700:
        return OH_Drawing_FontWeight::FONT_WEIGHT_700;
      case 800:
        return OH_Drawing_FontWeight::FONT_WEIGHT_800;
      case 900:
        return OH_Drawing_FontWeight::FONT_WEIGHT_900;
      default:
        return OH_Drawing_FontWeight::FONT_WEIGHT_400;
    }
  }

  std::unique_ptr<
      OH_Drawing_TypographyCreate,
      decltype(&OH_Drawing_DestroyTypographyHandler)>
      m_typographyHandler;
  size_t m_attachmentCount = 0;
  std::vector<size_t> m_fragmentLengths{};
  facebook::react::Float m_maximumWidth =
      std::numeric_limits<facebook::react::Float>::max();
};

} // namespace rnoh
