import uri from '@ohos.uri';
import call from '@ohos.telephony.call';
import type { TurboModuleContext } from "../../RNOH/TurboModule";
import { TurboModule } from "../../RNOH/TurboModule";
import type Want from '@ohos.app.ability.Want';
import bundleManager from '@ohos.bundle.bundleManager';

export class LinkingManagerTurboModule extends TurboModule {
  public static readonly NAME = 'LinkingManager' as const;

  private static readonly CUSTOM_HANDLED_SCHEMES = ['tel', 'sms']; // those exceptions are manually handled in openURL because OH doesn't support opening urls with those schemes

  private initialUrl: string | undefined;

  constructor(ctx: TurboModuleContext) {
    super(ctx);
    this.initialUrl = ctx.launchUri;
  }

  async getInitialURL(): Promise<string | undefined> {
    return this.initialUrl ? this.initialUrl : null;
  }

  async canOpenURL(urlString: string): Promise<boolean> {
    if (
      LinkingManagerTurboModule.CUSTOM_HANDLED_SCHEMES.includes(
        new uri.URI(urlString).scheme,
      )
    ) {
      return true;
    }
    try {
      // this will return true for custom schemes only if you enable quering those schemes in the module.json5 file (in the app which tries to make a query)
      return bundleManager.canOpenLink(urlString);
    } catch (e) {
      this.ctx.logger.error(e);
      return false;
    }
  }

  async openURL(urlString: string): Promise<void> {
    const uriObject = new uri.URI(urlString);
    switch (uriObject.scheme) {
      case "tel": {
        await call.makeCall(uriObject.ssp);
        return;
      }
      case "http":
      case "https": {
        const want = this.createBrowserWant(urlString);
        await this.ctx.uiAbilityContext.startAbility(want);
        return;
      }
      case "sms": {
        const want = this.createSMSWant(uriObject);
        await this.ctx.uiAbilityContext.startAbility(want);
        return;
      }
      default: {
        try {
          await this.ctx.uiAbilityContext.startAbility({ uri: urlString })
        } catch (e) {
          throw new Error(`URL scheme ${uriObject.scheme} is not supported`);
        }
      }
    }
  }

  openSettings(): Promise<void> {
    return this.ctx.uiAbilityContext.startAbility({
      bundleName: 'com.huawei.hmos.settings',
      abilityName:'com.huawei.hmos.settings.MainAbility',
      uri: 'application_info_entry',
      parameters: {
        settingsParamBundleName: this.ctx.uiAbilityContext.abilityInfo.bundleName
      }
    });
  }

  private createBrowserWant(urlString: string): Want {
    return {
      "action": "ohos.want.action.viewData",
      "entities": ["entity.system.browsable"],
      "uri": urlString,
    };
  }

  private createSMSWant(uri: uri.URI): Want {
    const telephone = uri.ssp;
    const parameters = telephone ? {
      contactObjects: JSON.stringify([{ telephone }]),
      pageFlag: 'conversation'
    } : undefined;
    // NOTE: the default sms app doesn't handle the `sendSms` action...
    // see https://gitee.com/openharmony/applications_mms/blob/master/entry/src/main/module.json5
    return {
      // "action": "ohos.want.action.sendSms",
      bundleName: "com.ohos.mms",
      abilityName: "com.ohos.mms.MainAbility",
      parameters
    }
  }
}
