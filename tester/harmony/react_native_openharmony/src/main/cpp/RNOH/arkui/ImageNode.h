/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <react/renderer/imagemanager/primitives.h>
#include "ArkUINode.h"

namespace rnoh {

class ImageNodeDelegate {
 public:
  virtual ~ImageNodeDelegate() = default;
  virtual void onComplete(float width, float height){};
  virtual void onError(int32_t errorCode){};
};

class ImageNode : public ArkUINode {
 protected:
  ArkUI_NodeHandle m_childArkUINodeHandle;
  ImageNodeDelegate* m_imageNodeDelegate;
  std::string m_uri;

 public:
  ImageNode();
  ~ImageNode();
  ImageNode& setSources(facebook::react::ImageSources const& src);
  ImageNode& setResizeMode(facebook::react::ImageResizeMode const& mode);
  ImageNode& setTintColor(facebook::react::SharedColor const& sharedColor);
  ImageNode& setBlur(facebook::react::Float blur);
  ImageNode& setObjectRepeat(
      facebook::react::ImageResizeMode const& resizeMode);

  ImageNode& setInterpolation(int32_t interpolation);
  ImageNode& setDraggable(bool draggable);
  ImageNode& setFocusable(bool focusable);
  ImageNode& setResizeMethod(std::string const& resizeMethod);
  ImageNode& setAlt(std::string const& uri);

  ImageNode& resetFocusable();
  ImageNode& resetResizeMethod();

  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;
  void setNodeDelegate(ImageNodeDelegate* imageNodeDelegate);

  std::string getUri();
};
} // namespace rnoh
