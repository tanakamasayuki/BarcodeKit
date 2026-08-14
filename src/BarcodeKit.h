// BarcodeKit - barcode and QR code generator for embedded systems.
//
//   https://github.com/tanakamasayuki/BarcodeKit
//
// Generates the logical black/white module pattern only; drawing, printing
// and transmitting are up to you. Include BarcodeKitDraw.h for optional
// drawing helpers.
//
// You provide the buffer and the library never allocates:
//
//   uint8_t buf[BarcodeKit::Code128::bufferSize(16)];
//   BarcodeKit::Code128 bc;
//   if (bc.encode("ABC-12345", buf, sizeof(buf))) {
//     for (uint16_t x = 0; x < bc.width(); x++) use(bc.module(x, 0));
//   }
//
// Keep `buf` alive for as long as you call module(): the object does not own
// it. See docs/FORMATS.md for the per-format reference.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "BarcodeKit/Common.h"

#include "BarcodeKit/Code128.h"

#include "barcodekit_version.h"
