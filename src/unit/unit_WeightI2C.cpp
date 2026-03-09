/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_WeightI2C.cpp
  @brief WeightI2C Unit for M5UnitUnified
 */
#include "unit_WeightI2C.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::weighti2c;
using namespace m5::unit::weighti2c::command;

namespace m5 {
namespace unit {

const char UnitWeightI2C::name[] = "UnitWeightI2C";
const types::uid_t UnitWeightI2C::uid{"UnitWeightI2C"_mmh3};
const types::attr_t UnitWeightI2C::attr{attribute::AccessI2C};

bool UnitWeightI2C::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    // WeightI2C/MiniScales use HX711. Wait for power-up/reset settling (RATE=0: up to about 400ms)
    // so the first weight readings are stable.
    m5::utility::delay(400);

    uint8_t ver{};
    bool ready{};
    auto timeout_at = m5::utility::millis() + 1500;
    do {
        ready = read_register8(FIRMWARE_VERSION_REG, ver) && ver != 0;
        if (ready) {
            break;
        }
        m5::utility::delay(100);
    } while (!ready && m5::utility::millis() <= timeout_at);
    if (!ready) {
        M5_LIB_LOGE("Failed to read firmware version %x", ver);
        return false;
    }
    M5_LIB_LOGD("firmware: %x", ver);

    // Detect GAP sign for polarity correction
    float gap{};
    if (readGap(gap)) {
        _gap_negative = (gap < 0.0f);
        if (_gap_negative) {
            M5_LIB_LOGD("GAP is negative (%f), weight sign will be corrected", gap);
        }
    }

    // Filter
    if (!enableLPFilter(_cfg.lp_enable) || !writeAvgFilterLevel(_cfg.avg_filter_level) ||
        !writeEmaFilterAlpha(_cfg.ema_filter_alpha)) {
        M5_LIB_LOGE("Failed to write filter");
        return false;
    }

    return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.mode, _cfg.interval) : true;
}

void UnitWeightI2C::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            _updated = read_measurement(d, _mode);
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitWeightI2C::start_periodic_measurement(const weighti2c::Mode mode, const uint32_t interval)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    _mode     = mode;
    _interval = interval;
    _latest   = 0;
    _periodic = true;
    return true;
}

bool UnitWeightI2C::stop_periodic_measurement()
{
    _periodic = false;
    return true;
}

bool UnitWeightI2C::measureSingleshot(weighti2c::Data& data, const weighti2c::Mode mode)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (!read_measurement(data, mode)) {
        return false;
    }
    // Correct sign when GAP is negative
    if (_gap_negative) {
        if (data.is_float) {
            float w = *(float*)data.raw.data();
            w       = -w;
            std::memcpy(data.raw.data(), &w, 4);
        } else {
            int32_t w = (int32_t)((uint32_t)data.raw[0] | ((uint32_t)data.raw[1] << 8) | ((uint32_t)data.raw[2] << 16) |
                                  ((uint32_t)data.raw[3] << 24));
            w         = -w;
            std::memcpy(data.raw.data(), &w, 4);
        }
    }
    return true;
}

bool UnitWeightI2C::measureSingleshot(char* buf)
{
    if (inPeriodic()) {
        M5_LIB_LOGD("Periodic measurements are running");
        return false;
    }
    if (buf) {
        buf[0] = '\0';
        // Spec: max 15 characters + '\0'
        auto ok = read_register(WEIGHTX100_STRING_REG, reinterpret_cast<uint8_t*>(buf), 16U);
        buf[15] = '\0';  // Defensive termination even if firmware returns malformed payload
        return ok;
    }
    return false;
}

bool UnitWeightI2C::readGap(float& gap)
{
    uint8_t buf[4]{};
    if (read_register(GAP_REG, buf, 4U)) {
        gap = *(float*)buf;
        return true;
    }
    return false;
}

bool UnitWeightI2C::writeGap(const float gap, const uint32_t duration)
{
    uint8_t buf[4]{};
    std::memcpy(buf, (uint8_t*)&gap, 4);
    if (writeRegister(GAP_REG, buf, 4U)) {
        m5::utility::delay(duration);
        return true;
    }
    return false;
}

bool UnitWeightI2C::resetOffset()
{
    return writeRegister8(OFFSET_REG, 0x01);  // write 1: reset offset
}

bool UnitWeightI2C::readRawADC(int32_t& value)
{
    uint8_t buf[4];
    if (read_register(RAW_ADC_REG, buf, 4U)) {
        value = buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
        return true;
    }
    return false;
}

bool UnitWeightI2C::isEnabledLPFilter(bool& enabled)
{
    enabled = false;
    uint8_t v{};
    if (read_register8(FILTER_LP_REG, v)) {
        enabled = v;
        return true;
    }
    return false;
}

bool UnitWeightI2C::enableLPFilter(const bool enable)
{
    return writeRegister8(FILTER_LP_REG, enable ? 0x01 : 0x00);
}

bool UnitWeightI2C::readAvgFilterLevel(uint8_t& level)
{
    return read_register8(FILTER_AVG_REG, level);
}

bool UnitWeightI2C::writeAvgFilterLevel(const uint8_t level)
{
    if (level > 50) {
        M5_LIB_LOGE("Must be 0-50");
        return false;
    }
    return writeRegister8(FILTER_AVG_REG, level);
}

bool UnitWeightI2C::readEmaFilterAlpha(uint8_t& alpha)
{
    return read_register8(FILTER_EMA_REG, alpha);
}

bool UnitWeightI2C::writeEmaFilterAlpha(const uint8_t alpha)
{
    if (alpha > 99) {
        M5_LIB_LOGE("Must be 0-99");
        return false;
    }
    return writeRegister8(FILTER_EMA_REG, alpha);
}

bool UnitWeightI2C::readI2CAddress(uint8_t& i2c_address)
{
    i2c_address = 0;
    return read_register8(I2C_ADDRESS_REG, i2c_address);
}

bool UnitWeightI2C::changeI2CAddress(const uint8_t i2c_address)
{
    if (!m5::utility::isValidI2CAddress(i2c_address)) {
        M5_LIB_LOGE("Invalid address : %02X", i2c_address);
        return false;
    }
    if (writeRegister8(I2C_ADDRESS_REG, i2c_address) && changeAddress(i2c_address)) {
        // Wait wakeup
        uint8_t v{};
        bool done{};
        auto timeout_at = m5::utility::millis() + 1000;
        do {
            m5::utility::delay(1);
            done = (read_register8(I2C_ADDRESS_REG, v) && v == i2c_address);
        } while (!done && m5::utility::millis() <= timeout_at);
        return done;
    }
    return false;
}

bool UnitWeightI2C::read_measurement(weighti2c::Data& d, const weighti2c::Mode m)
{
    d.is_float = m == Mode::Float;
    auto ret   = read_register(m == Mode::Float ? WEIGHT_REG : WEIGHTX100_INT_REG, d.raw.data(), d.raw.size());
    return ret;
}

bool UnitWeightI2C::read_register(const uint8_t reg, uint8_t* buf, const size_t len)
{
    // Read after writing register without stopbit
    return writeWithTransaction(reg, nullptr, 0U, false) == m5::hal::error::error_t::OK &&
           readWithTransaction(buf, len) == m5::hal::error::error_t::OK;
}

}  // namespace unit
}  // namespace m5
