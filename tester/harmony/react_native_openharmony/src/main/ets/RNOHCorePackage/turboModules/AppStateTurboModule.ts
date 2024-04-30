import type { TurboModuleContext } from "../../RNOH/TurboModule";
import { TurboModule } from "../../RNOH/TurboModule";
import type EnvironmentCallback from '@ohos.app.ability.EnvironmentCallback';
import { AbilityConstant } from '@kit.AbilityKit';

export class AppStateTurboModule extends TurboModule {
  public static readonly NAME = 'AppState';

  constructor(protected ctx: TurboModuleContext) {
    super(ctx);
    this.subscribeListeners();
  }

  private subscribeListeners() {
    this.ctx.rnInstance.subscribeToLifecycleEvents("FOREGROUND", () => {
      this.ctx.rnInstance.emitDeviceEvent("appStateDidChange", { app_state: this.getAppState() });
    })
    this.ctx.rnInstance.subscribeToLifecycleEvents("BACKGROUND", () => {
      this.ctx.rnInstance.emitDeviceEvent("appStateDidChange", { app_state: this.getAppState() });
    })
    this.ctx.rnInstance.subscribeToStageChangeEvents("APP_STATE_FOCUS", () => {
      this.ctx.rnInstance.emitDeviceEvent("appStateFocusChange",  true);
    })
    this.ctx.rnInstance.subscribeToStageChangeEvents("APP_STATE_BLUR", () => {
      this.ctx.rnInstance.emitDeviceEvent("appStateFocusChange", false);
    })
    let envCallback: EnvironmentCallback = {
      onConfigurationUpdated: () => {
      },

      onMemoryLevel: (level: AbilityConstant.MemoryLevel) => {
        if (level == AbilityConstant.MemoryLevel.MEMORY_LEVEL_CRITICAL) {
          this.ctx.rnInstance.emitDeviceEvent("memoryWarning", null);
        }
      }
    };

    const applicationContext = this.ctx.uiAbilityContext.getApplicationContext();
    applicationContext.on('environment', envCallback);
  }

  private getAppState() {
    const isActive = this.ctx.getUIAbilityState();

    return isActive === "FOREGROUND" ? 'active' : 'background';
  }

  getConstants() {
    return { initialAppState: this.getAppState() };
  }

  getCurrentAppState(success: (appState: { app_state: string }) => void, error: (error: Error) => void) {
    success({ app_state: this.getAppState() });
  };
}

