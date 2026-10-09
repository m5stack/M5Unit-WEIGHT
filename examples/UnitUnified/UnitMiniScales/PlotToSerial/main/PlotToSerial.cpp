/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitMiniScales
*/
#include <cmath>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedWEIGHT.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

using m5::unit::weighti2c::Mode;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitMiniScales unit;

uint32_t idx{};
constexpr Mode mode_table[] = {Mode::Float, Mode::Int};

constexpr float WEIGHT_MIN{0.0f};
constexpr float WEIGHT_MAX{5000.0f};  // MiniScales: 5kg load cell
bool led_enabled{true};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // Board-aware connection: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }
    float gap{};
    unit.readGap(gap);
    M5_LOGI("GAP:%f", gap);

#if 0
    // Restore factory GAP (enable temporarily if GAP was corrupted)
    unit.writeGap(400.0f);
    M5_LOGW("Wrote GAP=400.0");
#endif

    unit.resetOffset();

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.fillScreen(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        // Can be checked e.g. by serial plotters
        if (!idx) {
            M5.Log.printf(">Weight:%f\n", unit.weight());
        } else {
            M5.Log.printf(">iWeight:%d\n", unit.iweight());
        }

        // Update LED color based on weight: Blue(min) -> Red(max)
        if (led_enabled) {
            float w   = unit.weight();
            float t   = (w - WEIGHT_MIN) / (WEIGHT_MAX - WEIGHT_MIN);
            t         = std::fmin(1.0f, std::fmax(0.0f, t));
            uint8_t r = static_cast<uint8_t>(t * 255);
            uint8_t b = static_cast<uint8_t>((1.0f - t) * 255);
            unit.writeLEDColor(r, 0, b);
        }
    }

    // Button on MiniScales: toggle LED ON/OFF
    if (unit.wasPressed()) {
        led_enabled = !led_enabled;
        if (!led_enabled) {
            unit.writeLEDColor(0, 0, 0);
        }
    }

    // Behavior when BtnA is clicked changes depending on the value.
    constexpr int32_t BTN_A_FUNCTION{0};

    if (M5.BtnA.wasClicked()) {
        switch (BTN_A_FUNCTION) {
            case 0: {  // Change mode
                if (++idx > 1) {
                    idx = 0;
                }
                unit.stopPeriodicMeasurement();
                unit.startPeriodicMeasurement(mode_table[idx]);
            } break;
            case 1: {  // Singleshot as text
                static uint32_t sscnt{};
                unit.stopPeriodicMeasurement();
                char txt[16]{};
                if (unit.measureSingleshot(txt)) {
                    M5.Log.printf(">Singleshot:%s\n", txt);
                } else {
                    M5_LOGE("Failed to measure");
                }
                // Return to periodic measurement after 8 measurements
                if (++sscnt >= 8) {
                    sscnt = 0;
                    unit.startPeriodicMeasurement(Mode::Float);
                }

            } break;
        }
    }
}
