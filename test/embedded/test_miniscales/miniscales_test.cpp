/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitMiniScales
*/
#include "../weight_template.hpp"
#include <unit/unit_MiniScales.hpp>

using namespace m5::unit;
using namespace m5::unit::googletest;
using namespace m5::unit::miniscales;
using namespace m5::unit::miniscales::command;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<100000U>());

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C,
//                         ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C,
// ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C, ::testing::Values(false));

// For UnitMiniScales-specific testing
class TestMiniScales : public ComponentTestBase<UnitMiniScales, bool> {
protected:
    virtual UnitMiniScales* get_instance() override
    {
        auto ptr = new m5::unit::UnitMiniScales();
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestMiniScales,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestMiniScales, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestMiniScales, ::testing::Values(false));

TEST_P(TestMiniScales, LED)
{
    SCOPED_TRACE(ustr);

    uint32_t cnt{10};

    while (cnt--) {
        uint8_t r = rng() & 0xFF;
        uint8_t g = rng() & 0xFF;
        uint8_t b = rng() & 0xFF;
        uint8_t r2{}, g2{}, b2{};
        uint32_t tmp{};

        auto s = m5::utility::formatString("R:%u G:%u B:%u", r, g, b);
        SCOPED_TRACE(s);

        // r,g,b
        EXPECT_TRUE(unit->writeLEDColor(r, g, b));
        EXPECT_TRUE(unit->readLEDColor(r2, g2, b2));
        EXPECT_TRUE(unit->readLEDColor(tmp));
        EXPECT_EQ(r, r2);
        EXPECT_EQ(g, g2);
        EXPECT_EQ(b, b2);
        EXPECT_EQ(r, (tmp >> 16) & 0xFF);
        EXPECT_EQ(g, (tmp >> 8) & 0xFF);
        EXPECT_EQ(b, (tmp & 0xFF));

        // uin32_t
        lgfx::rgb888_t rgb888(r, g, b);
        EXPECT_TRUE(unit->writeLEDColor((uint32_t)rgb888));
        EXPECT_TRUE(unit->readLEDColor(r2, g2, b2));
        EXPECT_TRUE(unit->readLEDColor(tmp));

        rgb888 = lgfx::rgb888_t(tmp);
        EXPECT_EQ(r, r2);
        EXPECT_EQ(g, g2);
        EXPECT_EQ(b, b2);
        EXPECT_EQ(r, rgb888.R8());
        EXPECT_EQ(g, rgb888.G8());
        EXPECT_EQ(b, rgb888.B8());

        // uint16_t
        lgfx::rgb565_t rgb565(r, g, b);
        EXPECT_TRUE(unit->writeLEDColor((uint16_t)rgb565));
        EXPECT_TRUE(unit->readLEDColor(r2, g2, b2));

        EXPECT_EQ(rgb565.R8(), r2);
        EXPECT_EQ(rgb565.G8(), g2);
        EXPECT_EQ(rgb565.B8(), b2);
        lgfx::rgb565_t rgb565_2(r2, g2, b2);
        EXPECT_EQ((uint32_t)rgb565, (uint32_t)rgb565_2);
    }

    EXPECT_TRUE(unit->writeLEDColor(0, 0, 0));
}

TEST_P(TestMiniScales, Button)
{
    SCOPED_TRACE(ustr);

    bool press{};
    EXPECT_TRUE(unit->readButtonStatus(press));
    EXPECT_FALSE(press);

    unit->update();
    EXPECT_FALSE(unit->wasPressed());
    EXPECT_FALSE(unit->wasReleased());
}
