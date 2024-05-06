/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <folly/dynamic.h>
#include <react/renderer/core/ComponentDescriptor.h>
#include <react/renderer/scheduler/SchedulerDelegate.h>
#include "RNOH/ComponentInstanceFactory.h"
#include "RNOH/ComponentInstanceRegistry.h"
#include "RNOH/MountingManager.h"
#include "RNOH/SchedulerDelegateArkTS.h"
#include "RNOH/ShadowViewRegistry.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"

namespace facebook {
namespace react {
class Scheduler;
}
} // namespace facebook

namespace rnoh {

class SchedulerDelegateCAPI : public facebook::react::SchedulerDelegate {
 public:
  SchedulerDelegateCAPI(
      TaskExecutor::Shared taskExecutor,
      ComponentInstanceRegistry::Shared componentInstanceRegistry,
      ComponentInstanceFactory::Shared componentInstanceFactory,
      rnoh::SchedulerDelegateArkTS::Unique schedulerDelegateArkTS,
      MountingManager::Shared mountingManager)
      : m_taskExecutor(taskExecutor),
        m_componentInstanceRegistry(std::move(componentInstanceRegistry)),
        m_componentInstanceFactory(std::move(componentInstanceFactory)),
        m_schedulerDelegateArkTS(std::move(schedulerDelegateArkTS)),
        m_mountingManager(std::move(mountingManager)){};

  ~SchedulerDelegateCAPI() {
    VLOG(1) << "~SchedulerDelegateCAPI";
  }

  void schedulerDidFinishTransaction(
      facebook::react::MountingCoordinator::Shared mountingCoordinator)
      override {
    mountingCoordinator->getTelemetryController().pullTransaction(
        [this](
            facebook::react::MountingTransaction const& transaction,
            facebook::react::SurfaceTelemetry const& surfaceTelemetry) {
          // Will mount
        },
        [this](
            facebook::react::MountingTransaction const& transaction,
            facebook::react::SurfaceTelemetry const& surfaceTelemetry) {
          // Mounting
          m_mountingManager->performMountInstructions(
              transaction.getMutations(), transaction.getSurfaceId());
        },
        [this](
            facebook::react::MountingTransaction const& transaction,
            facebook::react::SurfaceTelemetry const& surfaceTelemetry) {
          // Did mount
          auto mutations = transaction.getMutations();
          m_mountingManager->processMutations(mutations);
          m_taskExecutor->runTask(TaskThread::MAIN, [this, mutations] {
            for (auto mutation : mutations) {
              try {
                this->handleMutation(mutation);
              } catch (std::runtime_error& e) {
                LOG(ERROR) << "Mutation "
                           << this->getMutationNameFromType(mutation.type)
                           << " failed: " << e.what();
              }
            }
            finalizeMutationUpdates(mutations);
          });
        });
  }

  void schedulerDidRequestPreliminaryViewAllocation(
      facebook::react::SurfaceId surfaceId,
      const facebook::react::ShadowNode& shadowNode) override {}

  void schedulerDidDispatchCommand(
      const facebook::react::ShadowView& shadowView,
      std::string const& commandName,
      folly::dynamic const& args) override {
    m_schedulerDelegateArkTS->schedulerDidDispatchCommand(
        shadowView, commandName, args);
    m_taskExecutor->runTask(
        TaskThread::MAIN, [this, tag = shadowView.tag, commandName, args] {
          auto componentInstance = m_componentInstanceRegistry->findByTag(tag);
          if (componentInstance != nullptr) {
            componentInstance->handleCommand(commandName, args);
          }
        });
  }

  void schedulerDidSendAccessibilityEvent(
      const facebook::react::ShadowView& shadowView,
      std::string const& eventType) override {}

  void schedulerDidSetIsJSResponder(
      facebook::react::ShadowView const& shadowView,
      bool isJSResponder,
      bool blockNativeResponder) override;

  void synchronouslyUpdateViewOnUIThread(
      facebook::react::Tag tag,
      folly::dynamic props,
      facebook::react::ComponentDescriptor const& componentDescriptor);

 private:
  TaskExecutor::Shared m_taskExecutor;
  ComponentInstanceRegistry::Shared m_componentInstanceRegistry;
  ComponentInstanceFactory::Shared m_componentInstanceFactory;
  facebook::react::ContextContainer::Shared m_contextContainer;
  rnoh::SchedulerDelegateArkTS::Unique m_schedulerDelegateArkTS;
  MountingManager::Shared m_mountingManager;

  void updateComponentWithShadowView(
      ComponentInstance::Shared const& componentInstance,
      facebook::react::ShadowView const& shadowView) {
    // NOTE: updating tag by id must happen before updating props
    m_componentInstanceRegistry->updateTagById(
        shadowView.tag, shadowView.props->nativeId, componentInstance->getId());
    componentInstance->setLayout(shadowView.layoutMetrics);
    componentInstance->setEventEmitter(shadowView.eventEmitter);
    componentInstance->setState(shadowView.state);
    componentInstance->setProps(shadowView.props);
  }

  void handleMutation(facebook::react::ShadowViewMutation mutation) {
    VLOG(1) << "Mutation (type:" << this->getMutationNameFromType(mutation.type)
            << "; componentName: "
            << (mutation.newChildShadowView.componentName != nullptr
                    ? mutation.newChildShadowView.componentName
                    : "null")
            << "; newTag: " << mutation.newChildShadowView.tag
            << "; oldTag: " << mutation.oldChildShadowView.tag
            << "; parentTag: " << mutation.parentShadowView.tag << ")";
    switch (mutation.type) {
      case facebook::react::ShadowViewMutation::Create: {
        auto newChild = mutation.newChildShadowView;
        auto componentInstance = m_componentInstanceFactory->create(
            newChild.tag, newChild.componentHandle, newChild.componentName);
        if (componentInstance != nullptr) {
          updateComponentWithShadowView(componentInstance, newChild);
          m_componentInstanceRegistry->insert(componentInstance);
        } else {
          LOG(INFO) << "Couldn't create CppComponentInstance for: "
                    << newChild.componentName;
        }
        break;
      }
      case facebook::react::ShadowViewMutation::Delete: {
        auto oldChild = mutation.oldChildShadowView;
        m_componentInstanceRegistry->deleteByTag(oldChild.tag);
        break;
      }
      case facebook::react::ShadowViewMutation::Insert: {
        auto parentComponentInstance = m_componentInstanceRegistry->findByTag(
            mutation.parentShadowView.tag);
        auto newChildComponentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        // Building subtrees through insert is bottom-up.
        // C++ nodes and ArkTS subtrees cannot be inserted into their ArkTS
        // parent unless the parent is a C++ node. builderNode is then
        // constructed in the ArkTS subtree of C++ nodes.
        if (newChildComponentInstance == nullptr &&
            parentComponentInstance != nullptr) {
          newChildComponentInstance =
              m_componentInstanceFactory->createArkTSComponent(
                  mutation.newChildShadowView.tag,
                  mutation.newChildShadowView.componentHandle,
                  mutation.newChildShadowView.componentName);
          if (newChildComponentInstance != nullptr) {
            m_componentInstanceRegistry->insert(newChildComponentInstance);
          } else {
            LOG(FATAL) << "Couldn't create ArkTSComponentInstance for: "
                       << mutation.newChildShadowView.componentName;
          }
        }

        if (parentComponentInstance != nullptr &&
            newChildComponentInstance != nullptr) {
          parentComponentInstance->insertChild(
              newChildComponentInstance, mutation.index);
        }
        break;
      }
      case facebook::react::ShadowViewMutation::Remove: {
        auto parentComponentInstance = m_componentInstanceRegistry->findByTag(
            mutation.parentShadowView.tag);
        if (parentComponentInstance) {
          parentComponentInstance->removeChild(
              m_componentInstanceRegistry->findByTag(
                  mutation.oldChildShadowView.tag));
        }

        auto newChildComponentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        if (newChildComponentInstance &&
            std::dynamic_pointer_cast<FallbackComponentInstance>(
                newChildComponentInstance)) {
          m_componentInstanceRegistry->deleteByTag(
              mutation.newChildShadowView.tag);
        }
        break;
      }
      case facebook::react::ShadowViewMutation::Update: {
        auto componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        if (componentInstance != nullptr) {
          updateComponentWithShadowView(
              componentInstance, mutation.newChildShadowView);
        }
        break;
      }
    }
  }

  void finalizeMutationUpdates(
      facebook::react::ShadowViewMutationList const& mutations);

  std::string getMutationNameFromType(
      facebook::react::ShadowViewMutation::Type mutationType) {
    switch (mutationType) {
      case facebook::react::ShadowViewMutation::Create:
        return "CREATE";
      case facebook::react::ShadowViewMutation::Delete:
        return "DELETE";
      case facebook::react::ShadowViewMutation::Update:
        return "UPDATE";
      case facebook::react::ShadowViewMutation::Insert:
        return "INSERT";
      case facebook::react::ShadowViewMutation::Remove:
        return "REMOVE";
      case facebook::react::ShadowViewMutation::RemoveDeleteTree:
        return "REMOVE_DELETE_TREE";
      default:
        return "UNKNOWN";
    }
  }
};

} // namespace rnoh
