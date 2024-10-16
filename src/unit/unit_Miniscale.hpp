/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Miniscale.hpp
  @brief Miniscale Unit for M5UnitUnified
 */
#ifndef M5_UNIT_WEIGHT_I2C_UNIT_MINISCALE_HPP
#define M5_UNIT_WEIGHT_I2C_UNIT_MINISCALE_HPP

#include "unit_WeightI2C.hpp"

namespace m5 {
namespace unit {

class UnitMiniscale : public UnitWeightI2C {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitMiniscale, 0x26);

public:
    explicit UnitMiniscale(const uint8_t addr = DEFAULT_ADDRESS) : UnitWeightI2C(addr)
    {
    }
    virtual ~UnitMiniscale()
    {
    }

    using UnitWeightI2C::config;

    ///@name LED
    ///@{
    /*!
      @brief Read the LED color as RGB32 (00RRGGBB HEX)
      @param[out] rgb32 color
      @return True if successful
    */
    bool readLEDColor(uint32_t& rgb32);
    /*!
      @brief Read the LED color as each colors
      @param[out] r R
      @param[out] g G
      @param[out] b B
      @return True if successful
    */
    bool readLEDColor(uint8_t& r, uint8_t& g, uint8_t& b);
    /*!
      @brief Write the LED color as RGB32 (00RRGGBB HEX)
      @param rgb32 color
      @return True if successful
    */
    inline bool writeLEDColor(const uint32_t rgb32)
    {
        return writeLEDColor((rgb32 >> 16) & 0xFF, (rgb32 >> 8) & 0xFF, rgb32 & 0xFF);
    }
    /*!
      @brief Write the LED color as RGB16 (RRRRRGGGGGGBBBBB BIN)
      @param rgb16 color
      @return True if successful
    */
    bool writeLEDColor(const uint16_t rgb16);
    /*!
      @brief Write the LED color as each colors
      @param r R
      @param g G
      @param b B
      @return True if successful
    */
    bool writeLEDColor(const uint8_t r, const uint8_t g, const uint8_t b);
    ///@}

    ////@name Button
    ///@{
    /*!
      @brief Is button pressed?
      @return True if pressed
      @warning Returns false if a communication error occurs
     */
    bool isPressed();
    ///@}
};

namespace miniscale {
namespace command {
/// @cond
// clang-format off
constexpr uint8_t BUTTON_REG    {0x20};
constexpr uint8_t RGB_LED_REG   {0x30};
// clang-format on
/// @endcond

}  // namespace command
}  // namespace miniscale

}  // namespace unit
}  // namespace m5

#endif
