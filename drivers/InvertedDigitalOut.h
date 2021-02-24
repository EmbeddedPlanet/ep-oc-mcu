/*
 * Mbed-OS Microcontroller Library
 * Copyright (c) 2020 Embedded Planet
 * Copyright (c) 2020 ARM Limited
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
 * limitations under the License
 */

#ifndef EP_OC_MCU_DRIVERS_INVERTEDDIGITALOUT_H_
#define EP_OC_MCU_DRIVERS_INVERTEDDIGITALOUT_H_

#include "drivers/DigitalOut.h"

#ifndef FEATURE_EXPERIMENTAL_API
#error InvertedDigitalOut requires the base DigitalOut class to be polymorphic. Please add the EXPERIMENTAL API feature.
#endif

namespace ep
{

/**
 * Class that simply inverts the standard DigitalOut operation
 */
class InvertedDigitalOut : public mbed::interface::DigitalOut
{
public:

    InvertedDigitalOut(PinName pin) : _do(pin) {
    }

    InvertedDigitalOut(PinName pin, int value) : _do(pin, !value) {
    }

    virtual ~InvertedDigitalOut() {
    }

    virtual void write(int value) override {
        _do.write(!value);
    }

    virtual int read() override {
        return !_do.read();
    }

    virtual int is_connected() override {
        return _do.is_connected();
    }

    using mbed::interface::DigitalOut::operator =;
    using mbed::interface::DigitalOut::operator int;

protected:

    mbed::DigitalOut _do;

};

} // namespace ep

#endif /* EP_OC_MCU_DRIVERS_INVERTEDDIGITALOUT_H_ */
