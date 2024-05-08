import { DevMenu } from './DevMenu'
import { DevToolsController } from './DevToolsController'
import { RNOHLogger } from './RNOHLogger'
import { SafeAreaInsetsProvider } from './SafeAreaInsetsProvider'
import { DisplayMetrics } from './types'
import { RNInstanceRegistry } from "./RNInstanceRegistry"
import { RNInstance, RNInstanceOptions, RNInstanceImpl } from "./RNInstance"
import common from '@ohos.app.ability.common'
import { RNOHError } from "./RNOHError"

export type UIAbilityState = "FOREGROUND" | "BACKGROUND"

/**
 * Provides dependencies and utilities shareable across RNInstances. Also includes methods for creating and destroying
 * RNInstances. For utilities or dependencies specific to an RNInstance, refer to RNOHContext.
 */
export class RNOHCoreContext {
  public static create(
    rnInstanceRegistry: RNInstanceRegistry,
    displayMetricsProvider: () => DisplayMetrics,
    uiAbilityStateProvider: () => UIAbilityState,
    launchUri: string | undefined,
    logger: RNOHLogger,
    uiAbilityContext: common.UIAbilityContext,
    isDebugModeEnabled: boolean,
    defaultBackPressHandler: () => void,
    devToolsController?: DevToolsController,
    devMenu?: DevMenu,
  ): RNOHCoreContext {
    const safeAreaInsetsProvider = new SafeAreaInsetsProvider(uiAbilityContext);
    const devToolsController_ = devToolsController ?? new DevToolsController(logger)
    return new RNOHCoreContext(
      // createAndRegisterRNInstance
      async (options) => {
        const stopTracing = logger.clone("createAndRegisterRNInstance").startTracing()
        const result = await rnInstanceRegistry.createInstance(
          options,
        )
        stopTracing()
        return result
      },
      // destroyAndUnregisterRNInstance
      (rnInstance) => {
        const stopTracing = logger.clone("destroyAndUnregisterRNInstance").startTracing()
        if (rnInstance instanceof RNInstanceImpl) {
          rnInstance.onDestroy()
        }
        rnInstanceRegistry.deleteInstance(rnInstance.getId())
        stopTracing()
      },
      logger,
      displayMetricsProvider,
      uiAbilityStateProvider,
      // dispatchBackPress
      () => {
        rnInstanceRegistry.forEach(rnInstance => rnInstance.onBackPress())
      },
      // markReadiness
      () => {
      },
      uiAbilityContext,
      devToolsController_,
      devMenu ?? new DevMenu(devToolsController_, uiAbilityContext, logger),
      safeAreaInsetsProvider,
      isDebugModeEnabled,
      launchUri,
      defaultBackPressHandler
    )
  }

  protected constructor(
    /**
     * Creates RNInstance and registers it in RNOH's internal registries.
     */
    public createAndRegisterRNInstance: (options: RNInstanceOptions) => Promise<RNInstance>,
    /**
     * Destroys RNInstance and unregisters it from RNOH's internal registries.
     */
    public destroyAndUnregisterRNInstance: (rnInstance: RNInstance) => void,
    public logger: RNOHLogger,
    /**
     * Returns current display metrics.
     */
    public getDisplayMetrics: () => DisplayMetrics,
    /**
     * Returns the current state of the application.
     */
    public getUIAbilityState: () => UIAbilityState,
    /**
     * Expected to be called from the root ArkUI component from method onBackPress. Currently, this is the only way to react on back presses.
     */
    public dispatchBackPress: () => void,
    /**
     * @deprecated - This function shouldn't be in RNOHCoreContext because readiness is relative to a RNInstance and this context is shared across instances.
     * @depreciationDate 2024-04-08
     */
    public markReadiness: () => void,
    /**
     * ArkUI's UIAbility context.
     */
    public uiAbilityContext: common.UIAbilityContext,
    public devToolsController: DevToolsController,
    public devMenu: DevMenu,
    public safeAreaInsetsProvider: SafeAreaInsetsProvider,
    /**
     * The result of checking if React Native was compiled with DEBUG flag.
     * */
    public isDebugModeEnabled: boolean,
    /**
     * ArkUI::Want::uri.
     */
    public launchUri: string | undefined,

    public _defaultBackPressHandler: () => void
  ) {
  }

  /**
   *
   */
  public reportRNOHError(rnohError: RNOHError) {
    this.devToolsController.setLastError(rnohError)
    this.devToolsController.eventEmitter.emit("NEW_ERROR", rnohError)
  }

  /**
   * Invoked by React Native when the React application doesn't want to handle the device back press. This method may be relocated in the future.
   */
  public invokeDefaultBackPressHandler() {
    this._defaultBackPressHandler();
  }
}
