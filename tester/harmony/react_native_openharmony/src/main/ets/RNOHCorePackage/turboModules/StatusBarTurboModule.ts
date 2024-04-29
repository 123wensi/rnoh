import type { TurboModuleContext } from "../../RNOH/TurboModule";
import { TurboModule } from "../../RNOH/TurboModule";
import window from '@ohos.window';

import { convertColorValueToHex, EventEmitter } from '../../RNOH/ts';


type StatusBarConstants = {
  DEFAULT_BACKGROUND_COLOR: string,
  HEIGHT: number,
}

declare function px2vp(px: number): number;

type StatusBarEventNameByListenerArgs = {
  SYSTEM_BAR_VISIBILITY_CHANGE: [{ hidden: boolean }]
}

// interpolate color could be removed after API 12 enableStatusBarAnimation property starts working
function interpolateColor(color1: string, color2: string, steps: number): string[] {
  let colorArray: string[] = [];

  // Convert hex to RGB first
  let colorStartRGB = hexToRGB(color1);
  let colorStopRGB = hexToRGB(color2);

  let rStep = (colorStopRGB[0] - colorStartRGB[0]) / (steps - 1);
  let gStep = (colorStopRGB[1] - colorStartRGB[1]) / (steps - 1);
  let bStep = (colorStopRGB[2] - colorStartRGB[2]) / (steps - 1);
  let aStep = (colorStopRGB[3] - colorStartRGB[3]) / (steps - 1);

  for (let i = 0; i < steps; i++) {
      // Calculate the interpolated color value
      let rVal = colorStartRGB[0] + (rStep * i);
      let gVal = colorStartRGB[1] + (gStep * i);
      let bVal = colorStartRGB[2] + (bStep * i);
      let aVal = colorStartRGB[3] + (aStep * i);

      // Ensure values are within the valid range
      rVal = Math.min(255, Math.max(0, rVal));
      gVal = Math.min(255, Math.max(0, gVal));
      bVal = Math.min(255, Math.max(0, bVal));
      aVal = Math.min(255, Math.max(0, aVal));

      // Convert RGB to hex color and add it to the array
      colorArray.push(rgbToHex(Math.round(rVal), Math.round(gVal), Math.round(bVal), Math.round(aVal))) ;
  }

  return colorArray;
}

function hexToRGB(hex: string): number[] {
  return [
      parseInt(hex.slice(3, 5), 16),
      parseInt(hex.slice(5, 7), 16),
      parseInt(hex.slice(7, 9), 16),
      parseInt(hex.slice(1, 3), 16),
  ];
}

function rgbToHex(r: number, g: number, b: number, a: number): string {
  let rHex = r.toString(16).padStart(2, '0');
  let gHex = g.toString(16).padStart(2, '0');
  let bHex = b.toString(16).padStart(2, '0');
  let aHex = a.toString(16).padStart(2, '0');

  return `#${aHex}${rHex}${gHex}${bHex}`;
}

export class StatusBarTurboModule extends TurboModule {
  public static readonly NAME = 'StatusBarManager';

  private constants?: StatusBarConstants = null;
  private eventEmitter = new EventEmitter<StatusBarEventNameByListenerArgs>()
  private _isStatusBarHidden = false;
  private _currentColor = "#66000000";

  constructor(protected ctx: TurboModuleContext) {
    super(ctx);
    this.setConstants();
  }

  private async setConstants() {
    const windowInstance = await window.getLastWindow(this.ctx.uiAbilityContext);
    try {
      const statusBarHeight = windowInstance.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM).topRect.height;

      const scaledStatusBarHeight = px2vp(statusBarHeight);
      this.constants = {
        DEFAULT_BACKGROUND_COLOR: '#00000066',
        HEIGHT: scaledStatusBarHeight,
      }
    } catch (exception) {
      this.ctx.logger.error('Failed to obtain the avoid area  (currentHeight). Cause:' + JSON.stringify(exception));
    }
  }

  getConstants(): StatusBarConstants {
    return this.constants ?? {
      DEFAULT_BACKGROUND_COLOR: "#00000066",
      HEIGHT: 0,
    };
  }

  async setTranslucent(translucent: boolean) {
    this.ctx.logger.error('Not supported. StatusBar is translucent by default. Use SafeAreaView.');
  }

  async setStyle(style: string, animated: boolean) {
    let systemBarProperties = {
      statusBarContentColor: '#E5FFFFFF',
      enableStatusBarAnimation: animated, // added in API 12, doesn't seem to work
    };
    if (style === 'dark-content') {
      systemBarProperties.statusBarContentColor = '#000000';
    }
    try {
      const windowInstance = await window.getLastWindow(this.ctx.uiAbilityContext);
      windowInstance.setWindowSystemBarProperties(systemBarProperties);
      this.ctx.logger.info('Succeeded in setting the status bar content style.');
    }
    catch (exception) {
      this.ctx.logger.error('Failed to set the status bar content style. Cause:' + JSON.stringify(exception));
    }
  }

  async setColor(color: number, animated: boolean) {
    const colorString = convertColorValueToHex(color);
    let systemBarProperties = {
      statusBarColor: colorString,
      enableStatusBarAnimation: animated, // added in API 12, doesn't seem to work
    };
    let intervalId;
    try {
      const windowInstance = await window.getLastWindow(this.ctx.uiAbilityContext);
      if (!animated) {
        windowInstance.setWindowSystemBarProperties(systemBarProperties);
      } else {
        // manually create color animation, can be removed after API 12 property enableStatusBarAnimation works
        let colors = interpolateColor(this._currentColor, colorString, 20);
        let i = 0;
        intervalId = setInterval(() => {
          windowInstance.setWindowSystemBarProperties({ statusBarColor: colors[i]});
          i++;
          if (i >= colors.length) {
            clearInterval(intervalId);
          }
        }, 16);
      }
      this._currentColor = colorString;
      this.ctx.logger.info('Succeeded in setting the status bar background color. Color: ' + colorString);
    }
    catch (exception) {
      clearInterval(intervalId);
      this.ctx.logger.error('Failed to set the status bar background color. Cause:' + JSON.stringify(exception));
    }
  }

  async setHidden(hidden: boolean, withAnimation?: string) {
    const skipAnimation = !withAnimation || withAnimation === 'none'
    try {
      const windowInstance = await window.getLastWindow(this.ctx.uiAbilityContext);
      await windowInstance.setSpecificSystemBarEnabled('status', !hidden, !skipAnimation);
      this._isStatusBarHidden = hidden
      this.eventEmitter.emit("SYSTEM_BAR_VISIBILITY_CHANGE", { hidden })
      this.ctx.logger.info('Succeeded in setting the system bar to be hidden.');
    } catch (exception) {
      this.ctx.logger.error('Failed to set the status bar to be hidden. Cause:' + JSON.stringify(exception));
    }
  }

  public subscribe<TEventType extends keyof StatusBarEventNameByListenerArgs>(eventType: TEventType, listener: (...args: StatusBarEventNameByListenerArgs[TEventType]) => void,
  ) {
    return this.eventEmitter.subscribe(eventType, listener)
  }

  public isStatusBarHidden() {
    return this._isStatusBarHidden
  }
}