import type { TurboModuleProvider } from "./TurboModuleProvider";
import type { Mutation } from "./Mutation";
import type { Tag } from "./DescriptorBase";
import type { AttributedString, ParagraphAttributes, LayoutConstrains } from "./TextLayoutManager";
import { measureParagraph } from "./TextLayoutManager"
import type { DisplayMode } from './CppBridgeUtils'
import { RNOHLogger } from "./RNOHLogger"
import type { InspectorInstance } from './types'
import { FatalRNOHError } from "./RNOHError"
import type { FrameNodeFactory } from "./RNInstance"
import type { ArkTSBridgeHandler } from './ArkTSBridgeHandler';

export type CppFeatureFlag = "ENABLE_NDK_TEXT_MEASURING" | "C_API_ARCH"

export class NapiBridge {
  private logger: RNOHLogger

  constructor(private libRNOHApp: any, logger: RNOHLogger) {
    this.logger = logger.clone("NapiBridge")
  }

  onInit(shouldCleanUpRNInstances: boolean): { isDebugModeEnabled: boolean } {
    if (!this.libRNOHApp) {
      const err = new FatalRNOHError({
        whatHappened: "Couldn't create bindings between ETS and CPP. libRNOHApp is undefined.",
        howCanItBeFixed: []
      })
      this.logger.fatal(err)
      throw err
    }
    return this.libRNOHApp?.onInit(shouldCleanUpRNInstances)
  }

  getNextRNInstanceId(): number {
    return this.libRNOHApp?.getNextRNInstanceId()
  }

  createReactNativeInstance(instanceId: number,
                            turboModuleProvider: TurboModuleProvider,
                            frameNodeFactory: FrameNodeFactory,
                            mutationsListener: (mutations: Mutation[]) => void,
                            componentCommandsListener: (tag: Tag,
                                                        commandName: string,
                                                        args: unknown) => void,
                            onCppMessage: (type: string, payload: any) => void,
                            shouldEnableDebugger: boolean,
                            shouldEnableBackgroundExecutor: boolean,
                            cppFeatureFlags: CppFeatureFlag[]
  ) {
    const cppFeatureFlagStatusByName = cppFeatureFlags.reduce((acc, cppFeatureFlag) => {
      acc[cppFeatureFlag] = true
      return acc
    }, {} as Record<CppFeatureFlag, boolean>)
    this.libRNOHApp?.createReactNativeInstance(
      instanceId,
      turboModuleProvider,
      mutationsListener,
      componentCommandsListener,
      onCppMessage,
      (attributedString: AttributedString, paragraphAttributes: ParagraphAttributes, layoutConstraints: LayoutConstrains) => {
        try {
          const stopTracing = this.logger.clone("measureParagraph").startTracing()
          const result = measureParagraph(attributedString, paragraphAttributes, layoutConstraints)
          stopTracing()
          return result
        } catch (err) {
          console.error(err)
          throw err
        }
      },
      shouldEnableDebugger,
      shouldEnableBackgroundExecutor,
      cppFeatureFlagStatusByName,
      frameNodeFactory,
    );
  }

  destroyReactNativeInstance(instanceId: number) {
    this.libRNOHApp?.destroyReactNativeInstance(instanceId)
  }

  emitComponentEvent(instanceId: number, tag: Tag, eventEmitRequestHandlerName: string, payload: any) {
    this.libRNOHApp?.emitComponentEvent(instanceId, tag, eventEmitRequestHandlerName, payload);
  }

  loadScript(instanceId: number, bundle: ArrayBuffer, sourceURL: string): Promise<void> {
    return new Promise((resolve, reject) => {
      this.libRNOHApp?.loadScript(instanceId, bundle, sourceURL, (errorMsg: string) => {
        errorMsg ? reject(new Error(errorMsg)) : resolve()
      });
    })
  }

  startSurface(
    instanceId: number,
    surfaceTag: number,
    initialSurfaceWidth: number,
    initialSurfaceHeight: number,
    surfaceOffsetX: number,
    surfaceOffsetY: number,
    pixelRatio: number,
    initialProps: any) {
    this.libRNOHApp?.startSurface(
      instanceId,
      surfaceTag,
      initialSurfaceWidth,
      initialSurfaceHeight,
      surfaceOffsetX,
      surfaceOffsetY,
      pixelRatio,
      initialProps,
    );
  }

  updateSurfaceConstraints(
    instanceId: number,
    surfaceTag: number,
    surfaceWidth: number,
    surfaceHeight: number,
    surfaceOffsetX: number,
    surfaceOffsetY: number,
    pixelRatio: number,
  ) {
    this.libRNOHApp?.updateSurfaceConstraints(
      instanceId,
      surfaceTag,
      surfaceWidth,
      surfaceHeight,
      surfaceOffsetX,
      surfaceOffsetY,
      pixelRatio
    );
  }

  createSurface(
    instanceId: number,
    surfaceTag: number,
    appKey: string,
  ) {
    this.libRNOHApp?.createSurface(
      instanceId,
      surfaceTag,
      appKey,
    );
  }

  setSurfaceProps(
    instanceId: number,
    surfaceTag: number,
    props: Record<string, any>,
  ) {
    this.libRNOHApp?.setSurfaceProps(
      instanceId,
      surfaceTag,
      props,
    )
  }

  async stopSurface(
    instanceId: number,
    surfaceTag: number,
  ) {
    this.libRNOHApp?.stopSurface(
      instanceId,
      surfaceTag
    );
  }

  async destroySurface(
    instanceId: number,
    surfaceTag: number,
  ) {
    this.libRNOHApp?.destroySurface(
      instanceId,
      surfaceTag
    );
  }

  setSurfaceDisplayMode(instanceId: number, surfaceTag: Tag, displayMode: DisplayMode): void {
    this.libRNOHApp?.setSurfaceDisplayMode(instanceId, surfaceTag, displayMode);
  }

  callRNFunction(instanceId: number, moduleName: string, functionName: string, args: unknown[]): void {
    this.libRNOHApp?.callRNFunction(instanceId, moduleName, functionName, args);
  }

  onMemoryLevel(level: number): void {
    this.libRNOHApp?.onMemoryLevel(level)
  }

  updateState(instanceId: number, componentName: string, tag: Tag, state: unknown): void {
    this.libRNOHApp?.updateState(instanceId, componentName, tag, state)
  }

  getInspectorWrapper(): InspectorInstance {
    return this.libRNOHApp?.getInspectorWrapper();
  }

  initializeArkTSBridge(bridge: ArkTSBridgeHandler) {
    this.libRNOHApp?.initializeArkTSBridge(bridge);
  }
}
