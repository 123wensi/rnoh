import type common from '@ohos.app.ability.common'
import window from '@ohos.window';
import { RNOHLogger } from "./RNOHLogger"
import { NapiBridge } from './NapiBridge';
import { RNInstanceRegistry } from "./RNInstanceRegistry"
import { RNOHContext } from "./RNOHContext"
import { RNOHCoreContext } from "./RNOHCoreContext"
import { DisplayMetricsManager } from './DisplayMetricsManager';
import { JSPackagerClient } from "./JSPackagerClient"
import { DevToolsController } from "./DevToolsController"
import { DevMenu } from "./DevMenu"
import { JSPackagerClientConfig } from "./JSPackagerClient"
import { RNOHError } from "./RNOHError"
import AbilityConfiguration from '@ohos.app.ability.Configuration';
import { HttpClientProvider, DefaultHttpClientProvider } from './HttpClientProvider';

/**
 * This interface allows providing dependencies in any order.
 * This is useful when some dependency is deprecated, made optional and later removed since such operation doesn't
 * create a breaking change.
 */
export interface RNInstancesCoordinatorDependencies {
  logger: RNOHLogger,
  uiAbilityContext: common.UIAbilityContext,

  /**
   * invoked when the React application doesn't want to handle the device back press
   */
  defaultBackPressHandler: () => void
}

export type BuildMode = "DEBUG" | "RELEASE"

export interface RNInstancesCoordinatorOptions {
  launchURI?: string
  onGetPackagerClientConfig?: (buildMode: BuildMode) => JSPackagerClientConfig | undefined
  httpClientProvider?: HttpClientProvider
}

const RNOH_BANNER = '\n\n\n' +
  '██████╗ ███╗   ██╗ ██████╗ ██╗  ██╗' + '\n' +
  '██╔══██╗████╗  ██║██╔═══██╗██║  ██║' + '\n' +
  '██████╔╝██╔██╗ ██║██║   ██║███████║' + '\n' +
  '██╔══██╗██║╚██╗██║██║   ██║██╔══██║' + '\n' +
  '██║  ██║██║ ╚████║╚██████╔╝██║  ██║' + '\n' +
  '╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝' + '\n\n'

type AppState = "FOREGROUND" | "BACKGROUND"

/**
 * Bootstraps React Native and forwards device events to RNInstances. This class is created for UIAbility's usage.
 * Use this class if RNAbility is too limiting for your use case, and adapting code to breaking changes is not a big deal in your project.
 */
export class RNInstancesCoordinator {
  private static isInitialized = false

  static create(dependencies: RNInstancesCoordinatorDependencies, options?: RNInstancesCoordinatorOptions): RNInstancesCoordinator {
    if (RNInstancesCoordinator.isInitialized) {
      dependencies.logger.fatal(new RNOHError({
        whatHappened: "RNInstancesCoordinator was already created",
        howCanItBeFixed: ["Change your code to call RNInstancesCoordinator.create only once"]
      }))
    }
    RNInstancesCoordinator.isInitialized = true

    dependencies.logger.info(RNOH_BANNER)
    const napiBridge = new NapiBridge(dependencies.logger);
    const isDebugModeEnabled = napiBridge.onInit(true).isDebugModeEnabled;
    const displayMetricsManager = new DisplayMetricsManager(dependencies.logger);
    const devToolsController = new DevToolsController(dependencies.logger);
    const devMenu = new DevMenu(devToolsController, dependencies.uiAbilityContext, dependencies.logger);
    const jsPackagerClient = new JSPackagerClient(
      dependencies.logger,
      devToolsController,
      devMenu
    )
    const jsPackagerClientConfig = options?.onGetPackagerClientConfig?.(isDebugModeEnabled ? "DEBUG" : "RELEASE")
    if (jsPackagerClientConfig) {
      jsPackagerClient.connectToMetroMessages(jsPackagerClientConfig)
    }
    napiBridge.initializeArkTSBridge({
      getDisplayMetrics: () => displayMetricsManager.getDisplayMetrics(),
      handleError: (err) => {
        devToolsController.setLastError(err)
        devToolsController.eventEmitter.emit("NEW_ERROR", err)
      }
    });
    return new RNInstancesCoordinator(napiBridge,
      dependencies.logger,
      displayMetricsManager,
      isDebugModeEnabled,
      dependencies.uiAbilityContext,
      devToolsController,
      devMenu,
      jsPackagerClient,
      dependencies.defaultBackPressHandler,
      options);
  }

  private rawLogger: RNOHLogger;
  private logger: RNOHLogger;
  private rnInstanceRegistry: RNInstanceRegistry
  private rnohCoreContext: RNOHCoreContext;
  private napiBridge: NapiBridge;
  private displayMetricsManager: DisplayMetricsManager;
  private isDebugModeEnabled: boolean
  private uiAbilityContext: common.UIAbilityContext
  private devToolsController: DevToolsController
  private devMenu: DevMenu
  private jsPackagerClient: JSPackagerClient
  private options: RNInstancesCoordinatorOptions | undefined
  private appState: AppState = "FOREGROUND"
  private defaultBackPressHandler: () => void
  private constructor(napiBridge: NapiBridge,
                      logger: RNOHLogger,
                      displayMetricsManager: DisplayMetricsManager,
                      isDebugModeEnabled: boolean,
                      uiAbilityContext: common.UIAbilityContext,
                      devToolsController: DevToolsController,
                      devMenu: DevMenu,
                      jsPackagerClient: JSPackagerClient,
                      defaultBackPressHandler: () => void,
                      options?: RNInstancesCoordinatorOptions) {
    this.logger = logger.clone("RNInstancesCoordinator")
    this.rawLogger = logger
    this.napiBridge = napiBridge
    this.displayMetricsManager = displayMetricsManager;
    this.isDebugModeEnabled = isDebugModeEnabled;
    this.uiAbilityContext = uiAbilityContext;
    this.devToolsController = devToolsController;
    this.devMenu = devMenu;
    this.options = options;
    this.jsPackagerClient = jsPackagerClient;
    this.defaultBackPressHandler = defaultBackPressHandler;
    const httpClientProvider = options?.httpClientProvider ?? new DefaultHttpClientProvider()
    this.rnInstanceRegistry = new RNInstanceRegistry(
      this.rawLogger,
      napiBridge,
      this.devToolsController,
      (rnInstance) => RNOHContext.fromCoreContext(this.rnohCoreContext, rnInstance),
      httpClientProvider,
    )
    this.rnohCoreContext = RNOHCoreContext.create(
      this.rnInstanceRegistry,
      () => this.displayMetricsManager.getDisplayMetrics(),
      () => this.appState,
      this.options?.launchURI,
      this.rawLogger,
      this.uiAbilityContext,
      this.isDebugModeEnabled,
      this.defaultBackPressHandler,
      this.devToolsController,
      this.devMenu,
    );
  }

  public onDestroy() {
    const stopTracing = this.logger.clone("onDestroy").startTracing()
    this.jsPackagerClient.onDestroy()
    this.rnInstanceRegistry.forEach(instance => instance.onDestroy())
    stopTracing()
  }

  public getBuildMode(): BuildMode {
    return this.isDebugModeEnabled ? "DEBUG" : "RELEASE"
  }

  public getRNOHCoreContext() {
    return this.rnohCoreContext
  }

  public onForeground() {
    this.setAppState("FOREGROUND")
    this.rnInstanceRegistry.forEach(instance => instance.onForeground())
  }

  private setAppState(appState: AppState) {
    this.appState = appState
  }

  public onBackground() {
    this.setAppState("BACKGROUND")
    this.rnInstanceRegistry.forEach(instance => instance.onBackground())
  }


  public onWindowSizeChange(windowSize: window.Size) {
    this.displayMetricsManager.updateWindowSize(windowSize);
    this.rnInstanceRegistry.forEach((rnInstance) => rnInstance.onWindowSizeChange(windowSize))
  }

  public onConfigurationUpdate(systemConfiguration: AbilityConfiguration.Configuration) {
    this.displayMetricsManager?.updateDisplayMetrics()
    this.rnInstanceRegistry?.forEach((rnInstance) => rnInstance.onConfigurationUpdate(systemConfiguration))
  }

  public onMemoryLevel(memoryLevel: number) {
    const MEMORY_LEVEL_NAMES = ["MEMORY_LEVEL_MODERATE", "MEMORY_LEVEL_LOW", "MEMORY_LEVEL_CRITICAL"]
    this.logger.debug("Received memory level event: " + MEMORY_LEVEL_NAMES[memoryLevel])
    this.napiBridge.onMemoryLevel(memoryLevel);
  }
}