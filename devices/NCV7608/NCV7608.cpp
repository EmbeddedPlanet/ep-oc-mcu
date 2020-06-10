/**
 * ep-oc-mcu
 * Embedded Planet Open Core for Microcontrollers
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2019-2020 Embedded Planet, Inc.
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
 *
 */

#include "NCV7608.h"

#include "mbed_assert.h"

#define NCV7608_TW_BIT (1 << 15)
#define NCV7608_VS_DIAG_BIT (1 << 0)

using namespace ep;

NCV7608::NCV7608(mbed::SPI& spi, PinName csb, PinName global_en) :
        _spi(spi), _cs(nullptr), _global_en(nullptr), _cached_state(0),
        _cached_diag(0), _owns_csb(false), _owns_gen(false) {

    // Instantiate optional control outputs
    if (csb != NC) {
        _cs = new mbed::DigitalOut(csb, 1);
        _owns_csb = true;
    }

    if (global_en != NC) {
        // Disabled by default
        _global_en = new mbed::DigitalOut(global_en, 0);
        _owns_csb = true;
    }
}

NCV7608::NCV7608(mbed::SPI& spi, mbed::DigitalOut* csb,
        mbed::DigitalOut* global_en) : _spi(spi), _cs(csb), _global_en(global_en),
                _cached_state(0), _cached_diag(0), _owns_csb(false), _owns_gen(false) {
}

NCV7608::~NCV7608(void) {
    // Clean up any dynamically allocated members
    if (_cs != nullptr && _owns_csb) {
        delete _cs;
    }

    if (_global_en != nullptr && _owns_gen) {
        delete _global_en;
    }
}

void NCV7608::enable(void) {
    if (_global_en != nullptr) {
        *_global_en = 1;
    }

    // Turn off all channels initially
    batch_write(0);
}

void NCV7608::disable(void) {
    if (_global_en != nullptr) {
        *_global_en = 0;
    }
}

void ep::NCV7608::assert_cs(void) {
    if (_cs != nullptr) {
        *_cs = 0;
    }
}

void ep::NCV7608::deassert_cs(void) {
    if (_cs != nullptr) {
        *_cs = 1;
    }
}

NCV7608::ChannelOut NCV7608::channel(int num) {
    return ChannelOut(*this, num);
}

uint16_t NCV7608::batch_write(uint16_t new_state) {

    // Lock the SPI mutex before asserting CS
    _spi.lock();

    assert_cs();

    _cached_state = new_state;
    _cached_diag = _spi.write(_cached_state);

    deassert_cs();

    // Unlock the SPI mutex
    _spi.unlock();

    return _cached_diag;
}

uint16_t NCV7608::sync(void) {
    return batch_write(_cached_state);
}

NCV7608::ChannelOut::ChannelOut(NCV7608& ncv7608, int channel_num) :
        _parent(ncv7608), _num(channel_num - 1) {
    // Only channels 1 through 8 are supported
    MBED_ASSERT((0 < channel_num) && (channel_num <= 8));
}

void NCV7608::ChannelOut::write(int value) {

    // Lock the device mutex
    _parent._mutex.lock();

    // Get the cached channel states
    uint16_t new_state = _parent.get_cached_state();

    // Set or clear the channel as indicated by value
    if (value) {
        // Set the channel enable bit
        new_state |= (0x8000 >> _num);
    } else {
        // Clear the channel enable bit
        new_state &= ~(0x8000 >> _num);
    }

    // Write out the value
    _parent.batch_write(new_state);

    // Unlock the device mutex
    _parent._mutex.unlock();

}

int NCV7608::ChannelOut::read() {
    return (_parent.get_cached_state() & (0x8000 >> _num));
}

NCV7608::fault_condition_t NCV7608::ChannelOut::get_fault(void) {

    // Lock the device mutex
    _parent._mutex.lock();

    // Sync diagnostic bits
    uint16_t diag_bits = _parent.sync();

    // Unlock the device mutex
    _parent._mutex.unlock();

    // First see if there's a fault reported on this channel
    if (!(diag_bits & (0x4000 >> _num))) {
        // No fault, return here
        return NO_FAULT;
    }

    // Okay, fault bit is set. Determine what caused it

    /**
     * Open load fault can only be detected when the channel is off
     * and the open load detection enable bit is set
     */
    if (this->is_off() && this->open_load_diag_enabled()) {
        return OPEN_LOAD;
    }

    /** Thermal fault is indicated globally, so check if that bit is set
     */
    if (diag_bits & NCV7608_TW_BIT) {
        return THERMAL_FAULT;
    }

    /** Otherwise, it must be an over-current fault
     */
    return OVER_CURRENT;
}

void NCV7608::ChannelOut::enable_open_load_diag(void) {

    // Lock the device mutex
    _parent._mutex.lock();

    // Get the cached channel states
    uint16_t new_state = _parent.get_cached_state();

    // Set the channel ol enable bit
    new_state |= (0x80 >> _num);

    // Write out the value
    _parent.batch_write(new_state);

    // Unlock the device mutex
    _parent._mutex.unlock();
}

void NCV7608::ChannelOut::disable_open_load_diag(void) {

    // Lock the device mutex
    _parent._mutex.lock();

    // Get the cached channel states
    uint16_t new_state = _parent.get_cached_state();

    // Clear the channel ol enable bit
    new_state &= ~(0x80 >> _num);

    // Write out the value
    _parent.batch_write(new_state);

    // Unlock the device mutex
    _parent._mutex.unlock();

}

bool NCV7608::ChannelOut::open_load_diag_enabled(void) {
    return (_parent.get_cached_state() & (0x80 >> _num));
}

