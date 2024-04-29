#include "PullToRefreshViewComponentInstance.h"
#include <react/renderer/components/rncore/Props.h>

using namespace rnoh;

PullToRefreshViewComponentInstance::PullToRefreshViewComponentInstance(
    Context context)
    : CppComponentInstance(std::move(context)) {
  m_refreshNode.setRefreshNodeDelegate(this);
  m_refreshNode.setRefreshContent(m_loadingProgressNode);
}

void PullToRefreshViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_refreshNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

RefreshNode& PullToRefreshViewComponentInstance::getLocalRootArkUINode() {
  return m_refreshNode;
}

void PullToRefreshViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_refreshNode.removeChild(childComponentInstance->getLocalRootArkUINode());
}

void PullToRefreshViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  CppComponentInstance::onPropsChanged(props);

  if (!m_props || m_props->title != props->title ||
      m_props->titleColor != props->titleColor) {
    m_loadingProgressNode.createOrUpdateLoadingProgressNodeTitle(
        props->title, props->titleColor);
  };

  if (props->rawProps.count("progressBackgroundColor") > 0) {
    m_loadingProgressNode.updateLoadingProgressBackgroundColor(
        props->rawProps["progressBackgroundColor"].asInt());
  };

  m_loadingProgressNode.setOffset(0, -props->progressViewOffset);
  m_refreshNode.setNativeRefreshing(props->refreshing);
  if (facebook::react::isColorMeaningful(props->tintColor)) {
    m_loadingProgressNode.updateLoadingSpinnerNodeColor(props->tintColor);
  }
}

void PullToRefreshViewComponentInstance::onRefresh() {
  m_refreshNode.setNativeRefreshing(true);
  m_eventEmitter->onRefresh({});
}