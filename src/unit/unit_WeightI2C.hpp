/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_WeightI2C.hpp
  @brief WeightI2C Unit for M5UnitUnified
 */
#ifndef M5_UNIT_WEIGHT_I2C_UNIT_WEIGHT_I2C_HPP
#define M5_UNIT_WEIGHT_I2C_UNIT_WEIGHT_I2C_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <m5_utility/types.hpp>
#include <array>
#include <limits>  // NaN

namespace m5 {
namespace unit {

namespace weighti2c {

/*!
  @enum Mode
  @brief Measurement mode
 */
enum class Mode : uint8_t { Float, Int };

/*!
  @struct Data
  @brief Measurement data group
 */
struct Data {
    static_assert(sizeof(float) == 4, "Invalid float size");
    std::array<uint8_t, 4> raw{};  //!< RAW data
    bool is_float{};

    inline float weight() const
    {
        return is_float ? *(float*)raw.data() : std::numeric_limits<float>::quiet_NaN();
    }
    inline int32_t iweight() const
    {
        return !is_float ? (int32_t)((uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16) |
                                     ((uint32_t)raw[3] << 24))
                         : std::numeric_limits<int32_t>::min();
    }
};
}  // namespace weighti2c

/*!
  @class UnitWeightI2C
  @brief WeightI2C unit
*/
class UnitWeightI2C : public Component, public PeriodicMeasurementAdapter<UnitWeightI2C, weighti2c::Data> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitWeightI2C, 0x26);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Enable the Low-Pass Filter
        bool lp_enable{true};
        //! Averaging Filter level (0 - 50)
        uint8_t avg_filter_level{10};
        // Exponential Moving Average Filter alpha (0-99)
        uint8_t ema_filter_alpha{10};
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Measurement mode if start on begin
        weighti2c::Mode mode{weighti2c::Mode::Float};
        //! Measurement interval if start on begin
        uint32_t interval{80};
    };

    explicit UnitWeightI2C(const uint8_t addr = DEFAULT_ADDRESS)
        : Component(addr), _data{new m5::container::CircularBuffer<weighti2c::Data>(1)}
    {
        auto ccfg  = component_config();
        ccfg.clock = 100 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitWeightI2C()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@warning Depends on Mode
    ///@name Measurement data by periodic
    ///@{
    /*!
      @brief Oldest measured weight (float)
      @warning Depends on Mode
     */
    inline float weight() const
    {
        return !empty() ? oldest().weight() : std::numeric_limits<float>::quiet_NaN();
    }
    /*!
      @brief Oldest measured weight (integer)
      @warning Depends on Mode
     */
    inline int32_t iweight() const
    {
        return !empty() ? oldest().iweight() : std::numeric_limits<int32_t>::min();
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param mode Measurement mode
      @param interval Measurement interval
      @return True if successful
    */
    inline bool startPeriodicMeasurement(const weighti2c::Mode mode, const uint32_t interval = 80)
    {
        return PeriodicMeasurementAdapter<UnitWeightI2C, weighti2c::Data>::startPeriodicMeasurement(mode, interval);
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitWeightI2C, weighti2c::Data>::stopPeriodicMeasurement();
    }
    ///@}

    ///@name Single shot measurement
    ///@{
    /*!
      @brief Measurement single shot
      @param[out] data Measuerd data
      @param mode Measurement mode
      @warning During periodic detection runs, an error is returned

    */
    bool measureSingleshot(weighti2c::Data& data, const weighti2c::Mode mode);
    /*!
      @brief Measurement single shot return as string
      @param[out] buf string buffer
      @return True if successful
      @warning Buffer length must be at least 16 bytes
      @warning During periodic detection runs, an error is returned
     */
    bool measureSingleshot(char* buf);
    ///@}

    ///@name Setting gap to calibration
    ///@{
    /*!
      @brief Read the gap value
      @param[out] gap value
      @return True if successful
     */
    bool readGap(float& gap);
    /*!
      @brief Write the gap value
      @param gap value
      @duration duration Max command duration(ms)
      @return True if successful
     */
    bool writeGap(const float gap, const uint32_t duration = 100);
    ///@}

    /*!
      @brief Reset offset
      @return True if successful
     */
    bool resetOffset();

    ///@name Filter
    ///@{
    /*!
      @brief Is enabled the Low-Pass Filter?
      @param[out] enabled True if enabled
      @return True if successful
     */
    bool isEnabledLPFilter(bool& enabled);
    /*!
      @brief Enable the Low-Pass Filter
      @param enable True:enable False:disable
      @return True if successful
     */
    bool enableLPFilter(const bool enable);
    /*!
      @brief Read the Averaging Filter level
      @param[out] level value
      @return True if successful
     */
    bool readAvgFilterLevel(uint8_t& level);
    /*!
      @brief Write the Averaging Filter level
      @param level value
      @return True if successful
      @warning Valid values range from 0 to 50
     */
    bool writeAvgFilterLevel(const uint8_t level);
    /*!
      @brief Read the Exponential Moving Average Filter alpha
      @param[out] alpha value
      @return True if successful
     */
    bool readEmaFilterAlpha(uint8_t& alpha);
    /*!
      @brief Write the Exponential Moving Average Filter alpha
      @param alpha value
      @return True if successful
      @warning Valid values range from 0 to 99
    */
    bool writeEmaFilterAlpha(const uint8_t alpha);
    ///@}

    /*!
      @brief Read the Raw ADC
      @param[out] value Raw ADC
      @return True if successful
     */
    bool readRawADC(int32_t& value);

    ///@warning Handling warning
    ///@name I2C Address
    ///@{
    /*!
      @brief Read the I2C address
      @param i2c_address[out] I2C address
      @return True if successful
     */
    bool readI2CAddress(uint8_t& i2c_address);
    /*!
      @brief Change unit I2C address
      @param i2c_address I2C address
      @return True if successful
    */
    bool changeI2CAddress(const uint8_t i2c_address);
    ///@}

protected:
    bool read_register(const uint8_t reg, uint8_t* buf, const size_t len);
    inline bool read_register8(const uint8_t reg, uint8_t& val)
    {
        return read_register(reg, &val, 1);
    }

    bool start_periodic_measurement(const weighti2c::Mode mode, const uint32_t interval);
    bool stop_periodic_measurement();
    bool read_measurement(weighti2c::Data& d, const weighti2c::Mode m);
    bool read_filter(uint8_t* buf3);
    bool write_filter(const uint8_t* buf3);

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitWeightI2C, weighti2c::Data);

protected:
    weighti2c::Mode _mode{};
    std::unique_ptr<m5::container::CircularBuffer<weighti2c::Data>> _data{};
    config_t _cfg{};
};

namespace weighti2c {
namespace command {
///@cond
// clang-format off
constexpr uint8_t RAW_ADC_REG           {0x00}; // R
constexpr uint8_t WEIGHT_REG            {0x10}; // R
constexpr uint8_t GAP_REG               {0x40}; // R/W
constexpr uint8_t OFFSET_REG            {0x50}; // W
constexpr uint8_t WEIGHTX100_INT_REG    {0x60}; // R
constexpr uint8_t WEIGHTX100_STRING_REG {0x70}; // R
constexpr uint8_t FILTER_REG            {0x80}; // R/W
constexpr uint8_t FILTER_LP_REG         {0x80}; // R/W
constexpr uint8_t FILTER_AVG_REG        {0x81}; // R/W
constexpr uint8_t FILTER_EMA_REG        {0x82}; // R/W
constexpr uint8_t FIRMWARE_VERSION_REG  {0xFE}; // R
constexpr uint8_t I2C_ADDRESS_REG       {0xFF}; // R/W
// clang-format on
///@endcond
}  // namespace command
}  // namespace weighti2c

}  // namespace unit
}  // namespace m5
#endif
