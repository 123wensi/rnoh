#pragma once
#include "RNOH/BaseComponentNapiBinder.h"
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/RectangleCorners.h>

namespace rnoh {

class ViewComponentNapiBinder : public BaseComponentNapiBinder {
  public:
    napi_value createProps(napi_env env, facebook::react::ShadowView const shadowView) override {
        napi_value napiBaseProps = BaseComponentNapiBinder::createProps(env, shadowView);
        if (auto props = std::dynamic_pointer_cast<const facebook::react::ViewProps>(shadowView.props)) {
            auto borderMetrics = props->resolveBorderMetrics(shadowView.layoutMetrics);
            auto rawProps = shadowView.props->rawProps;

            return ArkJS(env)
                .getObjectBuilder(napiBaseProps)
                .addProperty("backgroundColor", props->backgroundColor)
                .addProperty("opacity", props->opacity)
                .addProperty("borderWidth", borderMetrics.borderWidths.top)
                .addProperty("borderColor", borderMetrics.borderColors.top)
                .addProperty("borderRadius", borderMetrics.borderRadii)
                .addProperty("transform", props->transform.matrix)
                .addProperty("pointerEvents", this->stringifyPointerEvents(props->pointerEvents))
                .build();
        }
        return napiBaseProps;
    };

  private:
    std::string stringifyPointerEvents(facebook::react::PointerEventsMode pointerEventsMode) {
        switch (pointerEventsMode) {
        case facebook::react::PointerEventsMode::Auto:
            return "auto";
        case facebook::react::PointerEventsMode::BoxNone:
            return "box-none";
        case facebook::react::PointerEventsMode::BoxOnly:
            return "box-only";
        case facebook::react::PointerEventsMode::None:
            return "none";
        }
    }
};

} // namespace rnoh
