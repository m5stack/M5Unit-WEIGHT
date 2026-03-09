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
#include <M5UnitUnifiedWEIGHT.h>
#include <M5Utility.h>
#include <M5HAL.hpp>

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
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto board = M5.getBoard();

    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Wire1 exists but is reserved for HatPort — cannot be used for GROVE.
    //   Reconfiguring Wire to GROVE pins breaks In_I2C, causing ESP_ERR_INVALID_STATE in M5.update().
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
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
