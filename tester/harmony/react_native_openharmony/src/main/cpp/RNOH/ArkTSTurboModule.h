#pragma once
#include <ReactCommon/CallbackWrapper.h>
#include <ReactCommon/TurboModuleUtils.h>
#include <jsi/JSIDynamic.h>
#include <jsi/jsi.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <variant>
#include "napi/native_api.h"

#include "ArkJS.h"
#include "RNOH/EventDispatcher.h"
#include "RNOH/MessageQueueThread.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "RNOH/TurboModule.h"

#define ARK_METHOD_CALLER(name)                        \
  [](facebook::jsi::Runtime& rt,                       \
     facebook::react::TurboModule& turboModule,        \
     const facebook::jsi::Value* args,                 \
     size_t count) {                                   \
    return static_cast<ArkTSTurboModule&>(turboModule) \
        .call(rt, #name, args, count);                 \
  }

#define ARK_ASYNC_METHOD_CALLER(name)                  \
  [](facebook::jsi::Runtime& rt,                       \
     facebook::react::TurboModule& turboModule,        \
     const facebook::jsi::Value* args,                 \
     size_t count) {                                   \
    return static_cast<ArkTSTurboModule&>(turboModule) \
        .callAsync(rt, #name, args, count);            \
  }

#define ARK_SCHEDULE_METHOD_CALLER(name)        \
  [](facebook::jsi::Runtime& rt,                \
     facebook::react::TurboModule& turboModule, \
     const facebook::jsi::Value* args,          \
     size_t count) {                            \
    static_cast<ArkTSTurboModule&>(turboModule) \
        .scheduleCall(rt, #name, args, count);  \
    return facebook::jsi::Value::undefined();   \
  }

#define ARK_METHOD_METADATA(name, argc)      \
  {                                          \
#name, { argc, ARK_METHOD_CALLER(name) } \
  }

#define ARK_ASYNC_METHOD_METADATA(name, argc)      \
  {                                                \
#name, { argc, ARK_ASYNC_METHOD_CALLER(name) } \
  }

#define ARK_SCHEDULE_METHOD_METADATA(name, argc)      \
  {                                                   \
#name, { argc, ARK_SCHEDULE_METHOD_CALLER(name) } \
  }

namespace rnoh {

class ArkTSTurboModule : public TurboModule {
 public:
  struct Context : public TurboModule::Context {
    napi_env env;
    napi_ref arkTsTurboModuleInstanceRef;
    std::shared_ptr<TaskExecutor> taskExecutor;
    std::shared_ptr<EventDispatcher> eventDispatcher;
    std::shared_ptr<MessageQueueThread> jsQueue;
    std::shared_ptr<facebook::react::Scheduler> scheduler;
  };

  ArkTSTurboModule(Context ctx, std::string name);

  facebook::jsi::Value call(
      facebook::jsi::Runtime& runtime,
      const std::string& methodName,
      const facebook::jsi::Value* args,
      size_t argsCount);

  folly::dynamic callSync(
      const std::string& methodName,
      std::vector<ArkJS::IntermediaryArg> args);

  void scheduleCall(
      facebook::jsi::Runtime& runtime,
      const std::string& methodName,
      const facebook::jsi::Value* args,
      size_t argsCount);

  facebook::jsi::Value callAsync(
      facebook::jsi::Runtime& runtime,
      const std::string& methodName,
      const facebook::jsi::Value* args,
      size_t argsCount);

  static std::vector<ArkJS::IntermediaryArg>
  convertJSIValuesToIntermediaryValues(
      facebook::jsi::Runtime& runtime,
      std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
      const facebook::jsi::Value* jsiArgs,
      size_t argsCount);

 protected:
  Context m_ctx;
};
} // namespace rnoh
