/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitWeightI2C and UnitMiniScales via UnitPaHub

  Board ---> PaHub (0x70)
               |- ch:0 UnitWeightI2C  (0x26)
               `- ch:1 UnitMiniScales (0x26)

  Both units use the same I2C address (0x26).
  Each PaHub channel is an independent I2C bus, so they can be used at the same time.
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedHUB.h>  // UnitPaHub
#include <M5UnitUnifiedWEIGHT.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;

// The default address of PaHub is 0x70
m5::unit::UnitPaHub hub;

m5::unit::UnitWeightI2C weight_i2c;
m5::unit::UnitMiniScales mini_scales;

// PaHub channels used by each unit
constexpr uint8_t CH_WEIGHT_I2C{0};
constexpr uint8_t CH_MINI_SCALES{1};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    if (!hub.add(weight_i2c, CH_WEIGHT_I2C) || !hub.add(mini_scales, CH_MINI_SCALES)) {
        M5_LOGE("Failed to add");
        m5::unit::wiring::failStop();
    }

    // The hub is the I2C device on the port
    // NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        M5_LOGW("%s", Units.debugInfo().c_str());
        m5::unit::wiring::failStop();
    }

    weight_i2c.resetOffset();
    mini_scales.resetOffset();

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();

    // Can be checked e.g. by serial plotters
    if (weight_i2c.updated()) {
        M5.Log.printf(">WeightI2C:%f\n", weight_i2c.weight());
    }
    if (mini_scales.updated()) {
        M5.Log.printf(">MiniScales:%f\n", mini_scales.weight());
    }

    // Tare both units
    if (M5.BtnA.wasClicked()) {
        weight_i2c.resetOffset();
        mini_scales.resetOffset();
    }
}
