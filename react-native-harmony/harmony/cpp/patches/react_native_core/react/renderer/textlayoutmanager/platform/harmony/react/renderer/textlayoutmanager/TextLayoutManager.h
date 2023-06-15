/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/AttributedStringBox.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <react/utils/ContextContainer.h>

namespace facebook {
namespace react {

class TextLayoutManager;

using SharedTextLayoutManager = std::shared_ptr<const TextLayoutManager>;

class TextLayoutManagerDelegate {
  public:
    virtual facebook::react::Size measure(std::string textContent) = 0;
};

/*
 * Cross platform facade for Android-specific TextLayoutManager.
 */
class TextLayoutManager {
  public:
    TextLayoutManager(const ContextContainer::Shared &contextContainer) {
        m_textLayoutManagerDelegate = contextContainer->at<std::shared_ptr<TextLayoutManagerDelegate>>("textLayoutManagerDelegate");
    }

    /*
   * Measures `attributedStringBox` using native text rendering infrastructure.
   */
    TextMeasurement measure(
        AttributedStringBox attributedStringBox,
        ParagraphAttributes paragraphAttributes,
        LayoutConstraints layoutConstraints) const;

    /*
   * Measures lines of `attributedString` using native text rendering
   * infrastructure.
   */
    LinesMeasurements measureLines(
        AttributedString attributedString,
        ParagraphAttributes paragraphAttributes,
        Size size) const;

    /*
   * Returns an opaque pointer to platform-specific TextLayoutManager.
   * Is used on a native views layer to delegate text rendering to the manager.
   */
    void *getNativeTextLayoutManager() const;

  private:
    std::shared_ptr<TextLayoutManagerDelegate> m_textLayoutManagerDelegate;
};

} // namespace react
} // namespace facebook
