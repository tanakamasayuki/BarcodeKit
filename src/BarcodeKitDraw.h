// BarcodeKit - optional drawing helpers.
//
//   #include <BarcodeKit.h>
//   #include <BarcodeKitDraw.h>
//
// Works out the scale, centring, quiet zone and guard-bar extension, which is
// the part everyone has to write otherwise and the part that quietly makes a
// symbol unreadable when it is wrong.
//
// The core is graphics-library independent: render() reports rectangles to a
// callback. Include LovyanGFX / M5GFX / M5Unified before this header to also
// get draw() and drawCentered() for those.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "BarcodeKit.h"

#include "BarcodeKitDraw/Callback.h"
#include "BarcodeKitDraw/LovyanGFX.h"
#include "BarcodeKitDraw/Serial.h"
