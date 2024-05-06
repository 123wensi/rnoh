#include "SchedulerDelegateCAPI.h"
#include <react/renderer/scheduler/Scheduler.h>

namespace rnoh {

using namespace facebook;

std::optional<react::SurfaceId> findSurfaceIdForComponentInstance(
    ComponentInstance::Shared componentInstance) {
  auto parentInstance = componentInstance->getParent().lock();
  while (parentInstance != nullptr) {
    componentInstance = std::move(parentInstance);
    parentInstance = componentInstance->getParent().lock();
  }
  auto rootComponentTag = componentInstance->getTag();

  // Fabric reserves numbers equal 1 mod 10 for SurfaceIds
  if (rootComponentTag % 10 == 1) {
    return static_cast<react::SurfaceId>(rootComponentTag);
  }

  return std::nullopt;
}

void SchedulerDelegateCAPI::schedulerDidSetIsJSResponder(
    facebook::react::ShadowView const& shadowView,
    bool isJSResponder,
    bool blockNativeResponder) {
  m_schedulerDelegateArkTS->schedulerDidSetIsJSResponder(
      shadowView, isJSResponder, blockNativeResponder);

  auto componentInstance =
      m_componentInstanceRegistry->findByTag(shadowView.tag);
  while (componentInstance != nullptr) {
    componentInstance->setNativeResponderBlocked(blockNativeResponder);
    componentInstance = componentInstance->getParent().lock();
  }
}

void SchedulerDelegateCAPI::synchronouslyUpdateViewOnUIThread(
    react::Tag tag,
    folly::dynamic props,
    react::ComponentDescriptor const& componentDescriptor) {
  auto componentInstance = m_componentInstanceRegistry->findByTag(tag);
  if (componentInstance == nullptr) {
    return;
  }

  auto surfaceId = findSurfaceIdForComponentInstance(componentInstance);
  if (!surfaceId.has_value()) {
    return;
  }

  std::unordered_set<std::string> propKeys =
      componentInstance->getIgnoredPropKeys();
  for (const auto& key : props.keys()) {
    propKeys.insert(key.getString());
  }

  auto oldProps = componentInstance->getProps();
  auto newProps = componentDescriptor.cloneProps(
      react::PropsParserContext{surfaceId.value(), *m_contextContainer},
      oldProps,
      std::move(props));

  componentInstance->setIgnoredPropKeys({});
  componentInstance->setProps(newProps);
  componentInstance->finalizeUpdates();
  componentInstance->setIgnoredPropKeys(std::move(propKeys));
}

void SchedulerDelegateCAPI::finalizeMutationUpdates(
    facebook::react::ShadowViewMutationList const& mutations) {
  std::unordered_set<ComponentInstance::Shared> componentInstancesToFinalize;

  for (const auto& mutation : mutations) {
    std::shared_ptr<ComponentInstance> componentInstance = nullptr;
    switch (mutation.type) {
      case facebook::react::ShadowViewMutation::Create: {
        componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        break;
      }
      case facebook::react::ShadowViewMutation::Delete: {
        break;
      }
      case facebook::react::ShadowViewMutation::Insert: {
        componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.parentShadowView.tag);
        break;
      }
      case facebook::react::ShadowViewMutation::Remove: {
        componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.parentShadowView.tag);
        break;
      }
      case facebook::react::ShadowViewMutation::Update: {
        componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        break;
      }
    }
    if (componentInstance != nullptr) {
      componentInstancesToFinalize.insert(componentInstance);
    }
  }
  for (const auto& componentInstance : componentInstancesToFinalize) {
    componentInstance->finalizeUpdates();
  }
}

} // namespace rnoh
