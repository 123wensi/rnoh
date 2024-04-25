#include "TextComponentInstance.h"
#include <glog/logging.h>
#include <react/renderer/components/text/ParagraphProps.h>
#include <react/renderer/components/text/ParagraphState.h>
#include <react/renderer/core/ConcreteState.h>
#include <sstream>
#include "RNOH/TextMeasurer.h"
#include "TextConversions.h"
#include "react/renderer/attributedstring/primitives.h"
#include "react/renderer/components/view/conversions.h"

namespace rnoh {
const static float DEFAULT_LINE_SPACING = 0.15f;

TextComponentInstance::TextComponentInstance(Context context)
    : CppComponentInstance(std::move(context)) {
  m_stackNode.insertChild(m_textNode, 0);
  m_childNodes.clear();
}

TextComponentInstance::~TextComponentInstance() {
  for (auto const& item : m_childNodes) {
    m_textNode.removeChild(*item);
  }
  m_childNodes.clear();
}

void TextComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  m_stackNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void TextComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  m_stackNode.removeChild(childComponentInstance->getLocalRootArkUINode());
}

void TextComponentInstance::onPropsChanged(
    SharedConcreteProps const& textProps) {
  CppComponentInstance::onPropsChanged(textProps);
  if (textProps == nullptr) {
    return;
  }
  // padding
  TextPaddingInfo info = TextConversions::getArkUIPadding(textProps);
  VLOG(3) << "[text-debug] textProps->setPadding top=" << info.top
          << ", right=" << info.right << ", bottom=" << info.bottom
          << ", left=" << info.left;
  m_textNode.setPadding(info.top, info.right, info.bottom, info.left);

  // Copy Option
  VLOG(3) << "[text-debug] textProps->isSelectable=" << textProps->isSelectable;
  int32_t testCopyOption = ArkUI_CopyOptions::ARKUI_COPY_OPTIONS_NONE;
  if (textProps->isSelectable) {
    testCopyOption = ArkUI_CopyOptions::ARKUI_COPY_OPTIONS_LOCAL_DEVICE;
  }
  m_textNode.setTextCopyOption(testCopyOption);

  // MaxFontSize
  std::optional<float> maxFontSize = TextConversions::getMaxFontSize(textProps);
  if (maxFontSize.has_value()) {
    m_textNode.setMaxFontSize(maxFontSize.value());
    VLOG(3) << "[text-debug] maxFontSize=" << maxFontSize.value();
  }

  if (textProps->rawProps != nullptr) {
    // stack align
    facebook::react::TextAlignment alignHorizon =
        facebook::react::TextAlignment::Left;
    std::string alignVertical = "top";
    if (textProps->rawProps.count("textAlignVertical") != 0) {
      alignVertical = textProps->rawProps["textAlignVertical"].asString();
    }
    if (textProps->textAttributes.alignment.has_value()) {
      alignHorizon = textProps->textAttributes.alignment.value();
    }
    int32_t stackAlign =
        TextConversions::getStackAlign(alignHorizon, alignVertical);

    facebook::react::TextAlignment alignHorizonOld =
        facebook::react::TextAlignment::Left;
    std::string alignVerticalOld = "top";
    if (m_props && m_props->rawProps.count("textAlignVertical") != 0) {
      alignVerticalOld = m_props->rawProps["textAlignVertical"].asString();
    }
    if (m_props && m_props->textAttributes.alignment.has_value()) {
      alignHorizonOld = m_props->textAttributes.alignment.value();
    }
    int32_t stackAlignOld =
        TextConversions::getStackAlign(alignHorizonOld, alignVerticalOld);

    if (!m_props || stackAlign != stackAlignOld) {
      VLOG(3) << "[text-debug] stackAlign=" << stackAlign
              << ", stackAlignOld=" << stackAlignOld;
      m_stackNode.setAlign(stackAlign);
    }

    // enable
    if (textProps->rawProps.count("disabled") != 0) {
      bool enableFlag = textProps->rawProps["disabled"].asBool();
      VLOG(3) << "[text-debug] rawProps->disabled=" << enableFlag;
      m_textNode.setTextEnable(!enableFlag);
    }

    // MinFontSize
    std::optional<float> minFontSize =
        TextConversions::getMinFontSize(textProps);
    if (minFontSize.has_value()) {
      m_textNode.setMinFontSize(minFontSize.value());
      VLOG(3) << "[text-debug] minFontSize=" << minFontSize.value();
    }
  }
  this->setParagraphAttributes(textProps->paragraphAttributes);
  VLOG(3) << "[text-debug] setProps end";
}

void TextComponentInstance::onStateChanged(
    SharedConcreteState const& textState) {
  CppComponentInstance::onStateChanged(textState);
  m_touchTargetChildrenNeedUpdate = true;
  for (const auto& item : m_childNodes) {
    m_textNode.removeChild(*item);
  }
  m_childNodes.clear();
  uint32_t childIndex = 0;
  auto const& fragments = textState->getData().attributedString.getFragments();
  VLOG(3) << "[text-debug] getFragments size:" << fragments.size();
  if (fragments.empty()) {
    return;
  }
  for (const auto& fragment : fragments) {
    if (!fragment.isAttachment()) {
      std::shared_ptr<SpanNode> spanNode = std::make_shared<SpanNode>();
      VLOG(3) << "[text-debug] create span text=" << fragment.string.c_str()
              << ", index=" << childIndex;
      m_textNode.insertChild(*spanNode, childIndex);
      m_childNodes.push_back(spanNode);
      setFragment(fragment, spanNode, childIndex);
      childIndex++;
    } else {
      VLOG(3) << "[text-debug] create image span, index=" << childIndex;
      std::shared_ptr<ImageSpanNode> imageSpanNode =
          std::make_shared<ImageSpanNode>();
      m_textNode.insertChild(*imageSpanNode, childIndex);
      m_childNodes.push_back(imageSpanNode);
      setImageSpanSize(
          fragment.parentShadowView.layoutMetrics.frame.size, imageSpanNode);
      childIndex++;
    }
  }
  this->setTextAttributes(fragments[0].textAttributes);
}

void TextComponentInstance::setTextAttributes(
    const facebook::react::TextAttributes& textAttributes) {
  // TextAlign
  if (textAttributes.alignment.has_value()) {
    int32_t align = TextConversions::getArkUITextAlignment(
        textAttributes.alignment.value());
    VLOG(3) << "[text-debug] textAttributes.alignment=" << align;
    m_textNode.setTextAlign(align);
  }
}

void TextComponentInstance::setFragment(
    const facebook::react::AttributedString::Fragment& fragment,
    std::shared_ptr<SpanNode> spanNode,
    uint32_t index) {
  if (spanNode == nullptr) {
    return;
  }

  facebook::react::TextAttributes textAttributes = fragment.textAttributes;
  spanNode->setSpanContent(fragment.string);
  VLOG(3) << "[text-debug] fontColor="
          << (uint32_t)(*textAttributes.foregroundColor);
  if (textAttributes.foregroundColor) {
    spanNode->setFontColor((uint32_t)(*textAttributes.foregroundColor));
  }
  VLOG(3) << "[text-debug] fontFamily=" << textAttributes.fontFamily;
  spanNode->setFontFamily(textAttributes.fontFamily);

  if (textAttributes.fontWeight.has_value()) {
    int32_t realFontWeight = TextConversions::getArkUIFontWeight(
        (int32_t)textAttributes.fontWeight.value());
    VLOG(3) << "[text-debug] realFontWeight=" << realFontWeight
            << ", index=" << index;
    spanNode->setFontWeight(realFontWeight);
  }

  // FontSize
  float fontSize = isnan(textAttributes.fontSize)
      ? DEFAULT_FONT_SIZE
      : (float)textAttributes.fontSize;
  VLOG(3) << "[text-debug] textAttributes.fontSize=" << textAttributes.fontSize
          << ", fontSize=" << fontSize << ", index=" << index;
  spanNode->setFontSize(fontSize);

  // FontStyle
  if (textAttributes.fontStyle.has_value()) {
    VLOG(3) << "[text-debug] textAttributes.fontStyle="
            << (int32_t)textAttributes.fontStyle.value() << ", index=" << index;
    spanNode->setFontStyle((int32_t)textAttributes.fontStyle.value());
  }

  // TextDecoration
  if (textAttributes.textDecorationLineType.has_value()) {
    auto type = (int32_t)textAttributes.textDecorationLineType.value();
    VLOG(3) << "[text-debug] textAttributes.textDecorationLineType=" << type
            << ", textAttributes.textDecorationColor="
            << (uint32_t)(*textAttributes.textDecorationColor)
            << ", index=" << index;
    uint32_t color = 0xFF000000;
    if (textAttributes.textDecorationColor) {
      color = (uint32_t)(*textAttributes.textDecorationColor);
    } else if (textAttributes.foregroundColor) {
      color = (uint32_t)(*textAttributes.foregroundColor);
    }
    if (type ==
            (int32_t)facebook::react::TextDecorationLineType::Strikethrough ||
        type ==
            (int32_t)facebook::react::TextDecorationLineType::
                UnderlineStrikethrough) {
      type = ARKUI_TEXT_DECORATION_TYPE_LINE_THROUGH;
    }
    spanNode->setTextDecoration(type, color);
  }

  // TextLineHeight
  float lineHeight = isnan(textAttributes.lineHeight)
      ? fontSize * (1 + DEFAULT_LINE_SPACING)
      : (float)textAttributes.lineHeight;
  VLOG(3) << "[text-debug] textAttributes.lineHeight="
          << (float)textAttributes.lineHeight << ", lineHeight=" << lineHeight
          << ", index=" << index;
  spanNode->setTextLineHeight(lineHeight);

  // BackgroundColor
  VLOG(3) << "[text-debug] textAttributes.backgroundColor="
          << (uint32_t)(*textAttributes.backgroundColor) << ", index=" << index;
  if (textAttributes.backgroundColor) {
    spanNode->setBackgroundStyle((uint32_t)(*textAttributes.backgroundColor));
  }

  // letterSpacing
  if (!std::isnan(textAttributes.letterSpacing)) {
    VLOG(3) << "[text-debug] textAttributes.letterSpacing="
            << textAttributes.letterSpacing << ", index=" << index;
    spanNode->setTextLetterSpacing(textAttributes.letterSpacing);
  }

  // shadow
  float textShadowRadius = isnan(textAttributes.textShadowRadius)
      ? 0.0f
      : textAttributes.textShadowRadius;
  int32_t textShadowType = ARKUI_SHADOW_TYPE_COLOR;
  uint32_t textShadowColor = *textAttributes.textShadowColor;
  float textShadowOffsetX = textAttributes.textShadowOffset.has_value()
      ? textAttributes.textShadowOffset.value().width
      : 0.0f;
  float textShadowOffsetY = textAttributes.textShadowOffset.has_value()
      ? textAttributes.textShadowOffset.value().height
      : 0.0f;
  VLOG(3) << "[text-debug] setTextShadow:" << textShadowRadius
          << ", textShadowType:" << textShadowType
          << ", textShadowColor:" << textShadowColor
          << ", textShadowOffsetX:" << textShadowOffsetX
          << ", textShadowOffsetY:" << textShadowOffsetY << ", index=" << index;
  spanNode->setTextShadow(
      textShadowRadius,
      textShadowType,
      textShadowColor,
      textShadowOffsetX,
      textShadowOffsetY);

  // text Case
  if (textAttributes.textTransform.has_value()) {
    VLOG(3) << "[text-debug] textAttributes.textTransform="
            << (int)textAttributes.textTransform.value() << ", index=" << index;
    switch (textAttributes.textTransform.value()) {
      case facebook::react::TextTransform::None:
      case facebook::react::TextTransform::Uppercase:
      case facebook::react::TextTransform::Lowercase: {
        int32_t textCase = TextConversions::getArkUITextCase(
            textAttributes.textTransform.value());
        spanNode->setTextCase(textCase);
        break;
      }
      case facebook::react::TextTransform::Capitalize: {
        spanNode->setSpanContent(stringCapitalize(fragment.string));
        break;
      }
      default:
        // do nothing
        break;
    }
  }
  VLOG(3) << "[text-debug] setFragment end, index=" << index;
}

void TextComponentInstance::setImageSpanSize(
    const facebook::react::Size& imageSize,
    std::shared_ptr<ImageSpanNode> imageSpanNode) {
  if (imageSpanNode == nullptr) {
    return;
  }
  imageSpanNode->setSize(imageSize);
}

void TextComponentInstance::setParagraphAttributes(
    const facebook::react::ParagraphAttributes& paragraphAttributes) {
  // maximumNumberOfLines
  VLOG(3) << "[text-debug] paragraphAttributes.maximumNumberOfLines="
          << paragraphAttributes.maximumNumberOfLines;
  if (paragraphAttributes.maximumNumberOfLines > 0) {
    m_textNode.setTextMaxLines(paragraphAttributes.maximumNumberOfLines);
  } else {
    m_textNode.resetTextMaxLines();
  }

  // height Adaptive Policy
  VLOG(3) << "[text-debug] paragraphAttributes.heightAdaptivePolicy="
          << (int)paragraphAttributes.adjustsFontSizeToFit;
  int32_t heightAdaptivePolicy = paragraphAttributes.adjustsFontSizeToFit
      ? ARKUI_TEXT_HEIGHT_ADAPTIVE_POLICY_MIN_FONT_SIZE_FIRST
      : ARKUI_TEXT_HEIGHT_ADAPTIVE_POLICY_MAX_LINES_FIRST;
  m_textNode.setTextHeightAdaptivePolicy(heightAdaptivePolicy);

  // overflow
  int32_t ellipsizeMode =
      TextConversions::getArkUIEllipsizeMode(paragraphAttributes.ellipsizeMode);
  VLOG(3) << "[text-debug] paragraphAttributes.ellipsizeMode=" << ellipsizeMode;
  if (ellipsizeMode == -1) {
    m_textNode.setTextOverflow(ARKUI_TEXT_OVERFLOW_CLIP);
  } else {
    m_textNode.setTextOverflow(ARKUI_TEXT_OVERFLOW_ELLIPSIS);
    m_textNode.setTextEllipsisMode(ellipsizeMode);
  }

  // TextBreakStrategy
  auto textBreak = TextConversions::getArkUIWordBreakStrategy(
      paragraphAttributes.textBreakStrategy);
  m_textNode.setWordBreak(textBreak);
}

std::string TextComponentInstance::stringCapitalize(
    const std::string& strInput) {
  if (strInput.empty()) {
    return strInput;
  }

  std::string strRes;
  std::string split = " ";
  std::vector<std::string> subStringVector;
  subStringVector.clear();

  std::string strSrc = strInput + split;
  auto pos = strSrc.find(split);
  auto step = split.size();

  while (pos != std::string::npos) {
    std::string strTemp = strSrc.substr(0, pos);
    subStringVector.push_back(strTemp);

    strSrc = strSrc.substr(pos + step, strSrc.size());
    pos = strSrc.find(split);
  }

  for (auto subString : subStringVector) {
    if (std::isalpha(subString[0]) != 0) {
      std::transform(
          subString.begin(),
          subString.end(),
          subString.begin(),
          [](unsigned char c) { return std::tolower(c); });
      subString[0] = std::toupper(static_cast<unsigned char>(subString[0]));
    }
    if (!strRes.empty()) {
      strRes += split;
    }
    strRes += subString;
  }

  return strRes;
}

StackNode& TextComponentInstance::getLocalRootArkUINode() {
  return m_stackNode;
}

class TextFragmentTouchTarget : public TouchTarget {
 public:
  TextFragmentTouchTarget(
      facebook::react::Tag tag,
      TouchTarget::Weak parentTouchTarget,
      std::vector<facebook::react::Rect> rects,
      facebook::react::SharedTouchEventEmitter touchEventEmitter)
      : m_tag(tag),
        m_touchEventEmitter(std::move(touchEventEmitter)),
        m_rects(std::move(rects)),
        m_parentTouchTarget(parentTouchTarget) {}

  void setRects(std::vector<facebook::react::Rect> rects) {
    m_rects = std::move(rects);
  }

  facebook::react::SharedTouchEventEmitter getTouchEventEmitter()
      const override {
    return m_touchEventEmitter;
  }

  facebook::react::Point computeChildPoint(
      facebook::react::Point const& point,
      TouchTarget::Shared const& /*child*/) const override {
    return point;
  }

  bool containsPoint(facebook::react::Point const& point) const override {
    for (auto rect : m_rects) {
      if (rect.containsPoint(point)) {
        return true;
      }
    }
    return false;
  }

  bool containsPointInBoundingBox(
      facebook::react::Point const& point) override {
    return containsPoint(point);
  }

  bool canHandleTouch() const override {
    return m_touchEventEmitter != nullptr;
  }

  bool canChildrenHandleTouch() const override {
    return false;
  }

  facebook::react::Tag getTouchTargetTag() const override {
    return m_tag;
  }

  TouchTarget::Shared getTouchTargetParent() const override {
    return m_parentTouchTarget.lock();
  };

  std::vector<TouchTarget::Shared> getTouchTargetChildren() override {
    return {};
  }

  facebook::react::LayoutMetrics getLayoutMetrics() const override {
    return {};
  }

  facebook::react::Transform getTransform() const override {
    return {};
  }

  facebook::react::Rect getBoundingBox() override {
    return {};
  };

  bool isClippingSubviews() const override {
    return false;
  }

  void markBoundingBoxAsDirty() override{};

 private:
  facebook::react::Tag m_tag;
  facebook::react::SharedTouchEventEmitter m_touchEventEmitter;
  std::vector<facebook::react::Rect> m_rects;
  TouchTarget::Weak m_parentTouchTarget;
};

std::vector<TouchTarget::Shared>
TextComponentInstance::getTouchTargetChildren() {
  if (m_state == nullptr) {
    m_fragmentTouchTargetByTag.clear();
    return {};
  }

  if (m_touchTargetChildrenNeedUpdate) {
    m_touchTargetChildrenNeedUpdate = false;
    updateFragmentTouchTargets(m_state->getData());
  }

  auto const& fragments = m_state->getData().attributedString.getFragments();

  std::vector<TouchTarget::Shared> result;
  auto attachmentCount = 0;
  for (auto const& fragment : fragments) {
    if (fragment.isAttachment()) {
      result.push_back(m_children.at(attachmentCount));
      attachmentCount++;
    } else {
      result.push_back(
          m_fragmentTouchTargetByTag.at(fragment.parentShadowView.tag));
    }
  }
  return result;
}

void TextComponentInstance::updateFragmentTouchTargets(
    facebook::react::ParagraphState const& newState) {
  auto const& fragments = newState.attributedString.getFragments();
  auto textLayoutManager =
      newState.paragraphLayoutManager.getTextLayoutManager();
  if (textLayoutManager == nullptr || fragments.empty()) {
    m_fragmentTouchTargetByTag.clear();
    return;
  }

  auto nativeTextLayoutManager =
      textLayoutManager->getNativeTextLayoutManager();
  auto textMeasurer = static_cast<TextMeasurer*>(nativeTextLayoutManager);
  auto typography = textMeasurer->measureTypography(
      newState.attributedString,
      newState.paragraphAttributes,
      {m_layoutMetrics.frame.size, m_layoutMetrics.frame.size});
  auto rects = typography.getRectsForFragments();

  FragmentTouchTargetByTag touchTargetByTag;
  size_t textFragmentCount = 0;
  for (auto const& fragment : fragments) {
    if (fragment.isAttachment()) {
      continue;
    }
    auto index = textFragmentCount;
    auto eventEmitter =
        std::dynamic_pointer_cast<const facebook::react::TouchEventEmitter>(
            fragment.parentShadowView.eventEmitter);
    auto tag = fragment.parentShadowView.tag;
    if (m_fragmentTouchTargetByTag.count(tag) > 0) {
      // update the existing touch target
      auto fragmentTouchTarget =
          std::static_pointer_cast<TextFragmentTouchTarget>(
              m_fragmentTouchTargetByTag.at(tag));
      fragmentTouchTarget->setRects(rects.at(index));
      touchTargetByTag.try_emplace(tag, std::move(fragmentTouchTarget));
    } else {
      // create a new touch target
      touchTargetByTag.try_emplace(
          tag,
          std::make_shared<TextFragmentTouchTarget>(
              tag,
              this->shared_from_this(),
              rects.at(index),
              std::move(eventEmitter)));
    }
    textFragmentCount++;
  }
  m_fragmentTouchTargetByTag = std::move(touchTargetByTag);
}
} // namespace rnoh