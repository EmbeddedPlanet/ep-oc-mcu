/**
 * ep-oc-mcu
 * Embedded Planet Open Core for Microcontrollers
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2019-2021 Embedded Planet, Inc.
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

#ifndef EP_OC_MCU_EXTENSIONS_DSP_MOVINGAVERAGEFILTER_H_
#define EP_OC_MCU_EXTENSIONS_DSP_MOVINGAVERAGEFILTER_H_

#include <assert.h>

#include "platform/Span.h"

template<typename T, unsigned int Size>
class MovingAverageFilter
{

public:

    MovingAverageFilter(T initial_value) : _index(0) {
        _running_avg = initial_value;
        for(int i = 0; i < Size; i++) {
            _state[i] = initial_value/Size;
        }
    }

    MovingAverageFilter(mbed::Span<const T, Size> initial_values) : _index(0) {
        for(int i = 0; i < Size; i++) {
            _state[i] = initial_values[i]/Size;
            _running_avg += _state[i];
        }
    }

    /**
     * Push a single value into the MovingAverageFilter.
     * @param[in] val Value to average into the filter
     * @retval Updated running average
     */
    T push(T val) {
        _running_avg -= _state[_index];
        _state[_index] = val/Size;
        _running_avg += _state[_index];
        _index++;
        if(_index >= Size) {
            _index = 0;
        }
        return _running_avg;
    }

    /**
     * Push multiple values into the MovingAverageFilter, and get multiple
     * output values.
     *
     * @param[in] input Input array of values
     * @param[out] output Output array of values
     *
     * @note The output span must have a size equal to or larger than the input span
     */
    void push(mbed::Span<const T> input, mbed::Span<T> output) {

        assert(output.size() >= input.size());

        for(int i = 0; i < input.size(); i++) {
            output[i] = push(input[i]);
        }

    }

    T get_running_average() const {
        return _running_avg;
    }

protected:

    unsigned int _index;

    T _state[Size];

    T _running_avg;


};



#endif /* EP_OC_MCU_EXTENSIONS_DSP_MOVINGAVERAGEFILTER_H_ */
