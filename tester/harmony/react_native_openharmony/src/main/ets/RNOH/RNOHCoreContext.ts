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
      async (options) => {
        const stopTracing = logger.clone("createAndRegisterRNInstance").startTracing()
        const result = await rnInstanceRegistry.createInstance(
          options,
        )
        stopTracing()
        return result
      },
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
      () => {
        rnInstanceRegistry.forEach(rnInstance => rnInstance.onBackPress())
      },
      () => {
      }, // markReadiness
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
    public createAndRegisterRNInstance: (options: RNInstanceOptions) => Promise<RNInstance>,
    public destroyAndUnregisterRNInstance: (rnInstance: RNInstance) => void,
    public logger: RNOHLogger,
    public getDisplayMetrics: () => DisplayMetrics,
    public getUIAbilityState: () => UIAbilityState,
    public dispatchBackPress: () => void,
    /**
     * @deprecated - This function shouldn't be in RNOHCoreContext because readiness is relative to a RNInstance and this context is shared across instances.
     * @depreciationDate 2024-04-08
     */
    public markReadiness: () => void,
    public uiAbilityContext: common.UIAbilityContext,
    public devToolsController: DevToolsController,
    public devMenu: DevMenu,
    public safeAreaInsetsProvider: SafeAreaInsetsProvider,
    public isDebugModeEnabled: boolean,
    public launchUri: string | undefined,
    public defaultBackPressHandler: () => void
  ) {
  }

  /**
   * invoked when the React application doesn't want to handle the device back press
   */
  public reportRNOHError(rnohError: RNOHError) {
    this.devToolsController.setLastError(rnohError)
    this.devToolsController.eventEmitter.emit("NEW_ERROR", rnohError)
  }

  public invokeDefaultBackPressHandler() {
    this.defaultBackPressHandler();
  }
}
