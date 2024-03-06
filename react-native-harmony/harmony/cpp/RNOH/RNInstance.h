#pragma once
#include <memory>
#include <thread>
#include <functional>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <atomic>

#include <cxxreact/Instance.h>
#include <cxxreact/ModuleRegistry.h>
#include <cxxreact/NativeModule.h>
#include <folly/dynamic.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/uimanager/LayoutAnimationStatusDelegate.h>

#include "RNOH/GlobalJSIBinder.h"
#include "RNOH/MessageQueueThread.h"
#include "RNOH/SchedulerDelegate.h"
#include "RNOH/TurboModuleProvider.h"
#include "RNOH/EventEmitRequestHandler.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "RNOH/UITicker.h"
#include "RNOH/ArkTSChannel.h"

namespace rnoh {
    using MutationsListener =
        std::function<void(MutationsToNapiConverter const &, facebook::react::ShadowViewMutationList const &mutations)>;

    class RNInstance {
    public:
        virtual ~RNInstance() = default;
    
        virtual TaskExecutor::Shared getTaskExecutor() = 0;
        virtual void start() = 0;
        virtual void loadScript(std::vector<uint8_t> &&bundle, std::string const sourceURL,
                                std::function<void(const std::string)> &&onFinish) = 0;
        virtual void createSurface(facebook::react::Tag surfaceId, std::string const &moduleName) = 0;
        virtual void updateSurfaceConstraints(facebook::react::Tag surfaceId, float width, float height,
                                              float viewportOffsetX, float viewportOffsetY, float pixelRatio) = 0;
        virtual void startSurface(facebook::react::Tag surfaceId, float width, float height, float viewportOffsetX,
                                  float viewportOffsetY, float pixelRatio, folly::dynamic &&initialProps) = 0;
        virtual void setSurfaceProps(facebook::react::Tag surfaceId, folly::dynamic &&props) = 0;
        virtual void stopSurface(facebook::react::Tag surfaceId) = 0;
        virtual void destroySurface(facebook::react::Tag surfaceId) = 0;
        virtual void setSurfaceDisplayMode(facebook::react::Tag surfaceId, facebook::react::DisplayMode displayMode) = 0;
        virtual void callFunction(std::string &&module, std::string &&method, folly::dynamic &&params) = 0;
        virtual void emitComponentEvent(napi_env env, facebook::react::Tag tag, std::string eventName,
                                        napi_value payload) = 0;
        virtual void onMemoryLevel(size_t memoryLevel) = 0;
        virtual void updateState(napi_env env, std::string const &componentName, facebook::react::Tag tag,
                                 napi_value newState) = 0;
    };

} // namespace rnoh
