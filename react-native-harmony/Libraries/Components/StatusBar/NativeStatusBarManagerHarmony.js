/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict
 * @format
 */

import type { TurboModule } from "react-native/Libraries/TurboModule/RCTExport";

import * as TurboModuleRegistry from "react-native/Libraries/TurboModule/TurboModuleRegistry";

export interface Spec extends TurboModule {
  +getConstants: () => {|
    +HEIGHT: number,
    +DEFAULT_BACKGROUND_COLOR: number,
  |};
  +setColor: (color: string, animated: boolean) => void;
  +setTranslucent: (translucent: boolean) => void;

  /**
   *  - statusBarStyles can be:
   *    - 'default'
   *    - 'dark-content'
   */
  +setStyle: (statusBarStyle?: ?string, animated: boolean) => void;
    /**
   *  - withAnimation can be: 'none' | 'slide'
   */
  +setHidden: (hidden: boolean, withAnimation: string) => void;
}

const NativeModule = TurboModuleRegistry.getEnforcing<Spec>("StatusBarManager");
let constants = null;

const NativeStatusBarManager = {
  getConstants(): {|
    +HEIGHT: number,
    +DEFAULT_BACKGROUND_COLOR?: number,
  |} {
    if (constants == null) {
      constants = NativeModule.getConstants();
    }
    return constants;
  },

  setColor(color: number, animated: boolean): void {
    NativeModule.setColor(color, animated);
  },

  setTranslucent(translucent: boolean): void {
    NativeModule.setTranslucent(translucent);
  },

  /**
   *  - statusBarStyles can be:
   *    - 'default'
   *    - 'dark-content'
   */
  setStyle(statusBarStyle?: ?string, animated: boolean): void {
    NativeModule.setStyle(statusBarStyle, animated);
  },

  setHidden(hidden: boolean, withAnimation: string): void {
    NativeModule.setHidden(hidden, withAnimation);
  },
};

export default NativeStatusBarManager;
