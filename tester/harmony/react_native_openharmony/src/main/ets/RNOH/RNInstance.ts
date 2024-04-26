import type UIAbility from '@ohos.app.ability.UIAbility'
import { CommandDispatcher, RNComponentCommandHub } from './RNComponentCommandHub'
import { DescriptorRegistry, DescriptorWrapperFactory } from './DescriptorRegistry'
import { ComponentManagerRegistry } from './ComponentManagerRegistry'
import { SurfaceHandle } from './SurfaceHandle'
import { TurboModuleProvider } from './TurboModuleProvider'
import { EventEmitter } from './EventEmitter'
import type { RNOHLogger } from './RNOHLogger'
import type { NapiBridge, CppFeatureFlag } from './NapiBridge'
import type { RNOHContext } from './RNOHContext'
import { RNOHCorePackage } from '../RNOHCorePackage/ts'
import type { JSBundleProvider } from './JSBundleProvider'
import { JSBundleProviderError } from './JSBundleProvider'
import type { Tag } from './DescriptorBase'
import type { RNPackage, RNPackageContext } from './RNPackage'
import type { TurboModule } from './TurboModule'
import { ResponderLockDispatcher } from './ResponderLockDispatcher'
import { DevToolsController } from './DevToolsController'
import { RNOHError } from './RNOHError'
import window from '@ohos.window'
import { DevServerHelper } from './DevServerHelper';
import { HttpClient } from '../HttpClient/HttpClient'
import type { HttpClientProvider } from './HttpClientProvider'

export type SurfaceContext = {
  width: number
  height: number
  surfaceOffsetX: number
  surfaceOffsetY: number
  pixelRatio: number
}

export enum LifecycleState {
  BEFORE_CREATE,
  PAUSED,
  READY,
}

export type LifecycleEventArgsByEventName = {
  CONFIGURATION_UPDATE: Parameters<UIAbility["onConfigurationUpdate"]>
  FOREGROUND: [];
  BACKGROUND: [];
  JS_BUNDLE_EXECUTION_FINISH: [{
    jsBundleUrl: string,
    appKeys: string[]
  }];
  RELOAD: [{ reason: string | undefined }];
  WINDOW_SIZE_CHANGE: [windowSize: window.Size];
}


export type BundleExecutionStatus = "RUNNING" | "DONE"

const rootDescriptor = {
  isDynamicBinder: false,
  type: 'RootView',
  tag: 1,
  childrenTags: [],
  rawProps: {},
  props: {},
  state: {},
  layoutMetrics: {
    frame: {
      origin: {
        x: 0,
        y: 0,
      },
      size: {
        width: 0,
        height: 0,
      }
    }
  }
}

const DEFAULT_ASSETS_DEST: string = "assets/"; // assets destination path "assets/subpath/"

type FeatureFlagName = "ENABLE_RN_INSTANCE_CLEAN_UP" | "NDK_TEXT_MEASUREMENTS" | "IMAGE_LOADER" | "C_API_ARCH"

export interface RNInstance {
  descriptorRegistry: DescriptorRegistry;
  cppEventEmitter: EventEmitter<Record<string, unknown[]>>
  /**
   * @deprecated Use RNOHContext::componentCommandReceiver
   */
  commandDispatcher: CommandDispatcher;
  componentManagerRegistry: ComponentManagerRegistry;

  getLifecycleState(): LifecycleState;

  subscribeToLifecycleEvents: <TEventName extends keyof LifecycleEventArgsByEventName>(
    eventName: TEventName,
    listener: (...args: LifecycleEventArgsByEventName[TEventName]) => void
  ) => () => void;

  callRNFunction(moduleName: string, functionName: string, args: unknown[]): void;

  emitDeviceEvent(eventName: string, payload: any): void;

  emitComponentEvent(tag: Tag, eventName: string, payload: any): void;

  getBundleExecutionStatus(bundleURL: string): BundleExecutionStatus | undefined

  enableFeatureFlag(featureFlagName: FeatureFlagName): void

  isFeatureFlagEnabled(featureFlagName: FeatureFlagName): boolean

  runJSBundle(jsBundleProvider: JSBundleProvider): Promise<void>;

  getTurboModule<T extends TurboModule>(name: string): T;

  createSurface(appKey: string): SurfaceHandle;

  updateState(componentName: string, tag: Tag, state: unknown): void;

  getId(): number;

  bindComponentNameToDescriptorType(componentName: string, descriptorType: string);

  getComponentNameFromDescriptorType(descriptorType: string): string

  /**
   * Blocks gestures in targetComponent and its ancestors. Used by react-native-gesture-handler when panning in
   * RNScrollView or other scrollable components.
   * @returns a function that cancels this this effect
   */
  blockComponentsGestures(targetComponentTag: Tag): (() => void)

  getInitialBundleUrl(): string | undefined

  getArchitecture(): "ARK_TS" | "C_API"

  get httpClient(): HttpClient,

  getAssetsDest(): string

  postMessageToCpp(name: string, payload: any): void
}

export type RNInstanceOptions = {
  createRNPackages: (ctx: RNPackageContext) => RNPackage[],
  enableDebugger?: boolean,
  enableBackgroundExecutor?: boolean,
  enableNDKTextMeasuring?: boolean,
  enableImageLoader?: boolean,
  enableCAPIArchitecture?: boolean,
  assetsDest?: string,
}

/**
 * Used in the C-API architecture
 */
export interface FrameNodeFactory {
  create(tag: Tag, componentName: string);
}


export class RNInstanceImpl implements RNInstance {
  private turboModuleProvider: TurboModuleProvider
  private surfaceCounter = 0;
  private lifecycleState: LifecycleState = LifecycleState.BEFORE_CREATE
  private bundleExecutionStatusByBundleURL: Map<string, BundleExecutionStatus> = new Map()
  public descriptorRegistry: DescriptorRegistry;
  public componentCommandHub: RNComponentCommandHub;
  public componentManagerRegistry: ComponentManagerRegistry;
  public lifecycleEventEmitter = new EventEmitter<LifecycleEventArgsByEventName>()
  public cppEventEmitter = new EventEmitter<Record<string, unknown[]>>()
  public httpClient: HttpClient;
  private componentNameByDescriptorType = new Map<string, string>()
  private logger: RNOHLogger
  private surfaceHandles: Set<SurfaceHandle> = new Set()
  private responderLockDispatcher: ResponderLockDispatcher
  private isFeatureFlagEnabledByName = new Map<FeatureFlagName, boolean>()
  private initialBundleUrl: string | undefined = undefined

  /**
   * @deprecated
   */
  public get commandDispatcher() {
    return this.componentCommandHub
  }

  constructor(
    private id: number,
    private injectedLogger: RNOHLogger,
    private frameNodeFactory: FrameNodeFactory | null,
    private napiBridge: NapiBridge,
    private defaultProps: Record<string, any>,
    private devToolsController: DevToolsController,
    private createRNOHContext: (rnInstance: RNInstance) => RNOHContext,
    private shouldEnableDebugger: boolean,
    private shouldEnableBackgroundExecutor: boolean,
    private shouldUseNDKToMeasureText: boolean,
    private shouldUseImageLoader: boolean,
    private shouldUseCApiArchitecture: boolean,
    private assetsDest: string,
    httpClientProvider: HttpClientProvider,
  ) {
    this.httpClient = httpClientProvider.getInstance(this)
    this.logger = injectedLogger.clone("RNInstance")
    if (this.shouldUseNDKToMeasureText) {
      this.enableFeatureFlag("NDK_TEXT_MEASUREMENTS")
    }
    if (this.shouldUseImageLoader) {
      this.enableFeatureFlag("IMAGE_LOADER")
    }
    if (this.shouldUseCApiArchitecture) {
      this.enableFeatureFlag("C_API_ARCH")
    }
    this.onCreate()
  }

  public getArchitecture() {
    return this.shouldUseCApiArchitecture ? "C_API" : "ARK_TS"
  }

  public getAssetsDest(): string {
    return this.assetsDest ?? DEFAULT_ASSETS_DEST
  }

  public onCreate() {
    this.componentManagerRegistry = new ComponentManagerRegistry(this.injectedLogger);
    this.componentCommandHub = new RNComponentCommandHub();
    this.responderLockDispatcher = new ResponderLockDispatcher(this.componentManagerRegistry, this.componentCommandHub, this.injectedLogger)
    this.subscribeToDevTools();
  }

  public async onDestroy() {
    const stopTracing = this.logger.clone("onDestroy").startTracing()
    for (const surfaceHandle of this.surfaceHandles) {
      if (surfaceHandle.isRunning()) {
        this.logger.warn("Destroying instance with running surface with tag: " + surfaceHandle.getTag());
        surfaceHandle.stop();
      }
      surfaceHandle.destroy()
    }
    if (this.isFeatureFlagEnabled("ENABLE_RN_INSTANCE_CLEAN_UP")) {
      this.napiBridge.destroyReactNativeInstance(this.id)
    }
    this.turboModuleProvider.onDestroy()
    stopTracing()
  }

  public getId(): number {
    return this.id;
  }

  enableFeatureFlag(featureFlagName: FeatureFlagName): void {
    this.isFeatureFlagEnabledByName.set(featureFlagName, true)
  }

  isFeatureFlagEnabled(featureFlagName: FeatureFlagName): boolean {
    return this.isFeatureFlagEnabledByName.get(featureFlagName) ?? false
  }

  public async initialize(packages: RNPackage[]) {
    const stopTracing = this.logger.clone("initialize").startTracing()
    const { descriptorWrapperFactoryByDescriptorType, turboModuleProvider } = await this.processPackages(packages)
    this.turboModuleProvider = turboModuleProvider
    this.descriptorRegistry = new DescriptorRegistry(
      {
        '1': { ...rootDescriptor },
      },
      this.updateState.bind(this),
      this,
      descriptorWrapperFactoryByDescriptorType,
      this.logger,
    );
    const cppFeatureFlags: CppFeatureFlag[] = []
    if (this.shouldUseCApiArchitecture) {
      cppFeatureFlags.push("C_API_ARCH")
    }
    if (this.shouldUseNDKToMeasureText) {
      cppFeatureFlags.push("ENABLE_NDK_TEXT_MEASURING")
    }
    if (this.shouldUseCApiArchitecture && !this.frameNodeFactory) {
      throw new RNOHError({
        whatHappened: "frameNodeFactory is undefined, but it is required when C-API architecture is enabled",
        howCanItBeFixed: []
      })
    }
    this.napiBridge.createReactNativeInstance(
      this.id,
      this.turboModuleProvider,
      this.frameNodeFactory,
      (mutations) => {
        try {
          this.descriptorRegistry.applyMutations(mutations)
        } catch (err) {
          if (typeof err === "string") {
            this.logger.error(err)
          } else if (err instanceof Error) {
            this.logger.error(err.message)
            if (err.stack) {
              this.logger.error(err.stack)
            }
          } else {
            this.logger.error("Unexpected error when applying mutations")
          }
        }
      },
      (tag, commandName, args) => {
        this.componentCommandHub.dispatchCommand(tag, commandName, args)
      },
      (type, payload) => {
        this.onCppMessage(type, payload)
      },
      this.shouldEnableDebugger,
      this.shouldEnableBackgroundExecutor,
      cppFeatureFlags,
    )
    stopTracing()
  }

  private onCppMessage(type: string, payload: any) {
    try {
      this.cppEventEmitter.emit(type, payload);
      switch (type) {
        case "SCHEDULER_DID_SET_IS_JS_RESPONDER": {
          if (payload.blockNativeResponder) {
            this.responderLockDispatcher.onBlockResponder(payload.tag, "REACT_NATIVE")
          } else {
            this.responderLockDispatcher.onUnblockResponder("REACT_NATIVE")
          }
          break;
        }
        case "RNOH_ERROR": {
          this.logger.error(new RNOHError({
            whatHappened: payload.message,
            howCanItBeFixed: [],
            customStack: payload.stack,
            extraData: payload.nested,
          }));
          break;
        }
      }
    } catch (err) {
      this.logger.error(new RNOHError({
        whatHappened: `Failed to handle CPP Message: ${type}`,
        howCanItBeFixed: [],
        extraData: err
      }))
    }
  }

  public blockComponentsGestures(tag: Tag) {
    this.responderLockDispatcher.onBlockResponder(tag, "EXTERNAL")
    return () => {
      this.responderLockDispatcher.onUnblockResponder("EXTERNAL")
    }
  }

  private async processPackages(packages: RNPackage[]) {
    const logger = this.logger.clone("processPackages")
    const stopTracing = logger.startTracing()
    packages.unshift(new RNOHCorePackage({}));
    const turboModuleContext = this.createRNOHContext(this)
    const result = {
      descriptorWrapperFactoryByDescriptorType: packages.reduce((acc, pkg) => {
        const descriptorWrapperFactoryByDescriptorType = pkg.createDescriptorWrapperFactoryByDescriptorType({})
        for (const [descriptorType, descriptorWrapperFactory] of Object.entries(descriptorWrapperFactoryByDescriptorType)) {
          acc.set(descriptorType, descriptorWrapperFactory)
        }
        return acc
      }, new Map<string, DescriptorWrapperFactory>()),
      turboModuleProvider: new TurboModuleProvider(
        await Promise.all(packages.map(async (pkg, idx) => {
          const pkgDebugName = pkg.getDebugName()
          let traceName = `package${idx + 1}`
          if (pkgDebugName) {
            traceName += `: ${pkgDebugName}`
          }
          logger.clone(traceName).debug("")
          const turboModuleFactory = pkg.createTurboModulesFactory(turboModuleContext);
          await turboModuleFactory.prepareEagerTurboModules()
          return turboModuleFactory
        })),
        this.logger
      )
    }
    stopTracing()
    return result
  }

  public subscribeToLifecycleEvents<TEventName extends keyof LifecycleEventArgsByEventName>(type: TEventName, listener: (...args: LifecycleEventArgsByEventName[TEventName]) => void) {
    return this.lifecycleEventEmitter.subscribe(type, listener)
  }

  public getLifecycleState(): LifecycleState {
    return this.lifecycleState
  }

  public callRNFunction(moduleName: string, functionName: string, args: unknown[]): void {
    if (this.lifecycleState === LifecycleState.BEFORE_CREATE) {
      // wait until the bundle has been loaded before calling the function
      const cancel = this.subscribeToLifecycleEvents("JS_BUNDLE_EXECUTION_FINISH", () => {
        this.napiBridge.callRNFunction(this.id, moduleName, functionName, args);
        cancel();
      });
      return;
    }
    this.napiBridge.callRNFunction(this.id, moduleName, functionName, args)
  }

  public emitComponentEvent(tag: Tag, eventEmitRequestHandlerName: string, payload: any) {
    this.napiBridge.emitComponentEvent(this.id, tag, eventEmitRequestHandlerName, payload)
  }

  public emitDeviceEvent(eventName: string, params: any) {
    this.logger.clone(`emitDeviceEvent`).debug(eventName)
    this.callRNFunction("RCTDeviceEventEmitter", "emit", [eventName, params]);
  }

  public getBundleExecutionStatus(bundleURL: string): BundleExecutionStatus | undefined {
    return this.bundleExecutionStatusByBundleURL.get(bundleURL)
  }

  public async runJSBundle(jsBundleProvider: JSBundleProvider) {
    const stopTracing = this.logger.clone("runJSBundle").startTracing()
    const bundleURL = jsBundleProvider.getURL()
    const isMetroServer = jsBundleProvider.getHotReloadConfig() !== null
    try {
      this.devToolsController.eventEmitter.emit("SHOW_DEV_LOADING_VIEW", this.id, `Loading from ${jsBundleProvider.getHumanFriendlyURL()}...`)
      this.bundleExecutionStatusByBundleURL.set(bundleURL, "RUNNING")
      const jsBundle = await jsBundleProvider.getBundle((progress) => {
        this.devToolsController.eventEmitter.emit("SHOW_DEV_LOADING_VIEW", this.id, `Loading from ${jsBundleProvider.getHumanFriendlyURL()} (${Math.round(progress * 100)}%)`)
      })
      this.initialBundleUrl = this.initialBundleUrl ?? jsBundleProvider.getURL()
      await this.napiBridge.loadScript(this.id, jsBundle, bundleURL)
      this.lifecycleState = LifecycleState.READY
      const hotReloadConfig = jsBundleProvider.getHotReloadConfig()
      if (hotReloadConfig) {
        this.callRNFunction("HMRClient", "setup", ["harmony", hotReloadConfig.bundleEntry, hotReloadConfig.host, hotReloadConfig.port, true])
        this.logger.info("Configured hot reloading")
      }
      const isRemoteBundle = bundleURL.startsWith("http")
      if (this.shouldEnableDebugger && isRemoteBundle) {
        DevServerHelper.connectToDevServer(bundleURL, this.logger, this.napiBridge.getInspectorWrapper());
      }
      this.bundleExecutionStatusByBundleURL.set(bundleURL, "DONE")
      this.lifecycleEventEmitter.emit("JS_BUNDLE_EXECUTION_FINISH", {
        jsBundleUrl: bundleURL,
        appKeys: jsBundleProvider.getAppKeys()
      })
    } catch (err) {
      this.bundleExecutionStatusByBundleURL.delete(bundleURL)
      if (err instanceof JSBundleProviderError) {
        this.logger.error(err)
      } else {
        const suggestions: string[] = []
        if (isMetroServer) {
          suggestions.push("Please check your Metro Server console. Likely, the error details you need are displayed there.")
        }
        suggestions.push("Please revise your application code. It may contain syntax errors or unhandled exceptions at the top level that could be causing runtime failures.")
        this.logger.error(new RNOHError({
          whatHappened: "Couldn't run a JS bundle",
          howCanItBeFixed: suggestions,
          extraData: err,
        }))
      }
    } finally {
      this.devToolsController.eventEmitter.emit("HIDE_DEV_LOADING_VIEW", this.id)
      stopTracing()
    }
  }

  public getTurboModule<T extends TurboModule>(name: string): T {
    return this.turboModuleProvider.getModule(name);
  }

  public createSurface(appKey: string): SurfaceHandle {
    const stopTracing = this.logger.clone("createSurface").startTracing()
    const tag = this.getNextSurfaceTag();
    const result = new SurfaceHandle(this, tag, appKey, this.defaultProps, this.napiBridge, (handle) => this.surfaceHandles.delete(handle));
    this.surfaceHandles.add(result)
    stopTracing()
    return result
  }

  public updateState(componentName: string, tag: Tag, state: unknown): void {
    const stopTracing = this.logger.clone("updateState").startTracing()
    this.napiBridge.updateState(this.id, componentName, tag, state)
    stopTracing()
  }

  public onBackPress() {
    this.emitDeviceEvent('hardwareBackPress', {})
  }

  public onForeground() {
    this.lifecycleState = LifecycleState.READY
    this.lifecycleEventEmitter.emit("FOREGROUND")
  }

  public onBackground() {
    this.lifecycleState = LifecycleState.PAUSED
    this.lifecycleEventEmitter.emit("BACKGROUND")
  }

  public onConfigurationUpdate(...args: Parameters<UIAbility["onConfigurationUpdate"]>) {
    this.lifecycleEventEmitter.emit("CONFIGURATION_UPDATE", ...args)
  }

  public onWindowSizeChange(windowSize: window.Size) {
    this.lifecycleEventEmitter.emit("WINDOW_SIZE_CHANGE", windowSize);
    this.postMessageToCpp("WINDOW_SIZE_CHANGE", windowSize);
  }

  private getNextSurfaceTag(): Tag {
    // NOTE: this is done to mirror the iOS implementation.
    // For details, see `RCTAllocateRootViewTag` in iOS implementation.
    return (this.surfaceCounter++ * 10) + 1;
  }

  public shouldUIBeUpdated(): boolean {
    return this.lifecycleState === LifecycleState.READY
  }

  public bindComponentNameToDescriptorType(componentName: string, descriptorType: string): void {
    this.componentNameByDescriptorType.set(descriptorType, componentName)
  }

  public getComponentNameFromDescriptorType(descriptorType: string): string {
    if (this.componentNameByDescriptorType.has(descriptorType)) {
      return this.componentNameByDescriptorType.get(descriptorType)!
    }
    return descriptorType
  }

  public getInitialBundleUrl(): string | undefined {
    return this.initialBundleUrl
  }


  private subscribeToDevTools() {
    const emitter = this.devToolsController.eventEmitter;
    emitter.subscribe("TOGGLE_ELEMENT_INSPECTOR", () => this.emitDeviceEvent("toggleElementInspector", {}))
    emitter.subscribe("DEV_MENU_SHOWN", () => this.emitDeviceEvent("RCTDevMenuShown", {}))
    emitter.subscribe("DID_PRESS_MENU_ITEM", (item) => this.emitDeviceEvent("didPressMenuItem", item))
    emitter.subscribe("OPEN_URL", (url, onError) => {
      DevServerHelper.openUrl(url, this.getInitialBundleUrl(), onError);
    })
  }

  public postMessageToCpp(name: string, payload: any): void {
    this.napiBridge.postMessageToCpp(name, { rnInstanceId: this.id, payload });
  }
}

