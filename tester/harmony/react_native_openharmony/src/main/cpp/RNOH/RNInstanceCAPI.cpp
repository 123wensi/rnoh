#include "RNInstanceCAPI.h"
#include <cxxreact/JSBundleType.h>
#include <jsireact/JSIExecutor.h>
#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>
#include <react/renderer/componentregistry/ComponentDescriptorRegistry.h>
#include <react/renderer/scheduler/Scheduler.h>
#include "NativeLogger.h"
#include "RNInstanceArkTS.h"
#include "RNOH/EventBeat.h"
#include "RNOH/MessageQueueThread.h"
#include "RNOH/Performance/NativeTracing.h"
#include "RNOH/ShadowViewRegistry.h"
#include "RNOH/TurboModuleFactory.h"
#include "RNOH/TurboModuleProvider.h"
#include "SchedulerDelegateCAPI.h"
#include "hermes/executor/HermesExecutorFactory.h"

using namespace facebook;
using namespace rnoh;

TaskExecutor::Shared RNInstanceCAPI::getTaskExecutor() {
  return taskExecutor;
};

void RNInstanceCAPI::start() {
  DLOG(INFO) << "RNInstanceCAPI::start";
  this->initialize();
  m_turboModuleProvider = this->createTurboModuleProvider();
  this->initializeScheduler(m_turboModuleProvider);
  this->instance->getRuntimeExecutor()(
      [binders = this->m_globalJSIBinders,
       turboModuleProvider =
           m_turboModuleProvider](facebook::jsi::Runtime& rt) {
        for (auto& binder : binders) {
          binder->createBindings(rt, turboModuleProvider);
        }
      });
}

void RNInstanceCAPI::initialize() {
  DLOG(INFO) << "RNInstanceCAPI::initialize";
  // create a new event dispatcher every time RN is initialized
  m_eventDispatcher = std::make_shared<EventDispatcher>();
  std::vector<std::unique_ptr<react::NativeModule>> modules;
  auto instanceCallback = std::make_unique<react::InstanceCallback>();
  auto jsExecutorFactory = std::make_shared<react::HermesExecutorFactory>(
      // runtime installer, which is run when the runtime
      // is first initialized and provides access to the runtime
      // before the JS code is executed
      [](facebook::jsi::Runtime& rt) {
        // install `console.log` (etc.) implementation
        react::bindNativeLogger(rt, nativeLogger);
        // install tracing functions
        rnoh::setupTracing(rt);
      });
  jsExecutorFactory->setEnableDebugger(m_shouldEnableDebugger);
  m_jsQueue = std::make_shared<MessageQueueThread>(this->taskExecutor);
  auto moduleRegistry =
      std::make_shared<react::ModuleRegistry>(std::move(modules));
  this->instance->initializeBridge(
      std::move(instanceCallback),
      std::move(jsExecutorFactory),
      m_jsQueue,
      std::move(moduleRegistry));
}

void RNInstanceCAPI::initializeScheduler(
    std::shared_ptr<TurboModuleProvider> turboModuleProvider) {
  DLOG(INFO) << "RNInstanceCAPI::initializeScheduler";
  auto reactConfig = std::make_shared<react::EmptyReactNativeConfig>();
  m_contextContainer->insert("ReactNativeConfig", std::move(reactConfig));

  react::EventBeat::Factory eventBeatFactory =
      [taskExecutor = std::weak_ptr(taskExecutor),
       runtimeExecutor = this->instance->getRuntimeExecutor()](auto ownerBox) {
        return std::make_unique<EventBeat>(
            taskExecutor, runtimeExecutor, ownerBox);
      };

  react::ComponentRegistryFactory componentRegistryFactory =
      [registry = m_componentDescriptorProviderRegistry](
          auto eventDispatcher, auto contextContainer) {
        return registry->createComponentDescriptorRegistry(
            {eventDispatcher, contextContainer});
      };

  react::SchedulerToolbox schedulerToolbox{
      .contextContainer = m_contextContainer,
      .componentRegistryFactory = componentRegistryFactory,
      .runtimeExecutor = this->instance->getRuntimeExecutor(),
      .asynchronousEventBeatFactory = eventBeatFactory,
      .synchronousEventBeatFactory = eventBeatFactory,
  };

  if (m_shouldEnableBackgroundExecutor) {
    schedulerToolbox.backgroundExecutor =
        [executor = this->taskExecutor](std::function<void()>&& callback) {
          if (executor->isOnTaskThread(TaskThread::MAIN)) {
            callback();
            return;
          }

          executor->runTask(TaskThread::BACKGROUND, std::move(callback));
        };
  }

  m_animationDriver = std::make_shared<react::LayoutAnimationDriver>(
      this->instance->getRuntimeExecutor(), m_contextContainer, this);
  this->scheduler = std::make_shared<react::Scheduler>(
      schedulerToolbox, m_animationDriver.get(), m_schedulerDelegate.get());
  turboModuleProvider->setScheduler(this->scheduler);
  DLOG(INFO) << "RNInstanceCAPI::initializeScheduler::end";
}

std::shared_ptr<TurboModuleProvider>
RNInstanceCAPI::createTurboModuleProvider() {
  DLOG(INFO) << "RNInstanceCAPI::createTurboModuleProvider";
  auto sharedInstance = shared_from_this();
  auto turboModuleProvider = std::make_shared<TurboModuleProvider>(
      this->instance->getJSCallInvoker(),
      std::move(m_turboModuleFactory),
      m_eventDispatcher,
      std::move(m_jsQueue),
      sharedInstance);
  turboModuleProvider->installJSBindings(this->instance->getRuntimeExecutor());
  return turboModuleProvider;
}

void RNInstanceCAPI::loadScript(
    std::vector<uint8_t>&& bundle,
    std::string const sourceURL,
    std::function<void(const std::string)>&& onFinish) {
  DLOG(INFO) << "RNInstanceCAPI::loadScript";
  this->taskExecutor->runTask(
      TaskThread::JS,
      [this,
       bundle = std::move(bundle),
       sourceURL,
       onFinish = std::move(onFinish)]() mutable {
        std::unique_ptr<react::JSBigBufferString> jsBundle;
        jsBundle = std::make_unique<react::JSBigBufferString>(bundle.size());
        memcpy(jsBundle->data(), bundle.data(), bundle.size());

        react::BundleHeader header;
        memcpy(&header, bundle.data(), sizeof(react::BundleHeader));
        react::ScriptTag scriptTag = react::parseTypeFromHeader(header);
        // NOTE: Hermes bytecode bundles are treated as String bundles,
        // and don't throw an error here.
        if (scriptTag != react::ScriptTag::String) {
          throw new std::runtime_error("RAM bundles are not yet supported");
        }
        try {
          this->instance->loadScriptFromString(
              std::move(jsBundle), sourceURL, true);
          onFinish("");
        } catch (std::exception const& e) {
          try {
            std::rethrow_if_nested(e);
            onFinish(e.what());
          } catch (const std::exception& nested) {
            onFinish(e.what() + std::string("\n") + nested.what());
          }
        }
      });
}

void rnoh::RNInstanceCAPI::emitComponentEvent(
    napi_env env,
    react::Tag tag,
    std::string eventName,
    napi_value payload) {
  DLOG(INFO) << "RNInstanceCAPI::emitComponentEvent";
  EventEmitRequestHandler::Context ctx{
      .env = env,
      .tag = tag,
      .eventName = std::move(eventName),
      .payload = payload,
      .shadowViewRegistry = this->m_shadowViewRegistry,
  };

  if (m_eventDispatcher != nullptr) {
    m_eventDispatcher->sendEvent(ctx);
  }

  for (auto& eventEmitRequestHandler : m_eventEmitRequestHandlers) {
    eventEmitRequestHandler->handleEvent(ctx);
  }
}

void rnoh::RNInstanceCAPI::onMemoryLevel(size_t memoryLevel) {
  DLOG(INFO) << "RNInstanceCAPI::onMemoryLevel";
  // Android memory levels are 5, 10, 15, while Ark's are 0, 1, 2
  static const int memoryLevels[] = {5, 10, 15};
  if (this->instance) {
    this->instance->handleMemoryPressure(memoryLevels[memoryLevel]);
  }
}

void rnoh::RNInstanceCAPI::updateState(
    napi_env env,
    std::string const& componentName,
    facebook::react::Tag tag,
    napi_value newState) {
  DLOG(INFO) << "RNInstanceCAPI::updateState";
  if (auto state =
          m_shadowViewRegistry->getFabricState<facebook::react::State>(tag)) {
    m_mutationsToNapiConverter->updateState(
        env, componentName, state, newState);
  }
}

void rnoh::RNInstanceCAPI::synchronouslyUpdateViewOnUIThread(
    facebook::react::Tag tag,
    folly::dynamic props) {
  DLOG(INFO) << "RNInstanceCAPI::synchronouslyUpdateViewOnUIThread";

  auto schedulerDelegateCapi =
      dynamic_cast<SchedulerDelegateCAPI*>(m_schedulerDelegate.get());
  if (schedulerDelegateCapi == nullptr) {
    LOG(ERROR)
        << "RNInstanceCAPI::synchronouslyUpdateViewOnUIThread: scheduler delegate for this instance is not "
           "set up correctly";
    return;
  }

  auto componentInstance = m_componentInstanceRegistry->findByTag(tag);
  if (componentInstance == nullptr) {
    LOG(ERROR)
        << "RNInstanceCAPI::synchronouslyUpdateViewOnUIThread: could not find componentInstance for tag: "
        << tag;
    return;
  }

  auto componentHandle = componentInstance->getComponentHandle();
  auto componentDescriptor =
      scheduler->findComponentDescriptorByHandle_DO_NOT_USE_THIS_IS_BROKEN(
          componentHandle);
  if (componentDescriptor == nullptr) {
    LOG(ERROR)
        << "RNInstanceCAPI::synchronouslyUpdateViewOnUIThread: could not find componentDescriptor for tag: "
        << tag;
    return;
  }

  schedulerDelegateCapi->synchronouslyUpdateViewOnUIThread(
      tag, std::move(props), *componentDescriptor);
}

facebook::react::ContextContainer const&
rnoh::RNInstanceCAPI::getContextContainer() const {
  DLOG(INFO) << "RNInstanceCAPI::getContextContainer";
  return *m_contextContainer;
}

void RNInstanceCAPI::callFunction(
    std::string&& module,
    std::string&& method,
    folly::dynamic&& params) {
  //     DLOG(INFO) << "RNInstanceCAPI::callFunction"; // commented out, it's
  //     too verbose
  this->taskExecutor->runTask(
      TaskThread::JS,
      [weakInstance = std::weak_ptr(this->instance),
       module = std::move(module),
       method = std::move(method),
       params = std::move(params)]() mutable {
        if (auto instance = weakInstance.lock()) {
          instance->callJSFunction(
              std::move(module), std::move(method), std::move(params));
        }
      });
}

void RNInstanceCAPI::onAnimationStarted() {
  m_shouldRelayUITick.store(true);
}

void RNInstanceCAPI::onAllAnimationsComplete() {
  m_shouldRelayUITick.store(false);
}

void RNInstanceCAPI::onUITick() {
  if (this->m_shouldRelayUITick.load()) {
    this->scheduler->animationTick();
  }
}

void RNInstanceCAPI::registerNativeXComponentHandle(
    OH_NativeXComponent* nativeXComponent,
    facebook::react::Tag surfaceId) {
  DLOG(INFO) << "RNInstanceCAPI::registerNativeXComponentHandle";
  if (nativeXComponent == nullptr) {
    return;
  }
  // NOTE: for some reason, attaching in the NAPI call made by XComponent
  // fails to mount the ArkUI node. Posting a task to be executed separately
  // fixes the issue.
  taskExecutor->runTask(TaskThread::MAIN, [this, nativeXComponent, surfaceId] {
    auto it = m_surfaceById.find(surfaceId);
    if (it == m_surfaceById.end()) {
      LOG(ERROR) << "Surface with id: " << surfaceId << " not found";
      return;
    }
    it->second.attachNativeXComponent(nativeXComponent);
  });
}

TurboModule::Shared RNInstanceCAPI::getTurboModule(const std::string& name) {
  auto turboModule = m_turboModuleProvider->getTurboModule(name);
  if (turboModule) {
    auto rnohTurboModule = std::dynamic_pointer_cast<TurboModule>(turboModule);
    if (rnohTurboModule) {
      return rnohTurboModule;
    } else {
      DLOG(ERROR) << "TurboModule '" << name
                  << "' should extend rnoh::TurboModule";
      return nullptr;
    }
  }
  return nullptr;
}

void RNInstanceCAPI::createSurface(
    facebook::react::Tag surfaceId,
    std::string const& moduleName) {
  DLOG(INFO) << "RNInstanceCAPI::createSurface";
  m_surfaceById.emplace(
      surfaceId,
      XComponentSurface(
          scheduler,
          m_componentInstanceRegistry,
          m_componentInstanceFactory,
          surfaceId,
          moduleName));
}

void RNInstanceCAPI::updateSurfaceConstraints(
    facebook::react::Tag surfaceId,
    float width,
    float height,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL) {
  DLOG(INFO) << "RNInstanceCAPI::updateSurfaceConstraints";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second.updateConstraints(
      width, height, viewportOffsetX, viewportOffsetY, pixelRatio, isRTL);
}

void RNInstanceCAPI::startSurface(
    facebook::react::Tag surfaceId,
    float width,
    float height,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    folly::dynamic&& initialProps) {
  DLOG(INFO) << "RNInstanceCAPI::startSurface";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second.start(
      width,
      height,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      isRTL,
      std::move(initialProps),
      m_animationDriver);
}

void RNInstanceCAPI::setSurfaceProps(
    facebook::react::Tag surfaceId,
    folly::dynamic&& props) {
  DLOG(INFO) << "RNInstanceCAPI::setSurfaceProps";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second.setProps(std::move(props));
}

void RNInstanceCAPI::stopSurface(facebook::react::Tag surfaceId) {
  DLOG(INFO) << "RNInstanceCAPI::stopSurface";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second.stop();
}

void RNInstanceCAPI::destroySurface(facebook::react::Tag surfaceId) {
  DLOG(INFO) << "RNInstanceCAPI::destroySurface";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  m_surfaceById.erase(it);
}

void RNInstanceCAPI::setSurfaceDisplayMode(
    facebook::react::Tag surfaceId,
    facebook::react::DisplayMode displayMode) {
  DLOG(INFO) << "RNInstanceCAPI::setSurfaceDisplayMode";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second.setDisplayMode(displayMode);
}

ComponentInstance::Shared RNInstanceCAPI::findComponentInstanceByTag(
    facebook::react::Tag tag) {
  return m_componentInstanceRegistry->findByTag(tag);
}

std::optional<facebook::react::Tag>
RNInstanceCAPI::findComponentInstanceTagById(const std::string& id) {
  return m_componentInstanceRegistry->findTagById(id);
}

void RNInstanceCAPI::handleArkTSMessage(
    const std::string& name,
    folly::dynamic const& payload) {
  for (auto& arkTSMessageHandler : m_arkTSMessageHandlers) {
    arkTSMessageHandler->handleArkTSMessage(
        {.messageName = name,
         .messagePayload = payload,
         .rnInstance = this->shared_from_this()});
  }
}

void RNInstanceCAPI::postMessageToArkTS(
    const std::string& name,
    folly::dynamic const& payload) {
  m_arkTSChannel->postMessage(name, payload);
};