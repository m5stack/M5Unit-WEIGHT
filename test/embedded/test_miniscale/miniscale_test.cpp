/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitMiniscale
*/
#include "../weight_template.hpp"
#include <unit/unit_Miniscale.hpp>

using namespace m5::unit;
using namespace m5::unit::googletest;
using namespace m5::unit::miniscale;
using namespace m5::unit::miniscale::command;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<100000U>());

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C,
//                         ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C,
// ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C, ::testing::Values(false));

// For UnitMiniscale-specific testing
class TestMiniscale : public ComponentTestBase<UnitMiniscale, bool> {
protected:
    virtual UnitMiniscale* get_instance() override
    {
        auto ptr = new m5::unit::UnitMiniscale();
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

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestMiniscale,
//                          ::testing::Values(false, true));
//  INSTANTIATE_TEST_SUITE_P(ParamValues, TestMiniscale, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestMiniscale, ::testing::Values(false));

TEST_P(TestMiniscale, LED)
{
    SCOPED_TRACE(ustr);
#if 0
    uint32_t cnt{1};
    while (cnt--) {
        uint8_t r = rng() & 0xFF;
        uint8_t g = rng() & 0xFF;
        uint8_t b = rng() & 0xFF;
        uint8_t r2{}, g2{}, b2{};
        uint32_t tmp{};

        //
        EXPECT_TRUE(unit->writeLEDColor(r, g, b));
        EXPECT_TRUE(unit->readLEDColor(r2, g2, b2));
        EXPECT_TRUE(unit->readLEDColor(tmp));
        EXPECT_EQ(r, r2);
        EXPECT_EQ(g, g2);
        EXPECT_EQ(b, b2);
        EXPECT_EQ(r, (tmp >> 16) & 0xFF);
        EXPECT_EQ(g, (tmp >> 8) & 0xFF);
        EXPECT_EQ(b, (tmp & 0xFF));

        //
        uint32_t rgb32 = (r << 16) | (g << 8) | b;
        EXPECT_TRUE(unit->writeLEDColor(rgb32));
        EXPECT_TRUE(unit->readLEDColor(r2, g2, b2));
        EXPECT_TRUE(unit->readLEDColor(tmp));
        EXPECT_EQ(r, r2);
        EXPECT_EQ(g, g2);
        EXPECT_EQ(b, b2);
        EXPECT_EQ(r, (tmp >> 16) & 0xFF);
        EXPECT_EQ(g, (tmp >> 8) & 0xFF);
        EXPECT_EQ(b, (tmp & 0xFF));

        //
        uint16_t rgb16 = (((((rgb32 >> 19) & 0x1F) << 6) + ((rgb32 >> 10) & 0x3F)) << 5) + ((rgb32 >> 3) & 0x1F);
        EXPECT_TRUE(unit->writeLEDColor(rgb16));
        EXPECT_TRUE(unit->readLEDColor(r2, g2, b2));
        EXPECT_TRUE(unit->readLEDColor(tmp));
        uint16_t tmp16 = (((((tmp >> 19) & 0x1F) << 6) + ((tmp >> 10) & 0x3F)) << 5) + ((tmp >> 3) & 0x1F);
        EXPECT_EQ(rgb16, tmp16);
        uint8_t r3 = ((tmp >> 19) & 0x1F) << 6;
        uint8_t g3 = ((tmp >> 10) & 0x3F) << 5;
        uint8_t b3 = (tmp >> 3) & 0x1F;
        EXPECT_EQ(r2, r3);
        EXPECT_EQ(g2, g3);
        EXPECT_EQ(b2, b3);
    }
#endif
}

TEST_P(TestMiniscale, Button)
{
    SCOPED_TRACE(ustr);
}
