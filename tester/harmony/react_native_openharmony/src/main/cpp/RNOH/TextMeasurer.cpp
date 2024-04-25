#include "RNOH/TextMeasurer.h"
#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_text_typography.h>
#include "RNOH/ArkJS.h"
#include "RNOH/ArkUITypography.h"
#include "RNOHCorePackage/ComponentInstances/TextConversions.h"
#include "TextMeasurer.h"

namespace rnoh {

using TextMeasurement = facebook::react::TextMeasurement;
using AttributedString = facebook::react::AttributedString;
using ParagraphAttributes = facebook::react::ParagraphAttributes;
using LayoutConstraints = facebook::react::LayoutConstraints;

TextMeasurement TextMeasurer::measure(
    AttributedString attributedString,
    ParagraphAttributes paragraphAttributes,
    LayoutConstraints layoutConstraints) {
  auto const& fragments = attributedString.getFragments();
  auto canUseOHOSTextMeasurer = fragments.size() == 1 &&
      !fragments[0].isAttachment() &&
      isnan(fragments[0].textAttributes.letterSpacing) &&
      isnan(fragments[0].textAttributes.lineHeight);
  auto isNDKTextMeasuringEnabled =
      this->m_featureFlagRegistry->getFeatureFlagStatus(
          "ENABLE_NDK_TEXT_MEASURING");
  if (canUseOHOSTextMeasurer || isNDKTextMeasuringEnabled) {
    auto typography = measureTypography(
        attributedString, paragraphAttributes, layoutConstraints);
    auto height = typography.getHeight();
    auto longestLineWidth = typography.getLongestLineWidth();
    return {
        {.width = longestLineWidth + 0.5, .height = height},
        typography.getAttachments()};
  } else {
    TextMeasurement result = {{0, 0}, {}};
    m_taskExecutor->runSyncTask(
        TaskThread::MAIN,
        [&result,
         measureTextRef = m_measureTextFnRef,
         env = m_env,
         &attributedString,
         &paragraphAttributes,
         &layoutConstraints]() {
          ArkJS arkJs(env);
          auto napiMeasureText = arkJs.getReferenceValue(measureTextRef);
          auto napiAttributedStringBuilder = arkJs.createObjectBuilder();
          napiAttributedStringBuilder.addProperty(
              "string", attributedString.getString());
          std::vector<napi_value> napiFragments = {};
          for (auto fragment : attributedString.getFragments()) {
            auto textAttributesBuilder = arkJs.createObjectBuilder();
            textAttributesBuilder.addProperty(
                "fontSize", fragment.textAttributes.fontSize);
            textAttributesBuilder.addProperty(
                "lineHeight", fragment.textAttributes.lineHeight);
            if (!fragment.textAttributes.fontFamily.empty()) {
              textAttributesBuilder.addProperty(
                  "fontFamily", fragment.textAttributes.fontFamily);
            }
            textAttributesBuilder.addProperty(
                "letterSpacing", fragment.textAttributes.letterSpacing);
            if (fragment.textAttributes.fontWeight.has_value()) {
              textAttributesBuilder.addProperty(
                  "fontWeight",
                  int(fragment.textAttributes.fontWeight.value()));
            }

            auto napiFragmentBuilder = arkJs.createObjectBuilder();
            napiFragmentBuilder.addProperty("string", fragment.string)
                .addProperty("textAttributes", textAttributesBuilder.build());
            if (fragment.isAttachment()) {
              napiFragmentBuilder.addProperty(
                  "parentShadowView",
                  arkJs.createObjectBuilder()
                      .addProperty("tag", fragment.parentShadowView.tag)
                      .addProperty(
                          "layoutMetrics",
                          arkJs.createObjectBuilder()
                              .addProperty(
                                  "frame",
                                  arkJs.createObjectBuilder()
                                      .addProperty(
                                          "size",
                                          arkJs.createObjectBuilder()
                                              .addProperty(
                                                  "width",
                                                  fragment.parentShadowView
                                                      .layoutMetrics.frame.size
                                                      .width)
                                              .addProperty(
                                                  "height",
                                                  fragment.parentShadowView
                                                      .layoutMetrics.frame.size
                                                      .height)
                                              .build())
                                      .build())
                              .build())
                      .build());
            }

            napiFragments.push_back(napiFragmentBuilder.build());
          }
          napiAttributedStringBuilder.addProperty(
              "fragments", arkJs.createArray(napiFragments));

          auto napiParagraphAttributesBuilder = arkJs.createObjectBuilder();
          napiParagraphAttributesBuilder.addProperty(
              "maximumNumberOfLines", paragraphAttributes.maximumNumberOfLines);

          auto napiLayoutConstraintsBuilder = arkJs.createObjectBuilder();
          napiLayoutConstraintsBuilder.addProperty(
              "maximumSize",
              arkJs.createObjectBuilder()
                  .addProperty("width", layoutConstraints.maximumSize.width)
                  .addProperty("height", layoutConstraints.maximumSize.height)
                  .build());

          auto resultNapiValue = arkJs.call(
              napiMeasureText,
              {napiAttributedStringBuilder.build(),
               napiParagraphAttributesBuilder.build(),
               napiLayoutConstraintsBuilder.build()});

          result.size.width = arkJs.getDouble(arkJs.getObjectProperty(
              arkJs.getObjectProperty(resultNapiValue, "size"), "width"));
          result.size.height = arkJs.getDouble(arkJs.getObjectProperty(
              arkJs.getObjectProperty(resultNapiValue, "size"), "height"));
          auto napiAttachments =
              arkJs.getObjectProperty(resultNapiValue, "attachmentLayouts");
          for (auto i = 0; i < arkJs.getArrayLength(napiAttachments); i++) {
            auto napiAttachment = arkJs.getArrayElement(napiAttachments, i);
            auto napiPositionRelativeToContainer = arkJs.getObjectProperty(
                napiAttachment, "positionRelativeToContainer");
            auto napiSize = arkJs.getObjectProperty(napiAttachment, "size");
            TextMeasurement::Attachment attachment;
            attachment.frame.origin.x = arkJs.getDouble(
                arkJs.getObjectProperty(napiPositionRelativeToContainer, "x"));
            attachment.frame.origin.y = arkJs.getDouble(
                arkJs.getObjectProperty(napiPositionRelativeToContainer, "y"));
            attachment.frame.size.width =
                arkJs.getDouble(arkJs.getObjectProperty(napiSize, "width"));
            attachment.frame.size.height =
                arkJs.getDouble(arkJs.getObjectProperty(napiSize, "height"));
            result.attachments.push_back(attachment);
          }
        });
    return result;
  }
}

ArkUITypography TextMeasurer::measureTypography(
    AttributedString const& attributedString,
    ParagraphAttributes const& paragraphAttributes,
    LayoutConstraints const& layoutConstraints) {
  UniqueTypographyStyle typographyStyle(
      OH_Drawing_CreateTypographyStyle(), OH_Drawing_DestroyTypographyStyle);

  if (paragraphAttributes.maximumNumberOfLines > 0) {
    OH_Drawing_SetTypographyTextMaxLines(
        typographyStyle.get(), paragraphAttributes.maximumNumberOfLines);
  }

  OH_Drawing_SetTypographyTextWordBreakType(
      typographyStyle.get(),
      TextConversions::getArkUIWordBreakStrategy(
          paragraphAttributes.textBreakStrategy));

  if (!attributedString.getFragments().empty()) {
    auto textAlign =
        attributedString.getFragments()[0].textAttributes.alignment;
    if (textAlign.has_value()) {
      OH_Drawing_SetTypographyTextAlign(
          typographyStyle.get(), getOHDrawingTextAlign(textAlign.value()));
    }
  }

  ArkUITypographyBuilder typographyBuilder(typographyStyle.get());
  for (auto const& fragment : attributedString.getFragments()) {
    typographyBuilder.addFragment(fragment);
  }
  typographyBuilder.setMaximumWidth(layoutConstraints.maximumSize.width);
  return typographyBuilder.build();
}

int32_t TextMeasurer::getOHDrawingTextAlign(
    const facebook::react::TextAlignment& textAlign) {
  int32_t align = OH_Drawing_TextAlign::TEXT_ALIGN_START;
  switch (textAlign) {
    case facebook::react::TextAlignment::Natural:
    case facebook::react::TextAlignment::Left:
      align = OH_Drawing_TextAlign::TEXT_ALIGN_START;
      break;
    case facebook::react::TextAlignment::Right:
      align = OH_Drawing_TextAlign::TEXT_ALIGN_END;
      break;
    case facebook::react::TextAlignment::Center:
      align = OH_Drawing_TextAlign::TEXT_ALIGN_CENTER;
      break;
    case facebook::react::TextAlignment::Justified:
      align = OH_Drawing_TextAlign::TEXT_ALIGN_JUSTIFY;
      break;
    default:
      break;
  }
  return align;
}

} // namespace rnoh
