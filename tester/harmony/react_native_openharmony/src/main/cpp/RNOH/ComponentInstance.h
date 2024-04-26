/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/EventEmitter.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/core/State.h>
#include <vector>
#include "RNOH/ArkTSChannel.h"
#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/RNInstance.h"
#include "RNOH/TouchTarget.h"
#include "RNOH/arkui/ArkUINode.h"

namespace rnoh {

class ComponentInstance
    : public TouchTarget,
      public std::enable_shared_from_this<ComponentInstance> {
 private:
  std::string m_componentName;
  std::unordered_set<std::string> m_nativeResponderBlockOrigins;

 protected:
  using Tag = facebook::react::Tag;
  using ComponentHandle = facebook::react::ComponentHandle;

 public:
  struct Dependencies {
    using Shared = std::shared_ptr<const Dependencies>;

    ArkTSChannel::Shared arkTSChannel;
    ArkTSMessageHub::Shared arkTSMessageHub;
    RNInstance::Weak rnInstance;
  };

  struct Context {
    Tag tag;
    ComponentHandle componentHandle;
    std::string componentName;
    Dependencies::Shared dependencies;
  };

  virtual ArkUINode& getLocalRootArkUINode() = 0;

  using Shared = std::shared_ptr<ComponentInstance>;
  using Weak = std::weak_ptr<ComponentInstance>;

  ComponentInstance(Context ctx);

  virtual ~ComponentInstance() = default;

  Tag getTag() const {
    return m_tag;
  }

  std::string getId() const {
    auto props = getProps();
    if (props == nullptr) {
      return "";
    }
    return props->nativeId;
  }

  const std::string getComponentName() const {
    return m_componentName;
  }

  ComponentHandle getComponentHandle() const {
    return m_componentHandle;
  }

  void insertChild(
      ComponentInstance::Shared childComponentInstance,
      std::size_t index);

  void removeChild(ComponentInstance::Shared childComponentInstance);

  virtual facebook::react::Props::Shared getProps() const = 0;

  virtual void setProps(facebook::react::Props::Shared props){};

  virtual void setState(facebook::react::State::Shared state){};

  virtual void setLayout(facebook::react::LayoutMetrics layoutMetrics){};

  virtual void setEventEmitter(
      facebook::react::SharedEventEmitter eventEmitter){};

  virtual void finalizeUpdates() {}

  virtual void handleCommand(
      std::string const& commandName,
      folly::dynamic const& args) {}

  virtual std::vector<ComponentInstance::Shared> const& getChildren() const {
    return m_children;
  }

  virtual ComponentInstance::Weak const getParent() const {
    return m_parent;
  }

  virtual void setParent(ComponentInstance::Shared parent) {
    m_parent = parent;
  }

  void setNativeResponderBlocked(
      bool blocked,
      const std::string& origin = "REACT_NATIVE") {
    if (blocked) {
      m_nativeResponderBlockOrigins.insert(origin);
    } else if (
        m_nativeResponderBlockOrigins.find(origin) !=
        m_nativeResponderBlockOrigins.end()) {
      m_nativeResponderBlockOrigins.erase(origin);
    }
    onNativeResponderBlockChange(m_nativeResponderBlockOrigins.size() > 0);
  }

  // TouchTarget implementation
  Tag getTouchTargetTag() const override {
    return getTag();
  }

  facebook::react::SharedTouchEventEmitter getTouchEventEmitter()
      const override {
    return nullptr;
  }

  virtual std::vector<TouchTarget::Shared> getTouchTargetChildren() override {
    auto children = getChildren();
    return std::vector<TouchTarget::Shared>(children.begin(), children.end());
  }

  void setIgnoredPropKeys(std::unordered_set<std::string> propKeys) {
    m_ignoredPropKeys = std::move(propKeys);
  }

  std::unordered_set<std::string> const& getIgnoredPropKeys() const {
    return m_ignoredPropKeys;
  }

 protected:
  virtual void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) {}

  virtual void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) {}

  /**
   * Override this method if your component has scrollable functionality.
   * Temporarily blocking the scrolling is required to support continuous
   * gestures like panning within these scrollable components.
   *
   * @param isBlocked A boolean indicating whether scrolling should be blocked
   * or not.
   */
  virtual void onNativeResponderBlockChange(bool isBlocked) {}

  Tag m_tag;
  bool m_isRadiusSetValid = 0;
  float m_oldPointScaleFactor = 0.0f;
  ComponentHandle m_componentHandle;
  std::vector<ComponentInstance::Shared> m_children;
  ComponentInstance::Weak m_parent;
  facebook::react::BorderMetrics m_oldBorderMetrics;
  facebook::react::LayoutMetrics m_layoutMetrics;
  Dependencies::Shared m_deps;
  std::unordered_set<std::string> m_ignoredPropKeys;
};

} // namespace rnoh