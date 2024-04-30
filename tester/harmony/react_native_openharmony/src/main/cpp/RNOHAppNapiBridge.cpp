#include <ace/xcomponent/native_interface_xcomponent.h>
#include <cxxreact/JSExecutor.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "RNInstanceFactory.h"
#include "RNOH/ArkJS.h"
#include "RNOH/ArkTSBridge.h"
#include "RNOH/Inspector.h"
#include "RNOH/LogSink.h"
#include "RNOH/Performance/HarmonyReactMarker.h"
#include "RNOH/RNInstance.h"
#include "RNOH/RNInstanceCAPI.h"
#include "RNOH/TaskExecutor/ThreadTaskRunner.h"
#include "RNOH/UITicker.h"
#include "RNOH/arkui/ArkUINodeRegistry.h"
#include "napi/native_api.h"

std::mutex rnInstanceByIdMutex;
std::unordered_map<size_t, std::shared_ptr<RNInstanceInternal>> rnInstanceById;
auto uiTicker = std::make_shared<UITicker>();
static auto cleanupRunner = std::make_unique<ThreadTaskRunner>("RNOH_CLEANUP");

static napi_value onInit(napi_env env, napi_callback_info info) {
  HarmonyReactMarker::setLogPerfMarkerIfNeeded();
  LogSink::initializeLogging();
  auto logVerbosityLevel = 0;
#ifdef LOG_VERBOSITY_LEVEL
  FLAGS_v = LOG_VERBOSITY_LEVEL;
  logVerbosityLevel = LOG_VERBOSITY_LEVEL;
#endif
  DLOG(INFO) << "onInit (LOG_VERBOSITY_LEVEL=" << logVerbosityLevel << ")";
  ArkJS arkJs(env);
  auto args = arkJs.getCallbackArgs(info, 1);
  auto shouldClearRNInstances = arkJs.getBoolean(args[0]);
  if (shouldClearRNInstances) {
    /**
     * This CPP code can survive closing an app. The app can be closed before
     * removing all RNInstances. As a workaround, all rnInstances are removed on
     * the start.
     */
    cleanupRunner->runAsyncTask([] {
      decltype(rnInstanceById) instances;
      {
        std::lock_guard<std::mutex> lock(rnInstanceByIdMutex);
        std::swap(rnInstanceById, instances);
      }
      instances.clear();
    });
  }
  auto isDebugModeEnabled = false;
#ifdef REACT_NATIVE_DEBUG
  isDebugModeEnabled = true;
#endif
  return arkJs.createObjectBuilder()
      .addProperty("isDebugModeEnabled", isDebugModeEnabled)
      .build();
}

napi_value initializeArkTSBridge(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  auto args = arkJs.getCallbackArgs(info, 1);
  auto bridgeHandlerRef = arkJs.createReference(args[0]);
  ArkTSBridge::initializeInstance(env, bridgeHandlerRef);
#ifdef C_API_ARCH
  ArkUINodeRegistry::initialize(ArkTSBridge::getInstance());
#endif
  return arkJs.getUndefined();
}

static napi_value getNextRNInstanceId(
    napi_env env,
    napi_callback_info /*info*/) {
  try {
    static std::atomic_size_t nextId = 0;
    auto id = nextId++;
    DLOG(INFO) << "getNextRNInstanceId: " << id;
    return ArkJS(env).createInt(id);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
    return ArkJS(env).createInt(-1);
  }
}

static napi_value createReactNativeInstance(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    DLOG(INFO) << "createReactNativeInstance";
    HarmonyReactMarker::setAppStartTime(
        facebook::react::JSExecutor::performanceNow());
    auto args = arkJs.getCallbackArgs(info, 10);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto arkTsTurboModuleProviderRef = arkJs.createReference(args[1]);
    auto mutationsListenerRef = arkJs.createReference(args[2]);
    auto commandDispatcherRef = arkJs.createReference(args[3]);
    auto eventDispatcherRef = arkJs.createReference(args[4]);
    auto measureTextFnRef = arkJs.createReference(args[5]);
    auto shouldEnableDebugger = arkJs.getBoolean(args[6]);
    auto shouldEnableBackgroundExecutor = arkJs.getBoolean(args[7]);
    auto featureFlagRegistry = std::make_shared<FeatureFlagRegistry>();
    for (auto featureFlagNameAndStatus : arkJs.getObjectProperties(args[8])) {
      featureFlagRegistry->setFeatureFlagStatus(
          arkJs.getString(featureFlagNameAndStatus.first),
          arkJs.getBoolean(featureFlagNameAndStatus.second));
    }
    auto frameNodeFactoryRef = arkJs.createReference(args[9]);
    auto rnInstance = createRNInstance(
        instanceId,
        env,
        arkTsTurboModuleProviderRef,
        frameNodeFactoryRef,
        [env, instanceId, mutationsListenerRef](
            auto const& mutationsToNapiConverter, auto const& mutations) {
          {
            auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
            if (rnInstanceById.find(instanceId) == rnInstanceById.end()) {
              LOG(WARNING) << "RNInstance with the following id " +
                      std::to_string(instanceId) + " does not exist";
              return;
            }
          }
          ArkJS arkJs(env);
          auto napiMutations = mutationsToNapiConverter.convert(env, mutations);
          std::array<napi_value, 1> args = {napiMutations};
          auto listener = arkJs.getReferenceValue(mutationsListenerRef);
          arkJs.call<1>(listener, args);
        },
        [env, instanceId, commandDispatcherRef](
            auto tag, auto const& commandName, auto args) {
          {
            auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
            if (rnInstanceById.find(instanceId) == rnInstanceById.end()) {
              LOG(WARNING) << "RNInstance with the following id " +
                      std::to_string(instanceId) + " does not exist";
              return;
            }
          }
          ArkJS arkJs(env);
          auto napiArgs = arkJs.convertIntermediaryValueToNapiValue(args);
          std::array<napi_value, 3> napiArgsArray = {
              arkJs.createDouble(tag),
              arkJs.createString(commandName),
              napiArgs};
          auto commandDispatcher =
              arkJs.getReferenceValue(commandDispatcherRef);
          arkJs.call<3>(commandDispatcher, napiArgsArray);
        },
        measureTextFnRef,
        eventDispatcherRef,
        featureFlagRegistry,
        uiTicker,
        shouldEnableDebugger,
        shouldEnableBackgroundExecutor);

    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    if (rnInstanceById.find(instanceId) != rnInstanceById.end()) {
      LOG(FATAL) << "RNInstance with the following id " +
              std::to_string(instanceId) + " has been already created";
    }
    auto [it, _inserted] =
        rnInstanceById.emplace(instanceId, std::move(rnInstance));
    it->second->start();
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value destroyReactNativeInstance(
    napi_env env,
    napi_callback_info info) {
  DLOG(INFO) << "destroyReactNativeInstance";
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 1);
    size_t rnInstanceId = arkJs.getDouble(args[0]);
    cleanupRunner->runAsyncTask([rnInstanceId] {
      std::shared_ptr<RNInstanceInternal> instance;
      {
        auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
        if (auto it = rnInstanceById.find(rnInstanceId);
            it != rnInstanceById.end()) {
          std::swap(it->second, instance);
          rnInstanceById.erase(rnInstanceId);
        }
      }
    });
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value loadScript(napi_env env, napi_callback_info info) {
  DLOG(INFO) << "loadScript";
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 4);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    auto onFinishRef = arkJs.createReference(args[3]);
    rnInstance->loadScript(
        arkJs.getArrayBuffer(args[1]),
        arkJs.getString(args[2]),
        [taskExecutor = rnInstance->getTaskExecutor(), env, onFinishRef](
            const std::string& errorMsg) {
          taskExecutor->runTask(
              TaskThread::MAIN, [env, onFinishRef, errorMsg]() {
                ArkJS arkJs(env);
                auto listener = arkJs.getReferenceValue(onFinishRef);
                arkJs.call<1>(listener, {arkJs.createString(errorMsg)});
                arkJs.deleteReference(onFinishRef);
              });
        });
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value updateSurfaceConstraints(
    napi_env env,
    napi_callback_info info) {
  DLOG(INFO) << "updateSurfaceConstraints\n";
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 8);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    rnInstance->updateSurfaceConstraints(
        arkJs.getDouble(args[1]),
        arkJs.getDouble(args[2]),
        arkJs.getDouble(args[3]),
        arkJs.getDouble(args[4]),
        arkJs.getDouble(args[5]),
        arkJs.getDouble(args[6]),
        arkJs.getBoolean(args[7]));
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }

  return arkJs.getUndefined();
}

static napi_value createSurface(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 3);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    facebook::react::Tag surfaceId = arkJs.getDouble(args[1]);
    DLOG(INFO) << "createSurface: surfaceId=" << surfaceId;
    auto appKey = arkJs.getString(args[2]);
    rnInstance->createSurface(surfaceId, appKey);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value startSurface(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 9);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    facebook::react::Tag surfaceId = arkJs.getDouble(args[1]);
    DLOG(INFO) << "startSurface: surfaceId=" << surfaceId << "\n";
    rnInstance->startSurface(
        surfaceId,
        arkJs.getDouble(args[2]),
        arkJs.getDouble(args[3]),
        arkJs.getDouble(args[4]),
        arkJs.getDouble(args[5]),
        arkJs.getDouble(args[6]),
        arkJs.getBoolean(args[7]),
        arkJs.getDynamic(args[8]));
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value stopSurface(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 2);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    facebook::react::Tag surfaceId = arkJs.getDouble(args[1]);
    DLOG(INFO) << "stopSurface: surfaceId=" << surfaceId << "\n";
    rnInstance->stopSurface(surfaceId);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value destroySurface(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 2);
    size_t instanceId = arkJs.getDouble(args[0]);
    facebook::react::Tag surfaceId = arkJs.getDouble(args[1]);
    DLOG(INFO) << "destroySurface: surfaceId=" << surfaceId << "\n";
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    rnInstance->destroySurface(surfaceId);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value setSurfaceDisplayMode(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 3);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    facebook::react::Tag surfaceId = arkJs.getDouble(args[1]);
    DLOG(INFO) << "setSurfaceDisplayMode: surfaceId=" << surfaceId << "\n";
    rnInstance->setSurfaceDisplayMode(
        surfaceId,
        static_cast<facebook::react::DisplayMode>(arkJs.getDouble(args[2])));
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value emitComponentEvent(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 5);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    rnInstance->emitComponentEvent(
        env, arkJs.getDouble(args[1]), arkJs.getString(args[2]), args[3]);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value callRNFunction(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 4);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    auto moduleString = arkJs.getString(args[1]);
    auto nameString = arkJs.getString(args[2]);
    auto argsDynamic = arkJs.getDynamic(args[3]);
    rnInstance->callFunction(
        std::move(moduleString), std::move(nameString), std::move(argsDynamic));
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }
  return arkJs.getUndefined();
}

static napi_value onMemoryLevel(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 1);
    auto memoryLevel = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    for (auto& [id, instance] : rnInstanceById) {
      if (instance != nullptr) {
        instance->onMemoryLevel(static_cast<size_t>(memoryLevel));
      }
    }
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }

  return arkJs.getUndefined();
}

static napi_value updateState(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 4);
    size_t instanceId = arkJs.getDouble(args[0]);
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(instanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    auto componentName = arkJs.getString(args[1]);
    auto tag = arkJs.getDouble(args[2]);
    auto state = args[3];
    rnInstance->updateState(
        env, componentName, static_cast<facebook::react::Tag>(tag), state);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }

  return arkJs.getUndefined();
}

static napi_value onArkTSMessage(napi_env env, napi_callback_info info) {
  ArkJS arkJs(env);
  try {
    auto args = arkJs.getCallbackArgs(info, 2);
    auto messageName = arkJs.getString(args[0]);
    auto wrappedPayload = arkJs.getDynamic(args[1]);
    auto rnInstanceId = wrappedPayload["rnInstanceId"].getDouble();
    auto messagePayload = wrappedPayload["payload"];
    auto lock = std::lock_guard<std::mutex>(rnInstanceByIdMutex);
    auto it = rnInstanceById.find(rnInstanceId);
    if (it == rnInstanceById.end()) {
      return arkJs.getUndefined();
    }
    auto& rnInstance = it->second;
    rnInstance->handleArkTSMessage(messageName, messagePayload);
  } catch (...) {
    ArkTSBridge::getInstance()->handleError(std::current_exception());
  }

  return arkJs.getUndefined();
}

static void registerNativeXComponent(napi_env env, napi_value exports) {
  if ((env == nullptr) || (exports == nullptr)) {
    LOG(ERROR) << "registerNativeXComponent: env or exports is null"
               << "\n";
    return;
  }

  napi_value exportInstance = nullptr;
  if (napi_get_named_property(
          env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
    LOG(ERROR) << "registerNativeXComponent: napi_get_named_property fail"
               << "\n";
    return;
  }

  OH_NativeXComponent* nativeXComponent = nullptr;
  if (napi_unwrap(
          env, exportInstance, reinterpret_cast<void**>(&nativeXComponent)) !=
      napi_ok) {
    LOG(ERROR) << "registerNativeXComponent: napi_get_named_property fail"
               << "\n";
    return;
  }

  char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
  uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
  if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) !=
      OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
    LOG(ERROR)
        << "registerNativeXComponent: OH_NativeXComponent_GetXComponentId fail"
        << "\n";
    return;
  }
  std::string xcomponentStr(idStr);
  std::stringstream ss(xcomponentStr);
  std::string instanceId;
  std::getline(ss, instanceId, '_');
  std::string surfaceId;
  std::getline(ss, surfaceId, '_');
  size_t instanceIdNum = std::stod(instanceId, nullptr);
  if (rnInstanceById.find(instanceIdNum) == rnInstanceById.end()) {
    LOG(ERROR) << "RNInstance with the following id " +
            std::to_string(instanceIdNum) + " does not exist";
    return;
  }
  auto& rnInstance = rnInstanceById.at(instanceIdNum);

  auto* rnInstanceCPIRawPtr = dynamic_cast<RNInstanceCAPI*>(rnInstance.get());
  if (rnInstanceCPIRawPtr != nullptr) {
    rnInstanceCPIRawPtr->registerNativeXComponentHandle(
        nativeXComponent, std::stoi(surfaceId));
  }
  DLOG(INFO) << "registerNativeXComponent: id = " << instanceId << "\n";
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"onInit",
       nullptr,
       onInit,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"getNextRNInstanceId",
       nullptr,
       getNextRNInstanceId,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"createReactNativeInstance",
       nullptr,
       createReactNativeInstance,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"destroyReactNativeInstance",
       nullptr,
       destroyReactNativeInstance,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"loadScript",
       nullptr,
       loadScript,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"startSurface",
       nullptr,
       startSurface,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"stopSurface",
       nullptr,
       stopSurface,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"destroySurface",
       nullptr,
       destroySurface,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"createSurface",
       nullptr,
       createSurface,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"updateSurfaceConstraints",
       nullptr,
       updateSurfaceConstraints,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"setSurfaceDisplayMode",
       nullptr,
       setSurfaceDisplayMode,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"onArkTSMessage",
       nullptr,
       onArkTSMessage,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"emitComponentEvent",
       nullptr,
       emitComponentEvent,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"callRNFunction",
       nullptr,
       callRNFunction,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"onMemoryLevel",
       nullptr,
       onMemoryLevel,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"updateState",
       nullptr,
       updateState,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"getInspectorWrapper",
       nullptr,
       rnoh::getInspectorWrapper,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr},
      {"initializeArkTSBridge",
       nullptr,
       ::initializeArkTSBridge,
       nullptr,
       nullptr,
       nullptr,
       napi_default,
       nullptr}};

  napi_define_properties(
      env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc);
  registerNativeXComponent(
      env, exports); // NOTE: shouldn't this be called when creating surface?
  return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "rnoh_app",
    .nm_priv = ((void*)nullptr),
    .reserved = {nullptr},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  napi_module_register(&demoModule);
}
