#pragma once
#include <functional>
#include <butter/map.h>
#include <ReactCommon/TurboModule.h>
#include <ReactCommon/RuntimeExecutor.h>
#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/LongLivedObject.h>
#include <react/renderer/scheduler/Scheduler.h>
#include "RNOH/TurboModuleFactory.h"
#include "EventDispatcher.h"

namespace rnoh {

class TurboModuleProvider : public std::enable_shared_from_this<TurboModuleProvider> {
  public:
    TurboModuleProvider(std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
                        TurboModuleFactory &&turboModuleFactory,
                        std::shared_ptr<EventDispatcher> eventDispatcher,
                        std::shared_ptr<MessageQueueThread> jsQueue,
                        std::shared_ptr<RNInstance> const &instance = nullptr);

    std::shared_ptr<facebook::react::TurboModule> getTurboModule(std::string const &moduleName);
    void installJSBindings(facebook::react::RuntimeExecutor runtimeExecutor);
    void setScheduler(std::shared_ptr<facebook::react::Scheduler> scheduler) {
        this->m_scheduler = scheduler;
    }

  private:
    std::shared_ptr<facebook::react::CallInvoker> m_jsInvoker;
    std::weak_ptr<RNInstance> m_instance;
    std::function<std::shared_ptr<facebook::react::TurboModule>(std::string const &, std::shared_ptr<facebook::react::CallInvoker>,
                                                                std::shared_ptr<facebook::react::Scheduler>)>
        m_createTurboModule;
    facebook::butter::map<std::string, std::shared_ptr<facebook::react::TurboModule>> m_cache;
    std::shared_ptr<facebook::react::Scheduler> m_scheduler;
};

} // namespace rnoh