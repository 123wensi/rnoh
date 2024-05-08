import { Tag } from "./DescriptorBase";

/**
 * Component(Instance)Manager. Unlike (ComponentInstance)Descriptor, exposes behavior and encapsulates data.
 */
export abstract class ComponentManager {
  onDestroy() {}
  abstract getParentTag(): Tag
  abstract getTag(): Tag
}
