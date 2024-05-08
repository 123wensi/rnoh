import type { TurboModule, TurboModuleContext } from "./TurboModule";
import type { DescriptorWrapperFactory } from "./DescriptorRegistry"

export abstract class TurboModulesFactory {
  constructor(protected ctx: TurboModuleContext) {
  }

  abstract createTurboModule(name: string): TurboModule | null;

  prepareEagerTurboModules(): Promise<void> {
    return Promise.resolve()
  }

  abstract hasTurboModule(name: string): boolean;
}

class FakeTurboModulesFactory extends TurboModulesFactory {
  createTurboModule(name: string) {
    return null
  }

  hasTurboModule(name: string) {
    return false
  }
}

/**
 * Currently empty. This context provides a way to inject dependencies in the future without in non-breaking changes manner.
 */
export type RNPackageContext = {};
export type DescriptorWrapperFactoryByDescriptorTypeCtx = {}
export type DescriptorWrapperFactoryByDescriptorType = Record<string, DescriptorWrapperFactory>

export abstract class RNPackage {
  constructor(protected ctx: RNPackageContext) {
  };

  createTurboModulesFactory(ctx: TurboModuleContext): TurboModulesFactory {
    return new FakeTurboModulesFactory(ctx)
  };

  getDebugName(): string | undefined {
    return undefined
  }

  createDescriptorWrapperFactoryByDescriptorType(ctx: DescriptorWrapperFactoryByDescriptorTypeCtx): DescriptorWrapperFactoryByDescriptorType {
    return {}
  }
}