#pragma once
#include <react/renderer/core/ReactPrimitives.h>
#include "RNOH/ArkJS.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "glog/logging.h"

#ifdef C_API_ARCH
#include <arkui/native_node.h>
#include "arkui/native_node_napi.h"
#endif

namespace rnoh {
class CustomComponentArkUINodeHandleFactory {
 private:
  TaskExecutor::Shared m_taskExecutor;
  napi_env m_env;
  napi_ref m_customRNComponentFrameNodeFactoryRef;

 public:
  using Shared = std::shared_ptr<CustomComponentArkUINodeHandleFactory>;

  CustomComponentArkUINodeHandleFactory(
      napi_env env,
      napi_ref customRNComponentFrameNodeFactoryRef,
      TaskExecutor::Shared taskExecutor)
      : m_env(env),
        m_taskExecutor(taskExecutor),
        m_customRNComponentFrameNodeFactoryRef(
            customRNComponentFrameNodeFactoryRef) {}

  ArkUI_NodeHandle create(facebook::react::Tag tag, std::string componentName) {
#ifdef C_API_ARCH
    ArkUI_NodeHandle arkTSNodeHandle = nullptr;
    m_taskExecutor->runSyncTask(
        TaskThread::MAIN,
        [=, &arkTSNodeHandle, componentName = std::move(componentName)] {
          ArkJS arkJs(m_env);
          auto frameNodeFactory =
              arkJs.getObject(m_customRNComponentFrameNodeFactoryRef)
                  .getProperty("frameNodeFactory");
          auto n_result = arkJs.getObject(frameNodeFactory)
                              .call(
                                  "create",
                                  {arkJs.createInt(tag),
                                   arkJs.createString(componentName)});
          auto errorCode = OH_ArkUI_GetNodeHandleFromNapiValue(
              m_env, n_result, &arkTSNodeHandle);
          if (errorCode != 0) {
            LOG(ERROR) << "Couldn't get node handle. Error code: " << errorCode;
            arkTSNodeHandle = nullptr;
          }
        });
    return arkTSNodeHandle;
#else
    return nullptr;
#endif
  }
};
} // namespace rnoh