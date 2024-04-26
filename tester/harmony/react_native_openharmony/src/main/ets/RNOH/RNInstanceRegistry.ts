import type { RNInstance, RNInstanceOptions } from './RNInstance';
import { RNInstanceImpl, FrameNodeFactory } from './RNInstance';
import type { NapiBridge } from './NapiBridge';
import type { RNOHContext } from './RNOHContext';
import type { RNOHLogger } from './RNOHLogger';
import type { DevToolsController } from "./DevToolsController"
import { HttpClientProvider } from './HttpClientProvider';

export class RNInstanceRegistry {
  private instanceMap: Map<number, RNInstanceImpl> = new Map();

  constructor(
    private logger: RNOHLogger,
    private napiBridge: NapiBridge,
    private devToolsController: DevToolsController,
    private createRNOHContext: (rnInstance: RNInstance) => RNOHContext,
    private httpClientProvider: HttpClientProvider,
  ) {
  }

  public async createInstance(
    options: RNInstanceOptions,
    frameNodeFactory?: FrameNodeFactory,
  ): Promise<RNInstance> {
    const id = this.napiBridge.getNextRNInstanceId();
    const instance = new RNInstanceImpl(
      id,
      this.logger,
      frameNodeFactory ?? null,
      this.napiBridge,
      this.getDefaultProps(),
      this.devToolsController,
      this.createRNOHContext,
      options.enableDebugger ?? false,
      options.enableBackgroundExecutor ?? false,
      options.enableNDKTextMeasuring ?? false,
      options.enableImageLoader ?? false,
      options.enableCAPIArchitecture ?? false,
      options.assetsDest,
      this.httpClientProvider,
    )
    await instance.initialize(options.createRNPackages({}))
    this.instanceMap.set(id, instance)
    return instance;
  }

  public getInstance(id: number): RNInstance {
    return this.instanceMap.get(id);
  }

  public deleteInstance(id: number): boolean {
    if (this.instanceMap.has(id)) {
      this.instanceMap.delete(id);
      return true;
    }
    return false;
  }

  public forEach(cb: (rnInstance: RNInstanceImpl) => void) {
    this.instanceMap.forEach(cb)
  }

  private getDefaultProps(): Record<string, any> {
    return { concurrentRoot: true }
  }
}