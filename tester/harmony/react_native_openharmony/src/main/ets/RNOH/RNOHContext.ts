import type { DescriptorRegistry } from './DescriptorRegistry';
import type { RNComponentCommandReceiver } from './RNComponentCommandHub';
import { RNInstance, RNInstanceImpl } from './RNInstance';
import type { ComponentManagerRegistry } from './ComponentManagerRegistry';
import { RNOHCoreContext } from './RNOHCoreContext';
import { HttpClient } from '../HttpClient/HttpClient';

/**
 * Provides dependencies and utility functions in context of RNInstance.
 */
export class RNOHContext extends RNOHCoreContext {
  static fromCoreContext(coreContext: RNOHCoreContext, rnInstance: RNInstance) {
    if (!(rnInstance instanceof RNInstanceImpl)) {
      throw new Error("RNInstance must extend RNInstanceImpl")
    }
    return new RNOHContext("0.72.5", rnInstance, coreContext);
  }

  /**
   * Check DescriptorRegistry documentation for more information.
   */
  public get descriptorRegistry(): DescriptorRegistry {
    return this.rnInstance.descriptorRegistry;
  }

  /**
   * Check RNComponentCommandReceiver documentation for more information.
   */
  public get componentCommandReceiver(): RNComponentCommandReceiver {
    return this.rnInstanceImpl.componentCommandHub;
  }

  /**
   * Check ComponentManagerRegistry documentation for more information.
   */
  public get componentManagerRegistry(): ComponentManagerRegistry {
    return this.rnInstance.componentManagerRegistry;
  }

  /**
   * Check RNInstance documentation for more information.
   */
  public get rnInstance(): RNInstance {
    return this.rnInstanceImpl
  }

  /**
   * Check RNInstance::httpClient documentation
   * @returns
   */
  public get httpClient(): HttpClient {
    return this.rnInstance.httpClient;
  }

  protected constructor(
    /**
     * Current React Native (not React Native OpenHarmony) version
     */
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
      coreContext._defaultBackPressHandler
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
