/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitWeightI2C
*/
#include "../weight_template.hpp"

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<100000U>());

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C,
//                         ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C,
// ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestWeightI2C, ::testing::Values(false));
