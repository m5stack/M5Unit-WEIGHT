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
const types::attr_t UnitMiniScales::attr{attribute::AccessI2C};

void UnitMiniScales::update(const bool force)
{
    UnitWeightI2C::update(force);
    if (_cfg_mini.manage_button_status) {
        _prev_button = _button;
        readButtonStatus(_button);
    }
}

bool UnitMiniScales::readLEDColor(uint32_t& rgb32)
{
    rgb32 = 0;

    uint8_t r{}, g{}, b{};
    if (readLEDColor(r, g, b)) {
        rgb32 = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
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
    const uint8_t r5 = static_cast<uint8_t>((rgb16 >> 11) & 0x1F);
    const uint8_t g6 = static_cast<uint8_t>((rgb16 >> 5) & 0x3F);
    const uint8_t b5 = static_cast<uint8_t>(rgb16 & 0x1F);

    uint8_t r = static_cast<uint8_t>((r5 << 3) + (r5 >> 2));
    uint8_t g = static_cast<uint8_t>((g6 << 2) + (g6 >> 4));
    uint8_t b = static_cast<uint8_t>((b5 << 3) + (b5 >> 2));

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
