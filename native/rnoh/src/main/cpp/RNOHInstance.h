#ifndef native_RNOHInstance_H
#define native_RNOHInstance_H
#include <memory>
#include <utility>
#include <thread>
#include <functional>
#include <napi/native_api.h>
#include <js_native_api.h>
#include <js_native_api_types.h>

#include <cxxreact/Instance.h>
#include <cxxreact/ModuleRegistry.h>
#include <cxxreact/NativeModule.h>
#include <folly/dynamic.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/renderer/componentregistry/ComponentDescriptorProviderRegistry.h>

#include "RNOHMessageQueueThread.h"
#include "RNOHSchedulerDelegate.h"
#include "TaskExecutor/TaskExecutor.h"
#include "mounting/events/EventEmitterRegistry.h"
#include "mounting/events/EventEmitterHelper.h"

class RNOHInstance {
  public:
    RNOHInstance(napi_env env, std::function<void(facebook::react::ShadowViewMutationList const &mutations)> onComponentDescriptorTreeChanged)
        : env(env),
          surfaceHandler("rnempty", 1),
          instance(std::make_shared<facebook::react::Instance>()),
          scheduler(nullptr),
          taskExecutor(nullptr),
          onComponentDescriptorTreeChanged(onComponentDescriptorTreeChanged),
          eventEmitterRegistry(std::make_shared<rnoh::EventEmitterRegistry>()),
          eventEmitterHelper(ArkJS(env), eventEmitterRegistry) {}

    void start();
    void runApplication();

    void emitEvent(facebook::react::Tag tag, rnoh::ReactEventKind eventKind, napi_value eventObject);

  private:
    napi_env env;
    std::shared_ptr<facebook::react::ContextContainer> contextContainer;
    std::shared_ptr<facebook::react::Instance> instance;
    std::function<void(facebook::react::ShadowViewMutationList const &mutations)> onComponentDescriptorTreeChanged;
    facebook::react::SurfaceHandler surfaceHandler;
    std::unique_ptr<facebook::react::Scheduler> scheduler;
    std::unique_ptr<RNOHSchedulerDelegate> schedulerDelegate;
    std::shared_ptr<rnoh::TaskExecutor> taskExecutor;
    std::shared_ptr<ComponentDescriptorProviderRegistry> componentDescriptorProviderRegistry;
    rnoh::EventEmitterRegistry::Shared eventEmitterRegistry;
    rnoh::EventEmitterHelper eventEmitterHelper;

    void initialize();
    void initializeComponentDescriptorRegistry();
    void initializeScheduler();
};

#endif // native_RNOHInstance_H
