import type { DescriptorRegistry } from './DescriptorRegistry';
import type { RNComponentCommandReceiver } from './RNComponentCommandHub';
import { RNInstance, RNInstanceImpl } from './RNInstance';
import type { ComponentManagerRegistry } from './ComponentManagerRegistry';
import { RNOHCoreContext } from './RNOHCoreContext';
import { HttpClient } from '../HttpClient/HttpClient';

export class RNOHContext extends RNOHCoreContext {
  static fromCoreContext(coreContext: RNOHCoreContext, rnInstance: RNInstance) {
    if (!(rnInstance instanceof RNInstanceImpl)) {
      throw new Error("RNInstance must extend RNInstanceImpl")
    }
    return new RNOHContext("0.72.5", rnInstance, coreContext);
  }

  public get descriptorRegistry(): DescriptorRegistry {
    return this.rnInstance.descriptorRegistry;
  }

  public get componentCommandReceiver(): RNComponentCommandReceiver {
    return this.rnInstanceImpl.componentCommandHub;
  }

  public get componentManagerRegistry(): ComponentManagerRegistry {
    return this.rnInstance.componentManagerRegistry;
  }

  public get rnInstance(): RNInstance {
    return this.rnInstanceImpl
  }

  public get httpClient(): HttpClient {
    return this.rnInstance.httpClient;
  }

  protected constructor(
    public reactNativeVersion: string,
    private rnInstanceImpl: RNInstanceImpl,
    coreContext: RNOHCoreContext
  ) {
    super(
      coreContext.createAndRegisterRNInstance,
      coreContext.destroyAndUnregisterRNInstance,
      coreContext.logger,
      coreContext.getDisplayMetrics,
      coreContext.getUIAbilityState,
      coreContext.dispatchBackPress,
      coreContext.markReadiness,
      coreContext.uiAbilityContext,
      coreContext.devToolsController,
      coreContext.devMenu,
      coreContext.safeAreaInsetsProvider,
      coreContext.isDebugModeEnabled,
      coreContext.launchUri,
      coreContext.defaultBackPressHandler
    )
    this.devToolsController = coreContext.devToolsController
    this.devMenu = coreContext.devMenu
    this.safeAreaInsetsProvider = coreContext.safeAreaInsetsProvider
    this.launchUri = coreContext.launchUri
    this.getDisplayMetrics = () => coreContext.getDisplayMetrics()
    this.getUIAbilityState = () => coreContext.getUIAbilityState()
    this.markReadiness = () => coreContext.markReadiness()
  }
}
