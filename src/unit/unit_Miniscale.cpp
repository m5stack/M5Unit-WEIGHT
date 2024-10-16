/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Miniscale.cpp
  @brief Miniscale Unit for M5UnitUnified
 */
#include "unit_Miniscale.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::miniscale;
using namespace m5::unit::miniscale::command;

namespace m5 {
namespace unit {

const char UnitMiniscale::name[] = "UnitMiniscale";
const types::uid_t UnitMiniscale::uid{"UnitMiniscale"_mmh3};
const types::uid_t UnitMiniscale::attr{0};

bool UnitMiniscale::readLEDColor(uint32_t& rgb32)
{
    uint8_t r{}, g{}, b{};
    if (readLEDColor(r, g, b)) {
        rgb32 = (r << 16) | (g << 8) | b;
        return true;
    }
    return false;
}

bool UnitMiniscale::readLEDColor(uint8_t& r, uint8_t& g, uint8_t& b)
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

bool UnitMiniscale::writeLEDColor(const uint8_t r, const uint8_t g, const uint8_t b)
{
    uint8_t color[3] = {r, g, b};
    return writeRegister(RGB_LED_REG, color, 3U);
}

bool UnitMiniscale::writeLEDColor(const uint16_t rgb16)
{
    uint_fast8_t r = (rgb16 >> 11) & 0x1F;
    r              = (r << 3) + (r >> 2);
    uint_fast8_t g = (rgb16 >> 5) & 0x3F;
    g              = (g << 2) + (g >> 4);
    uint_fast8_t b = rgb16 & 0x1F;
    b              = (b << 3) + (b >> 2);
    return writeLEDColor(r, g, b);
}

bool UnitMiniscale::isPressed()
{
    uint8_t v{};
    return read_register8(BUTTON_REG, v) && (v == 0);
}

}  // namespace unit
}  // namespace m5
