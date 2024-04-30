#include "RNInstanceArkTS.h"
#include <cxxreact/JSBundleType.h>
#include <jsireact/JSIExecutor.h>
#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>
#include <react/renderer/componentregistry/ComponentDescriptorRegistry.h>
#include <react/renderer/scheduler/Scheduler.h>
#include "NativeLogger.h"
#include "RNOH/EventBeat.h"
#include "RNOH/MessageQueueThread.h"
#include "RNOH/Performance/NativeTracing.h"
#include "RNOH/ShadowViewRegistry.h"
#include "RNOH/TurboModuleFactory.h"
#include "RNOH/TurboModuleProvider.h"
#include "hermes/executor/HermesExecutorFactory.h"

using namespace facebook;
using namespace rnoh;

TaskExecutor::Shared RNInstanceArkTS::getTaskExecutor() {
  return taskExecutor;
};

void RNInstanceArkTS::start() {
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

void RNInstanceArkTS::initialize() {
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

void RNInstanceArkTS::initializeScheduler(
    std::shared_ptr<TurboModuleProvider> turboModuleProvider) {
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
}

std::shared_ptr<TurboModuleProvider>
RNInstanceArkTS::createTurboModuleProvider() {
  auto turboModuleProvider = std::make_shared<TurboModuleProvider>(
      this->instance->getJSCallInvoker(),
      std::move(m_turboModuleFactory),
      m_eventDispatcher,
      std::move(m_jsQueue));
  turboModuleProvider->installJSBindings(this->instance->getRuntimeExecutor());
  return turboModuleProvider;
}

void RNInstanceArkTS::loadScript(
    std::vector<uint8_t>&& bundle,
    std::string const sourceURL,
    std::function<void(const std::string)>&& onFinish) {
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

void rnoh::RNInstanceArkTS::createSurface(
    react::Tag surfaceId,
    std::string const& appKey) {
  taskExecutor->runTask(TaskThread::JS, [this, surfaceId, appKey] {
    if (surfaceHandlers.count(surfaceId)) {
      LOG(ERROR) << "createSurface: Surface with surface id " << surfaceId
                 << " already exists.";
      return;
    }
    auto surfaceHandler =
        std::make_shared<react::SurfaceHandler>(appKey, surfaceId);
    this->scheduler->registerSurface(*surfaceHandler);
    surfaceHandlers.insert({surfaceId, std::move(surfaceHandler)});
  });
}

void RNInstanceArkTS::startSurface(
    react::Tag surfaceId,
    float width,
    float height,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    folly::dynamic&& initialProps) {
  taskExecutor->runTask(TaskThread::JS, [=] {
    try {
      auto it = surfaceHandlers.find(surfaceId);
      if (it == surfaceHandlers.end()) {
        LOG(ERROR) << "startSurface: No surface with id " << surfaceId;
        return;
      }

      auto surfaceHandler = it->second;
      surfaceHandler->setProps(std::move(initialProps));
      auto layoutConstraints = surfaceHandler->getLayoutConstraints();
      layoutConstraints.layoutDirection = react::LayoutDirection::LeftToRight;
      layoutConstraints.minimumSize =
          layoutConstraints.maximumSize = {.width = width, .height = height};
      auto layoutContext = surfaceHandler->getLayoutContext();
      layoutContext.viewportOffset = {viewportOffsetX, viewportOffsetY};
      layoutContext.pointScaleFactor = pixelRatio;
      surfaceHandler->constraintLayout(layoutConstraints, layoutContext);
      LOG(INFO) << "startSurface::starting: surfaceId=" << surfaceId;
      surfaceHandler->start();
      LOG(INFO) << "startSurface::started surfaceId=" << surfaceId;
      auto mountingCoordinator = surfaceHandler->getMountingCoordinator();
      mountingCoordinator->setMountingOverrideDelegate(m_animationDriver);
    } catch (const std::exception& e) {
      LOG(ERROR) << "startSurface: " << e.what() << "\n";
      throw e;
    };
  });
}

void rnoh::RNInstanceArkTS::setSurfaceProps(
    facebook::react::Tag surfaceId,
    folly::dynamic&& props) {
  taskExecutor->runTask(TaskThread::JS, [=] {
    auto it = surfaceHandlers.find(surfaceId);
    if (it == surfaceHandlers.end()) {
      LOG(ERROR) << "setSurfaceProps: No surface with id " << surfaceId;
      return;
    }
    it->second->setProps(std::move(props));
  });
}

void rnoh::RNInstanceArkTS::stopSurface(react::Tag surfaceId) {
  taskExecutor->runTask(TaskThread::JS, [=] {
    auto it = surfaceHandlers.find(surfaceId);
    if (it == surfaceHandlers.end()) {
      LOG(ERROR) << "stopSurface: No surface with id " << surfaceId;
      return;
    }
    auto surfaceHandle = it->second;
    LOG(INFO) << "stopSurface: stopping " << surfaceId;
    try {
      surfaceHandle->stop();
      LOG(INFO) << "stopSurface: stopped " << surfaceId;
    } catch (const std::exception& e) {
      LOG(ERROR) << "stopSurface: failed - " << e.what() << "\n";
      throw e;
    };
  });
}

void rnoh::RNInstanceArkTS::destroySurface(react::Tag surfaceId) {
  taskExecutor->runTask(TaskThread::JS, [=] {
    auto it = surfaceHandlers.find(surfaceId);
    if (it == surfaceHandlers.end()) {
      LOG(ERROR) << "destroySurface: No surface with id " << surfaceId;
      return;
    }
    scheduler->unregisterSurface(*it->second);
    surfaceHandlers.erase(it);
  });
}

void rnoh::RNInstanceArkTS::setSurfaceDisplayMode(
    facebook::react::Tag surfaceId,
    facebook::react::DisplayMode displayMode) {
  taskExecutor->runTask(TaskThread::JS, [this, surfaceId, displayMode]() {
    try {
      auto surfaceIt = surfaceHandlers.find(surfaceId);
      if (surfaceIt == surfaceHandlers.end()) {
        LOG(ERROR) << "setSurfaceDisplayMode: No surface with id " << surfaceId;
        return;
      }
      auto surfaceHandler = surfaceIt->second;
      surfaceHandler->setDisplayMode(displayMode);
    } catch (const std::exception& e) {
      LOG(ERROR) << "setSurfaceDisplayMode: " << e.what() << "\n";
      throw e;
    }
  });
}

void RNInstanceArkTS::updateSurfaceConstraints(
    react::Tag surfaceId,
    float width,
    float height,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL) {
  taskExecutor->runTask(
      TaskThread::JS,
      [this,
       surfaceId,
       width,
       height,
       viewportOffsetX,
       viewportOffsetY,
       pixelRatio,
       isRTL]() {
        try {
          if (surfaceHandlers.count(surfaceId) == 0) {
            LOG(ERROR) << "updateSurfaceConstraints: No surface with id "
                       << surfaceId;
            return;
          }
          auto layoutConstraints =
              surfaceHandlers[surfaceId]->getLayoutConstraints();
          layoutConstraints.minimumSize = layoutConstraints.maximumSize = {
              .width = width, .height = height};
          auto layoutContext = surfaceHandlers[surfaceId]->getLayoutContext();
          layoutContext.viewportOffset = {viewportOffsetX, viewportOffsetY};
          layoutContext.pointScaleFactor = pixelRatio;

          surfaceHandlers[surfaceId]->constraintLayout(
              layoutConstraints, layoutContext);
        } catch (const std::exception& e) {
          LOG(ERROR) << "updateSurfaceConstraints: " << e.what() << "\n";
          throw e;
        }
      });
}

void rnoh::RNInstanceArkTS::emitComponentEvent(
    napi_env env,
    react::Tag tag,
    std::string eventName,
    napi_value payload) {
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

void rnoh::RNInstanceArkTS::onMemoryLevel(size_t memoryLevel) {
  // Android memory levels are 5, 10, 15, while Ark's are 0, 1, 2
  static const int memoryLevels[] = {5, 10, 15};
  if (this->instance) {
    this->instance->handleMemoryPressure(memoryLevels[memoryLevel]);
  }
}

void rnoh::RNInstanceArkTS::updateState(
    napi_env env,
    std::string const& componentName,
    facebook::react::Tag tag,
    napi_value newState) {
  if (auto state =
          m_shadowViewRegistry->getFabricState<facebook::react::State>(tag)) {
    m_mutationsToNapiConverter->updateState(
        env, componentName, state, newState);
  }
}

void RNInstanceArkTS::callFunction(
    std::string&& module,
    std::string&& method,
    folly::dynamic&& params) {
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

void RNInstanceArkTS::onAnimationStarted() {
  m_shouldRelayUITick.store(true);
}

void RNInstanceArkTS::onAllAnimationsComplete() {
  m_shouldRelayUITick.store(false);
}

void RNInstanceArkTS::onUITick() {
  if (this->m_shouldRelayUITick.load()) {
    this->scheduler->animationTick();
  }
}

void RNInstanceArkTS::handleArkTSMessage(
    const std::string& name,
    folly::dynamic const& payload) {
  for (auto const& arkTSMessageHandler : m_arkTSMessageHandlers) {
    arkTSMessageHandler->handleArkTSMessage(
        {.messageName = name,
         .messagePayload = payload,
         .rnInstance = this->shared_from_this()});
  }
}

void RNInstanceArkTS::synchronouslyUpdateViewOnUIThread(
    facebook::react::Tag tag,
    folly::dynamic props) {
  LOG(WARNING)
      << "RNInstance::synchronouslyUpdateViewOnUIThread is not supported in ArkTS architecture";
};

TurboModule::Shared RNInstanceArkTS::getTurboModule(const std::string& name) {
  auto turboModule = m_turboModuleProvider->getTurboModule(name);
  if (turboModule != nullptr) {
    auto rnohTurboModule = std::dynamic_pointer_cast<TurboModule>(turboModule);
    if (rnohTurboModule != nullptr) {
      return rnohTurboModule;
    } else {
      DLOG(ERROR) << "TurboModule '" << name
                  << "' should extend rnoh::TurboModule";
      return nullptr;
    }
  }
  return nullptr;
}

facebook::react::ContextContainer const&
rnoh::RNInstanceArkTS::getContextContainer() const {
  DLOG(INFO) << "RNInstanceArkTS::getContextContainer";
  return *m_contextContainer;
}

void RNInstanceArkTS::postMessageToArkTS(
    const std::string& name,
    folly::dynamic const& payload) {
  m_arkTSChannel->postMessage(name, payload);
};