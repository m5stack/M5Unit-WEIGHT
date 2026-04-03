/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_MiniScales.hpp
  @brief MiniScales Unit for M5UnitUnified
 */
#ifndef M5_UNIT_WEIGHT_I2C_UNIT_MINISCALES_HPP
#define M5_UNIT_WEIGHT_I2C_UNIT_MINISCALES_HPP

#include "unit_WeightI2C.hpp"

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitMiniScales
  @brief MiniScales unit
*/
class UnitMiniScales : public UnitWeightI2C {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitMiniScales, 0x26);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t : public UnitWeightI2C::config_t {
        //! Manage button status with update?
        bool manage_button_status{true};
    };

    explicit UnitMiniScales(const uint8_t addr = DEFAULT_ADDRESS) : UnitWeightI2C(addr)
    {
    }
    virtual ~UnitMiniScales()
    {
    }

    /*!
      @brief Update the cached measurement and button state
      @param force Force the update even when the normal timing check would skip it
     */
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config()
    {
        return _cfg_mini;
    }
    //! @brief Set the configuration
    inline void config(const config_t& cfg)
    {
        _cfg_mini = cfg;
        UnitWeightI2C::config(static_cast<UnitWeightI2C::config_t>(_cfg_mini));
    }
    ///@}

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

    ///@name Button
    ///@{
    /*!
      @brief Read the button status
      @param[out] press Press if true
      @return True if successful
     */
    bool readButtonStatus(bool& press);
    /*!
      @brief Is button pressed?
      @return True if pressed
      @note The state is managed by update
     */
    inline bool isPressed() const
    {
        return _button;
    }
    /*!
      @brief Was button pressed?
      @return True if pressed
      @note The state is managed by update
     */
    inline bool wasPressed() const
    {
        return _button && (_button != _prev_button);
    }
    /*!
      @brief Is button released?
      @return True if released
      @note The state is managed by update
     */
    inline bool wasReleased() const
    {
        return !_button && (_button != _prev_button);
    }
    ///@}

private:
    bool _button{}, _prev_button{};
    config_t _cfg_mini{};
};

namespace miniscales {
namespace command {
/// @cond
// clang-format off
constexpr uint8_t BUTTON_REG    {0x20};
constexpr uint8_t RGB_LED_REG   {0x30};
// clang-format on
/// @endcond

}  // namespace command
}  // namespace miniscales

}  // namespace unit
}  // namespace m5

#endif
