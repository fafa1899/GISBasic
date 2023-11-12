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
  BoxGeometry_default
} from "./chunk-FK5BM4WD.js";
import "./chunk-EZPA2AXQ.js";
import "./chunk-HYNBEK4C.js";
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

// packages/engine/Source/Workers/createBoxGeometry.js
function createBoxGeometry(boxGeometry, offset) {
  if (defined_default(offset)) {
    boxGeometry = BoxGeometry_default.unpack(boxGeometry, offset);
  }
  return BoxGeometry_default.createGeometry(boxGeometry);
}
var createBoxGeometry_default = createBoxGeometry;
export {
  createBoxGeometry_default as default
};
