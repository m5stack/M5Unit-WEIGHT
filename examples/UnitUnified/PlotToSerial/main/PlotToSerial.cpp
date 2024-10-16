/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitWeightI2C
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedWeightI2C.h>
#include <M5Utility.h>

using m5::unit::weighti2c::Mode;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitWeightI2C unit;

uint32_t idx{};
constexpr Mode mode_table[] = {Mode::Float, Mode::Int};

}  // namespace

void setup()
{
    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    unit.resetOffset();

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();
    if (unit.updated()) {
        // Can be checked e.g. by serial plotters
        if (!idx) {
            M5_LOGI("\n>Weight:%f", unit.weight());
        } else {
            M5_LOGI("\n>iWeight:%d", unit.iweight());
        }
    }

    // Behavior when BtnA is clicked changes depending on the value.
    constexpr int32_t BTN_A_FUNCTION{-1};

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
                    M5_LOGI("\n>Singleshort:%s", txt);
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
