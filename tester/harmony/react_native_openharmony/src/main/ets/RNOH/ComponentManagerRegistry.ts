import type { ComponentManager } from './ComponentManager';
import type { Tag } from './DescriptorBase';
import type { RNOHLogger } from "./RNOHLogger"

interface ComponentManagerRegistryEntry {
  componentManager: ComponentManager,
  refCounter: number,
}

/**
 * Stores ComponentManagers. Check ComponentManager documentation for more information.
 */
export class ComponentManagerRegistry {
  private componentManagersByTag: Map<Tag, ComponentManager[]>;
  private entryByTag: Map<Tag, ComponentManagerRegistryEntry> = new Map();
  private logger: RNOHLogger

  constructor(logger: RNOHLogger) {
    this.componentManagersByTag = new Map();
    this.logger = logger.clone("ComponentManagerRegistry")
  }

  public getComponentManager(tag: Tag): ComponentManager | undefined {
    const entry = this.entryByTag.get(tag);
    if (entry) {
      return entry.componentManager;
    }
    const componentManagers = this.componentManagersByTag.get(tag);
    if (!componentManagers || componentManagers.length === 0) {
      return undefined
    }
    if (componentManagers.length > 1) {
      this.logger.clone("getComponentManager").warn(`Found ${componentManagers.length} component managers with the same tag (${tag})`)
    }
    return componentManagers[componentManagers.length - 1]
  }

  /**
   * @param tag
   * @param manager
   * @deprecated Use findOrCreateComponentManager instead
   */
  public registerComponentManager(tag: Tag, manager: ComponentManager) {
    const componentManagers = this.componentManagersByTag.get(tag)
    if (!componentManagers) {
      this.componentManagersByTag.set(tag, [])
    }
    this.componentManagersByTag.get(tag)!.push(manager)

    return () => {
      const componentManagers = this.componentManagersByTag.get(tag)
      if (componentManagers) {
        manager.onDestroy()
        const filteredComponentManagers = componentManagers.filter(cm => cm !== manager)
        this.componentManagersByTag.set(tag, filteredComponentManagers)
        if (filteredComponentManagers.length === 0) {
          this.componentManagersByTag.delete(tag);
        }
      }
    }
  }

  /**
   * Returns the `ComponentManager` for a view,
   * creating it if there's none registered.
   * Each call to `findOrCreateComponentManager` must be matched 1:1
   * with a call to `releaseComponentManager` for the same `tag`
   * @param tag
   * tag of the view
   * @param createComponentManager
   * factory function to call when the manager has not yet been registered
   */
  public findOrCreateComponentManager<TComponentManager extends ComponentManager>(
    tag: Tag,
    createComponentManager: () => TComponentManager
  ): TComponentManager {
    const entry = this.entryByTag.get(tag);
    if (entry !== undefined) {
      entry.refCounter += 1
      return entry.componentManager as TComponentManager
    }

    const componentManager = createComponentManager();
    this.entryByTag.set(tag, { componentManager, refCounter: 1 });
    return componentManager;
  }

  /**
   * Called to allow releasing a `ComponentManager`
   * previously obtained from `findOrCreateComponentManager`.
   * Each call to `findOrCreateComponentManager` must be matched 1:1
   * with a call to `releaseComponentManager` for the same `tag`
   * @param tag
   */
  public releaseComponentManager(tag: Tag) {
    const entry = this.entryByTag.get(tag);
    if (entry === undefined) {
      this.logger.warn(`Tried to release already disposed component manager for tag: ${tag}`)
      return;
    }

    entry.refCounter -= 1;
    if (entry.refCounter === 0) {
      entry.componentManager.onDestroy();
      this.entryByTag.delete(tag);
    }
  }

  public getComponentManagerLineage(tag: Tag): ComponentManager[] {
    const results: ComponentManager[] = []
    let currentTag: Tag | undefined = tag
    do {
      const componentManager = this.getComponentManager(currentTag)
      if (!componentManager) {
        // https://gl.swmansion.com/rnoh/react-native-harmony/-/issues/707
        break;
      }
      currentTag = componentManager.getParentTag()
      results.push(componentManager)
    } while (currentTag !== undefined);
    return results.reverse();
  }
}