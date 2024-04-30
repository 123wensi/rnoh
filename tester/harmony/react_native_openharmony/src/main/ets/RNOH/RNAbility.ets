import UIAbility from '@ohos.app.ability.UIAbility';
import type { RNOHLogger } from './RNOHLogger';
import { StandardRNOHLogger } from './RNOHLogger';
import { RNOHCoreContext } from "./RNOHCoreContext"
import window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import { RNInstancesCoordinator } from "./RNInstancesCoordinator"
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import AbilityConfiguration from '@ohos.app.ability.Configuration';

/**
 Application developers are expected to extend this class. RNAbility aims to move much of the code to the RNOH side in order to improve stability.
 RNInstancesCoordinator can be used if more control is needed.
 */
export abstract class RNAbility extends UIAbility {
  // region RNAbility custom methods that can be safely overridden.
  protected createLogger(): RNOHLogger {
    return new StandardRNOHLogger((err) => {
      this.rnInstancesCoordinator?.getRNOHCoreContext().reportRNOHError(err)
    });
  }

  /**
   * Override this method if you want to create an RNInstance, and load bundle here, rather than in an ArkUI component.
   * RNAbility waits for the completion of this method before loading the content. This approach reduces a flash that
   * can occur after a splash screen is hidden, and a RN application is ready.
   */
  protected async onPrepareRNInstance(rnohCoreContext: RNOHCoreContext) {
  }

  /**
   * Override this method if you want to specify screen orientation, window dimensions etc.
   */
  protected async onWindowSetup(window: window.Window) {
    // NOTE: disabling full screen breaks the SafeAreaView component
    await window.setWindowLayoutFullScreen(true)
  }

  /**
   * Override this method if, for example, you want to customize loading behavior.
   */
  protected async onWindowStageSetup(windowStage: window.WindowStage) {
    windowStage.loadContent(this.getPagePath(), (err, data) => {
      const logger = this.logger.clone("onLoadContent")
      if (err.code) {
        logger.error("Failed to load the content", err.code)
        return;
      }
      logger.info("Succeeded in loading the content", JSON.stringify(data))
    });
  }

  protected abstract getPagePath(): string

  /**
   * Invoked when the React application doesn't handle the device back press.
   */
  protected defaultBackPressHandler() {
    this.context.terminateSelf()
  }

  // endregion


  private unregisterWindowListenerCallback = () => {
  }
  private rnInstancesCoordinator!: RNInstancesCoordinator
  private logger!: RNOHLogger

  onCreate(want: Want) {
    this.logger = this.createLogger().clone("RNAbility")
    this.rnInstancesCoordinator = RNInstancesCoordinator.create({
      logger: this.createLogger(),
      uiAbilityContext: this.context,
      defaultBackPressHandler: () => {
        this.defaultBackPressHandler()
      },
    }, {
      launchURI: want.uri, onGetPackagerClientConfig: (buildMode) => buildMode === "DEBUG" ? {
        host: "localhost",
        port: 8081
      } : undefined
    })
    AppStorage.setOrCreate('RNOHCoreContext', this.rnInstancesCoordinator.getRNOHCoreContext())
  }

  onDestroy() {
    this.rnInstancesCoordinator?.onDestroy()
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    const logger = this.logger.clone("onWindowStageCreate")
    const stopTracing = logger.startTracing()
    this.onAppSetup(windowStage).finally(stopTracing)
  }

  private async onAppSetup(windowStage: window.WindowStage) {
    const logger = this.logger.clone("onWindowSetup");
    const stopTracing = logger.startTracing()
    try {
      await this.onPrepareRNInstance(AppStorage.get<RNOHCoreContext>('RNOHCoreContext')!)
      const mainWindow = await windowStage.getMainWindow();
      const onWindowSizeChange = (windowSize: window.Size) => {
        this.rnInstancesCoordinator?.onWindowSizeChange(windowSize)
      }
      const onWindowStageChange = (windowStageEvent: window.WindowStageEventType) => {
        this.logger?.error("WindowStageEventType !!" + windowStageEvent)
        this.rnInstancesCoordinator?.onWindowStageChange(windowStageEvent)
      }
      mainWindow.on('windowSizeChange', onWindowSizeChange)
      windowStage.on('windowStageEvent', onWindowStageChange)
      this.unregisterWindowListenerCallback()
      this.unregisterWindowListenerCallback = () => {
        try {
          mainWindow.off("windowSizeChange", onWindowSizeChange)
        } catch {
          this.logger?.error("Error when trying to unsubscribe from window size changes")
        }
      }
      await this.onWindowSetup(mainWindow)
      await this.onWindowStageSetup(windowStage)
      this.rnInstancesCoordinator.onWindowSizeChange(mainWindow.getWindowProperties().windowRect)
    } catch (err) {
      logger.error("Failed to setup window", JSON.stringify(err))
    } finally {
      stopTracing()
    }
  }

  onMemoryLevel(level: AbilityConstant.MemoryLevel) {
    this.rnInstancesCoordinator.onMemoryLevel(level)
  }

  onConfigurationUpdate(config: AbilityConfiguration.Configuration) {
    this.rnInstancesCoordinator.onConfigurationUpdate(config)
  }

  onForeground() {
    this.rnInstancesCoordinator.onForeground();
  }

  onBackground() {
    this.rnInstancesCoordinator.onBackground();
  }
}
