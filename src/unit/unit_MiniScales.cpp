/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_MiniScales.cpp
  @brief MiniScales Unit for M5UnitUnified
 */
#include "unit_MiniScales.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::miniscales;
using namespace m5::unit::miniscales::command;

namespace m5 {
namespace unit {

const char UnitMiniScales::name[] = "UnitMiniScales";
const types::uid_t UnitMiniScales::uid{"UnitMiniScales"_mmh3};
const types::uid_t UnitMiniScales::attr{0};

void UnitMiniScales::update(const bool force)
{
    UnitWeightI2C::update(force);
    if (_cfg.manage_button_status) {
        _prev_button = _button;
        readButtonStatus(_button);
    }
}

bool UnitMiniScales::readLEDColor(uint32_t& rgb32)
{
    rgb32 = 0;

    uint8_t r{}, g{}, b{};
    if (readLEDColor(r, g, b)) {
        rgb32 = (r << 16) | (g << 8) | b;
        return true;
    }
    return false;
}

bool UnitMiniScales::readLEDColor(uint8_t& r, uint8_t& g, uint8_t& b)
{
    uint8_t tmp[3]{};
    if (read_register(RGB_LED_REG, tmp, 3U)) {
        r = tmp[0];
        g = tmp[1];
        b = tmp[2];
        return true;
    }
    return false;
}

bool UnitMiniScales::writeLEDColor(const uint8_t r, const uint8_t g, const uint8_t b)
{
    uint8_t color[3] = {r, g, b};
    return writeRegister(RGB_LED_REG, color, 3U);
}

bool UnitMiniScales::writeLEDColor(const uint16_t rgb16)
{
    // Same as M5GFX colortype rgb565_t
    union {
        struct {
            uint16_t b5 : 5;
            uint16_t g6 : 6;
            uint16_t r5 : 5;
        };
        uint16_t raw;
    } rgb565{};
    rgb565.raw = rgb16;
    uint8_t r  = (rgb565.r5 << 3) + (rgb565.r5 >> 2);
    uint8_t g  = (rgb565.g6 << 2) + (rgb565.g6 >> 4);
    uint8_t b  = (rgb565.b5 << 3) + (rgb565.b5 >> 2);

    M5_LIB_LOGE("%x %u/%u/%u", rgb16, r, g, b);

    return writeLEDColor(r, g, b);
}

bool UnitMiniScales::readButtonStatus(bool& press)
{
    press = false;
    // 0:press 1:no press
    uint8_t v{};
    if (read_register8(BUTTON_REG, v)) {
        press = (v == 0);  // Invert status
        return true;
    }
    return false;
}

}  // namespace unit
}  // namespace m5
