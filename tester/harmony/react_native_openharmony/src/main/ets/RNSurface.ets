import { RNComponentFactory, RNView } from './RNOHCorePackage';
import {
  ComponentBuilderContext,
  CustomRNComponentFrameNodeFactory,
  Descriptor,
  DisplayMode,
  RNComponentContext,
  RNOHContext,
  RNOHError,
  RNOHLogger
} from './RNOH';
import {
  ViewBaseProps,
  ViewDescriptorWrapper,
  ViewDescriptorWrapperBase,
  ViewRawProps
} from './RNOHCorePackage/components/RNViewBase';
import { TouchDispatcher } from './RNOH/TouchDispatcher';
import { SurfaceHandle, SurfaceProps } from './RNOH/SurfaceHandle';
import { I18nManagerTurboModule } from './RNOHCorePackage/turboModules';

interface RootViewState {}

export type RootDescriptor = Descriptor<"RootView", ViewBaseProps, RootViewState, ViewRawProps>;

export interface SurfaceConfig {}

export interface SurfaceConfig1 extends SurfaceConfig {
  surfaceHandle: SurfaceHandle
}

export interface SurfaceConfig2 extends SurfaceConfig {
  appKey: string,
  initialProps: SurfaceProps,
}

@Component
export struct RNSurface {
  public ctx!: RNOHContext | RNComponentContext;
  public surfaceConfig!: SurfaceConfig;
  /**
   * @deprecated: provide RNComponentContext to RNSurface::ctx instead
   */
  @BuilderParam buildCustomComponent: (ctx: ComponentBuilderContext) => void;
  surfaceWidth: number = 0;
  surfaceHeight: number = 0;
  surfaceOffsetX: number = 0;
  surfaceOffsetY: number = 0;
  @State descriptor: RootDescriptor = Object() as RootDescriptor;
  @State descriptorWrapper: ViewDescriptorWrapperBase | undefined = undefined
  @State private shouldStartSurface: boolean = false;
  private surfaceHandle!: SurfaceHandle;
  private cleanUpCallbacks: (() => void)[] = [];
  private touchDispatcher!: TouchDispatcher;
  private shouldStopSurface: boolean = false;
  private initialProps!: SurfaceProps;
  private logger!: RNOHLogger

  aboutToAppear() {
    this.logger = this.ctx.logger.clone("RNSurface")
    if (!(this.ctx instanceof RNComponentContext)) {
      this.logger.error(new RNOHError({
        whatHappened: "RNSurface received RNOHContext instead of RNComponentContext. Custom RN components won't be visible.",
        howCanItBeFixed: ["Provide RNComponentContext instead of RNOHContext"]
      }))
    }
    const stopTracing = this.logger.clone("aboutToAppear").startTracing()
    this.ctx.rnInstance.setFrameNodeFactory(new CustomRNComponentFrameNodeFactory(this.getUIContext(), this.ctx.logger,
      () => {
        return this.ctx as RNComponentContext
      }))
    if (Object.keys(this.surfaceConfig).includes("surfaceHandle")) {
      this.surfaceHandle = (this.surfaceConfig as SurfaceConfig1).surfaceHandle;
      this.surfaceHandle.setDisplayMode(DisplayMode.Visible);
    } else {
      this.surfaceHandle = this.ctx.rnInstance.createSurface((this.surfaceConfig as SurfaceConfig2).appKey);
      this.initialProps = (this.surfaceConfig as SurfaceConfig2).initialProps;
      this.shouldStartSurface = true;
      this.shouldStopSurface = true;
    }
    const tag = this.surfaceHandle.getTag();
    this.onUpdateComponent(this.ctx.rnInstance.descriptorRegistry.getDescriptor(tag))
    this.touchDispatcher = new TouchDispatcher(tag, this.ctx.rnInstance, this.ctx.logger)
    this.cleanUpCallbacks.push(this.ctx.descriptorRegistry.subscribeToDescriptorChanges(
      tag,
      (newDescriptor) => this.onUpdateComponent(newDescriptor)
    ))
    stopTracing()
  }

  aboutToDisappear() {
    const stopTracing = this.logger.clone("aboutToDisappear").startTracing()
    this.cleanUpCallbacks.forEach(cb => cb())
    if (this.shouldStopSurface) {
      if (!this.surfaceHandle.isDestroyed()) {
        // NOTE: when the app is stopping, it's possible the ability cleaned up the surface
        // before us. This is fine.
        this.surfaceHandle.stop();
        this.surfaceHandle.destroy();
      }
    } else if (this.surfaceHandle.getDisplayMode() === DisplayMode.Visible) {
      this.surfaceHandle.setDisplayMode(DisplayMode.Suspended);
    }
    stopTracing()
  }

  onUpdateComponent(newDescriptor: Descriptor) {
    this.descriptor = newDescriptor as RootDescriptor
    this.descriptorWrapper = new ViewDescriptorWrapper(newDescriptor)
  }

  handleAreaChange(_oldValue: Area, newValue: Area) {
    const stopTracing = this.logger.clone("handleAreaChange").startTracing()
    this.surfaceWidth = newValue.width as number
    this.surfaceHeight = newValue.height as number
    this.surfaceOffsetX = newValue.globalPosition.x as number
    this.surfaceOffsetY = newValue.globalPosition.y as number
    this.updateSurfaceConstraints()
    stopTracing()
  }

  updateSurfaceConstraints() {
    let isRTL = this.ctx.rnInstance.getTurboModule<I18nManagerTurboModule>('I18nManager').getConstants()
      .isRTL;

    if (this.shouldStartSurface) {
      this.surfaceHandle.setDisplayMode(DisplayMode.Visible)
      this.surfaceHandle.start(
        {
          width: this.surfaceWidth,
          height: this.surfaceHeight,
          surfaceOffsetX: this.surfaceOffsetX,
          surfaceOffsetY: this.surfaceOffsetY,
          pixelRatio: vp2px(1),
          isRTL,
        },
        this.initialProps)
      this.shouldStartSurface = false
    } else {
      this.surfaceHandle.updateConstraints({
        width: this.surfaceWidth,
        height: this.surfaceHeight,
        surfaceOffsetX: this.surfaceOffsetX,
        surfaceOffsetY: this.surfaceOffsetY,
        pixelRatio: vp2px(1),
        isRTL,
      });
    }
  }

  handleTouch(event: TouchEvent) {
    if (this.ctx.rnInstance.getArchitecture() === "ARK_TS") {
      this.logger.clone("handleTouch").debug()
      this.touchDispatcher.handleTouchEvent(event)
    }
  }

  build() {
    Stack() {
      if (this.ctx.rnInstance.getArchitecture() === "C_API") {
        XComponent({
          id: this.ctx.rnInstance.getId() + "_" + this.surfaceHandle.getTag(),
          type: "node",
          libraryname: 'rnoh_app'
        })
          .width("100%")
          .height("100%")
      } else {
        if (this.ctx instanceof RNComponentContext) {
          RNView({
            ctx: this.ctx,
            tag: this.surfaceHandle.getTag()
          }) // THIS LINE IS WRONG!!!! WE MUST ALLOW REPLACING ROOT VIEW
        } else {
          RNComponentFactory({
            ctx: this.ctx,
            tag: this.surfaceHandle.getTag(),
            buildCustomComponent: this.buildCustomComponent
          })
        }
      }
    }
    .width("100%")
    .height("100%")
    .onAreaChange((oldArea, newArea) => this.handleAreaChange(oldArea, newArea))
    .onTouch((e) => this.handleTouch(e))
  }
}
