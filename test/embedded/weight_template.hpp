/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Unit test template for UnitWeightI2C and inherited classes
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_WeightI2C.hpp>
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::weighti2c;
using namespace m5::unit::weighti2c::command;

class TestWeightI2C : public ComponentTestBase<UnitWeightI2C, bool> {
protected:
    virtual UnitWeightI2C* get_instance() override
    {
        auto ptr = new m5::unit::UnitWeightI2C();
        if (ptr) {
            auto ccfg        = ptr->component_config();
            ccfg.stored_size = 8;
            ptr->component_config(ccfg);
        }
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

namespace {
auto rng = std::default_random_engine{};

constexpr Mode mode_table[] = {Mode::Float, Mode::Int};

}  // namespace

TEST_P(TestWeightI2C, CheckConfig)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    //
    auto cfg           = unit->config();
    cfg.interval       = 78;
    cfg.start_periodic = false;
    unit->config(cfg);

    EXPECT_TRUE(unit->begin());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_NE(unit->interval(), 78);

    //
    cfg.lp_enable        = false;
    cfg.avg_filter_level = 34;
    cfg.ema_filter_alpha = 56;
    cfg.start_periodic   = true;
    unit->config(cfg);

    EXPECT_TRUE(unit->begin());

    bool lp{};
    uint8_t avg{}, ema{};
    EXPECT_TRUE(unit->isEnabledLPFilter(lp));
    EXPECT_TRUE(unit->readAvgFilterLevel(avg));
    EXPECT_TRUE(unit->readEmaFilterAlpha(ema));
    EXPECT_FALSE(lp);
    EXPECT_EQ(avg, 34);
    EXPECT_EQ(ema, 56);
    EXPECT_EQ(unit->interval(), 78);
    EXPECT_TRUE(unit->inPeriodic());
}

TEST_P(TestWeightI2C, Settings)
{
    SCOPED_TRACE(ustr);

    // GAP
    uint32_t cnt{8};
    while (cnt--) {
        float gap{};
        std::uniform_real_distribution<> dist(-100000.f, 100000.f);
        gap = dist(rng);
        EXPECT_TRUE(unit->writeGap(gap));
        float gap2{};
        EXPECT_TRUE(unit->readGap(gap2));
        // M5_LOGI("GAP:%f/%f", gap, gap2);
        EXPECT_EQ(gap, gap2);
        m5::utility::delay(1);
    }

    // Reset offset
    EXPECT_TRUE(unit->resetOffset());

    // Filter
    cnt = 32;
    while (cnt--) {
        bool lp{}, tmp{};
        lp = (bool)(rng() & 1);
        EXPECT_TRUE(unit->enableLPFilter(lp));
        EXPECT_TRUE(unit->isEnabledLPFilter(tmp));
        // M5_LOGI("%u/%u", lp, tmp);
        EXPECT_EQ(lp, tmp);
    }

    cnt = 32;
    while (cnt--) {
        uint8_t level{}, tmp{}, prev{};
        level = rng() & 0x7F;  // 0-127
        // M5_LOGW("lv:%u", level);

        EXPECT_TRUE(unit->readAvgFilterLevel(prev));
        if (level <= 50) {
            EXPECT_TRUE(unit->writeAvgFilterLevel(level));
            EXPECT_TRUE(unit->readAvgFilterLevel(tmp));
            EXPECT_EQ(level, tmp);

        } else {
            EXPECT_TRUE(unit->readAvgFilterLevel(tmp));
            EXPECT_EQ(prev, tmp);
        }
    }
    EXPECT_TRUE(unit->writeAvgFilterLevel(0));
    EXPECT_TRUE(unit->writeAvgFilterLevel(50));
    EXPECT_FALSE(unit->writeAvgFilterLevel(51));

    cnt = 32;
    while (cnt--) {
        uint8_t alpha{}, tmp{}, prev{};
        alpha = rng() & 0x7F;
        // M5_LOGW("alpha:%u", alpha);

        EXPECT_TRUE(unit->readEmaFilterAlpha(prev));
        if (alpha <= 99) {
            EXPECT_TRUE(unit->writeEmaFilterAlpha(alpha));
            EXPECT_TRUE(unit->readEmaFilterAlpha(tmp));
            EXPECT_EQ(alpha, tmp);
        } else {
            EXPECT_TRUE(unit->readEmaFilterAlpha(tmp));
            EXPECT_EQ(prev, tmp);
        }
    }
    EXPECT_TRUE(unit->writeEmaFilterAlpha(0));
    EXPECT_TRUE(unit->writeEmaFilterAlpha(99));
    EXPECT_FALSE(unit->writeEmaFilterAlpha(100));
}

TEST_P(TestWeightI2C, Singleshot)
{
    SCOPED_TRACE(ustr);
    Data dd{};
    char dt[16]{};

    EXPECT_FALSE(unit->measureSingleshot(dd, Mode::Float));
    EXPECT_FALSE(unit->measureSingleshot(dd, Mode::Int));
    EXPECT_FALSE(unit->measureSingleshot(dt));

    EXPECT_TRUE(unit->stopPeriodicMeasurement());

    uint32_t cnt{16};
    while (cnt--) {
        for (auto&& m : mode_table) {
            Data d{};
            EXPECT_TRUE(unit->measureSingleshot(d, m)) << (int)m;
            if (m == Mode::Float) {
                EXPECT_TRUE(std::isfinite(d.weight()));
                EXPECT_EQ(d.iweight(), std::numeric_limits<int32_t>::min());
            } else {
                EXPECT_FALSE(std::isfinite(d.weight()));
            }
        }
        char txt[16]{};
        EXPECT_TRUE(unit->measureSingleshot(txt));
        EXPECT_NE(txt[0], '\0');
    }
}

TEST_P(TestWeightI2C, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& m : mode_table) {
        EXPECT_TRUE(unit->startPeriodicMeasurement(m));
        EXPECT_TRUE(unit->inPeriodic());

        test_periodic_measurement(unit.get(), 8, 1);

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_TRUE(unit->full());
        EXPECT_FALSE(unit->empty());
        EXPECT_EQ(unit->available(), 8U);

        uint32_t cnt{4};
        while (unit->available() && cnt--) {
            if (m == Mode::Float) {
                EXPECT_TRUE(std::isfinite(unit->weight()));
                EXPECT_EQ(unit->iweight(), std::numeric_limits<int32_t>::min());
            } else {
                EXPECT_FALSE(std::isfinite(unit->weight()));
            }
            unit->discard();
            EXPECT_FALSE(unit->empty());
            EXPECT_FALSE(unit->full());
        }

        EXPECT_EQ(unit->available(), 4U);
        unit->flush();

        EXPECT_EQ(unit->available(), 0U);
        EXPECT_FALSE(unit->full());
        EXPECT_TRUE(unit->empty());

        EXPECT_FALSE(std::isfinite(unit->weight()));
        EXPECT_EQ(unit->iweight(), std::numeric_limits<int32_t>::min());
    }
}

TEST_P(TestWeightI2C, ChageI2CAddress)
{
    SCOPED_TRACE(ustr);

    uint8_t addr{};

    EXPECT_FALSE(unit->changeI2CAddress(0x07));  // Invalid
    EXPECT_FALSE(unit->changeI2CAddress(0x78));  // Invalid

    // Change to 0x10
    EXPECT_TRUE(unit->changeI2CAddress(0x10));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x10);
    EXPECT_EQ(unit->address(), 0x10);

    // Change to 0x77
    EXPECT_TRUE(unit->changeI2CAddress(0x77));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x77);
    EXPECT_EQ(unit->address(), 0x77);

    // Change to 0x52
    EXPECT_TRUE(unit->changeI2CAddress(0x52));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x52);
    EXPECT_EQ(unit->address(), 0x52);

    // Change to default
    EXPECT_TRUE(unit->changeI2CAddress(UnitWeightI2C::DEFAULT_ADDRESS));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, +UnitWeightI2C::DEFAULT_ADDRESS);
    EXPECT_EQ(unit->address(), +UnitWeightI2C::DEFAULT_ADDRESS);
}
