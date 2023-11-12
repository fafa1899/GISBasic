/**
 * @license
 * Cesium - https://github.com/CesiumGS/cesium
 * Version 1.110.1
 *
 * Copyright 2011-2022 Cesium Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Columbus View (Pat. Pend.)
 *
 * Portions licensed separately.
 * See https://github.com/CesiumGS/cesium/blob/main/LICENSE.md for full licensing details.
 */

import {
  FrustumGeometry_default
} from "./chunk-XEOY7FXL.js";
import "./chunk-HYNBEK4C.js";
import "./chunk-FDNL4I6K.js";
import "./chunk-I6R5G76Q.js";
import "./chunk-VEPSTOBV.js";
import "./chunk-TV4Q4TKV.js";
import "./chunk-MQ32KIOU.js";
import "./chunk-EJWAO37W.js";
import "./chunk-7W3AMBD6.js";
import "./chunk-M57VBCI4.js";
import "./chunk-A7T4GOG3.js";
import "./chunk-GQ66IPL6.js";
import "./chunk-ZADWOYGF.js";
import "./chunk-CGHLEZ43.js";
import "./chunk-6EZ4NFIC.js";
import {
  defined_default
} from "./chunk-XZKRFAT7.js";

// packages/engine/Source/Workers/createFrustumGeometry.js
function createFrustumGeometry(frustumGeometry, offset) {
  if (defined_default(offset)) {
    frustumGeometry = FrustumGeometry_default.unpack(frustumGeometry, offset);
  }
  return FrustumGeometry_default.createGeometry(frustumGeometry);
}
var createFrustumGeometry_default = createFrustumGeometry;
export {
  createFrustumGeometry_default as default
};
