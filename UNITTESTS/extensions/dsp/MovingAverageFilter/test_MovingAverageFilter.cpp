/**
 * ep-oc-mcu
 * Embedded Planet Open Core for Microcontrollers
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2019 Embedded Planet, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"

#include "extensions/dsp/MovingAverageFilter.h"

#include <string>

float input_data[9] = { 2.0f, 4.0f, 6.0f, 8.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f };

/**
 * Test the MovingAverageFilter
 */
class TestMovingAverageFilter : public testing::Test {

	virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};


/**
 * Test MovingAverageFilter with single initial value
 */
TEST_F(TestMovingAverageFilter, single_iv)
{
    MovingAverageFilter<float, 2> filter(2);
    EXPECT_FLOAT_EQ(filter.push(4), 3.0f);
    EXPECT_FLOAT_EQ(filter.push(4), 4.0f);
    EXPECT_FLOAT_EQ(filter.push(2), 3.0f);
    EXPECT_FLOAT_EQ(filter.push(0), 1.0f);
}

/**
 * Test MovingAverageFilter with multiple initial values
 */
TEST_F(TestMovingAverageFilter, multiple_iv)
{
    MovingAverageFilter<float, 4> filter(mbed::make_const_Span<4, float>(input_data));
    EXPECT_FLOAT_EQ(filter.get_running_average(), 5.0f);
    EXPECT_FLOAT_EQ(filter.push(input_data[4]), 7.5f);
    EXPECT_FLOAT_EQ(filter.push(input_data[5]), 10.0f);
    EXPECT_FLOAT_EQ(filter.push(input_data[6]), 12.5f);
    EXPECT_FLOAT_EQ(filter.push(input_data[7]), 15.0f);
    EXPECT_FLOAT_EQ(filter.push(input_data[8]), 17.0f);
}

/**
 * Test MovingAverageFilter with array input push values
 */
TEST_F(TestMovingAverageFilter, push_multiple)
{
    float output_data[5];
    float expected[5] = { 7.5f, 10.0f, 12.5f, 15.0f, 17.0f };
    MovingAverageFilter<float, 4> filter(mbed::make_const_Span<4, float>(input_data));
    EXPECT_FLOAT_EQ(filter.get_running_average(), 5.0f);
    filter.push(mbed::make_const_Span<5, float>(&input_data[4]),
            mbed::make_Span<float, 5>(output_data));
    for(int i = 0; i < 5; i++) {
        EXPECT_FLOAT_EQ(expected[i], output_data[i]);
    }
}


