import { RNInstanceRegistry } from "./RNInstanceRegistry"
import { EventEmitter } from "./EventEmitter"


export class DevToolsController {
  public eventEmitter = new EventEmitter<{
    "RELOAD": [{ reason: string | undefined }],
    "DISMISS_RNOH_ERROR_DIALOG",
    "SHOW_DEV_LOADING_VIEW": [
      rnInstanceId: number,
      message: string,
      color?: number,
      backgroundColor?: number
    ],
    "HIDE_DEV_LOADING_VIEW": [      rnInstanceId: number,
    ]
  }>()

  constructor(private rnInstanceRegistry: RNInstanceRegistry) {
  }

  reload(reason: string | undefined = undefined): void {
    this.eventEmitter.emit("RELOAD", { reason })
  }

  toggleElementInspector(): void {
    this.rnInstanceRegistry.forEach(rnInstance => {
      rnInstance.emitDeviceEvent("toggleElementInspector", {});
    })
  }

  dismissRNOHErrorDialog(): void {
    this.eventEmitter.emit("DISMISS_RNOH_ERROR_DIALOG")
  }
}