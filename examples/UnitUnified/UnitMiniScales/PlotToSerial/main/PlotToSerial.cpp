/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitMiniScales
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedWEIGHT.h>
#include <M5Utility.h>
#include <M5HAL.hpp>  // For NessoN1

using m5::unit::weighti2c::Mode;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitMiniScales unit;

uint32_t idx{};
constexpr Mode mode_table[] = {Mode::Float, Mode::Int};

constexpr lgfx::rgb888_t color_table[] = {
    {128, 16, 32},
    {32, 128, 16},
    {16, 32, 128},
    {0, 0, 0},
};

}  // namespace

void setup()
{
    m5::utility::delay(2000);

    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto board = M5.getBoard();

    // For NessoN1 GROVE
    if (board == m5::board_t::board_ArduinoNessoN1) {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);  // GROVE SDA
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);   // GROVE SCL
        M5_LOGI("getPin(NessoN1): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        // Wire is used internally, so SoftwareI2C handles the unit
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        if (!Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);
        if (!Units.add(unit, Wire) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    }
    unit.resetOffset();

    M5_LOGI("M5UnitUnified has been begun");
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
    }

    // Button on MiniScales
    if (unit.wasPressed()) {
        static uint32_t cidx{};
        unit.writeLEDColor((uint32_t)color_table[cidx]);
        cidx = (cidx + 1) % m5::stl::size(color_table);
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
                    M5.Log.printf(">Singleshort:%s\n", txt);
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
