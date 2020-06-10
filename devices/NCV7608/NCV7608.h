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

#ifndef EP_OC_MCU_DEVICES_NCV7608_NCV7608_H_
#define EP_OC_MCU_DEVICES_NCV7608_NCV7608_H_

#include "drivers/SPI.h"
#include "drivers/DigitalOut.h"

#include "platform/PlatformMutex.h"

namespace ep {

/**
 * The NCV7608 is an automotive-grade, configurable,
 * octal low-side/high-side driver that can be controlled using a
 * 16-bit SPI bus.
 *
 * The NCV7608 has built-in protection, including flyback diodes, ESD,
 * over-temperature, and over-current. Each failure mode can be diagnosed
 * through the SPI bus interface.
 *
 * The built-in protection features and configurable HS/LS outputs
 * make the NCV7608 ideal for driving resistive (eg: indicator lamps)
 * as well as inductive (eg: relays, small solenoids, DC motors, stepper motors)
 * loads. It is possible to configure the NCV7608 as an H-bridge driver
 *
 */
class NCV7608 {

public:

    /**
     * Important notes on NCV7608 fault detection:
     * - Open load faults can only be detected with the channel OFF
     * - Thermal warning is a global bit, so if a channel activates the thermal
     * warning bit while another channel is exhibiting a different fault, the
     * latter channel will also be reported as having triggered the thermal fault
     */
    typedef enum {
        NO_FAULT,           /** No fault condition */
        OPEN_LOAD,          /** Open load condition exists on channel */
        OVER_CURRENT,       /** Over-current condition exists on channel */
        THERMAL_FAULT,      /** Thermal fault (includes thermal warning/shutdown) */
        VS_POWER_FAIL,      /** Supply voltage power failure detected */
    } fault_condition_t;

    /**
     * Convenience class similar to DigitalOut
     */
    class ChannelOut {

    public:

        /**
         * Construct a ChannelOut
         * @param[in] ncv7608 Parent ncv7608 instance
         * @param[in] num Channel number
         *
         * @note the preferred method create a ChannelOut is
         * to use ::channel on the given NCV7608 instance
         */
        ChannelOut(NCV7608& ncv7608, int num);

        /** Set the output off or on, specified as 0 or 1 (int), respectively
         *
         *  @param value An integer specifying the pin output value,
         *      0 for logical 0, 1 (or any other non-zero value) for logical 1
         */
        void write(int value);

        /** Return the output setting, represented as 0 or 1 (int)
         *
         *  @returns
         *    an integer representing the output setting of the pin,
         *    0 for logical 0, 1 for logical 1
         */
        int read();

        /**
         * Gets the fault condition of the channel
         *
         * @note this does not check the PWM input status bits
         *
         * @note If a fault is reported on a channel, you
         * must typically disable the channel and then re-enable
         * the channel to reset the fault condition. This will only
         * work if the cause of the fault is also removed.
         */
        fault_condition_t get_fault(void);

        /**
         * Enables the open load diagnostics on this channel
         */
        void enable_open_load_diag(void);

        /**
         * Disables the open load diagnostics on this channel
         */
        void disable_open_load_diag(void);

        /**
         * Checks if open load diagnostics are enabled on this channel
         */
        bool open_load_diag_enabled(void);

        /**
         * Set the output to on
         */
        inline void on(void) {
            write(1);
        }

        /**
         * Set the output to off
         */
        inline void off(void) {
            write(0);
        }

        /**
         * Returns true if current state of channel is on
         */
        inline bool is_on(void) {
            return (read() == 1);
        }

        /**
         * Returns true if current state of channel is off
         */
        inline bool is_off(void) {
            return (read() == 0);
        }

        /** A shorthand for write()
         * \sa ChannelOut::write()
         * @code
         *      ChannelOut led(LED1);
         *      led = 1;   // Equivalent to led.write(1)
         * @endcode
         */
        ChannelOut &operator=(int value) {
            // Underlying write is thread safe
            write(value);
            return *this;
        }

        /** A shorthand for write() using the assignment operator which copies the
         * state from the DigitalOut argument.
         * \sa DigitalOut::write()
         */
        ChannelOut &operator=(ChannelOut &rhs) {
            write(rhs.read());
            return *this;
        }

        /** A shorthand for read()
         * \sa DigitalOut::read()
         */
        operator int() {
            // Underlying call is thread safe
            return read();
        }

    protected:
        NCV7608& _parent; /** Parent NCV7608 of this channel object */
        int _num; /** Channel number of this object (1-8) */

    };

public:

    /** Allow ChannelOut to access internal members */
    friend class ChannelOut;

    /**
     * Instantiate an NCV7608 driver
     * @param[in] spi SPI bus instance to use for communication (16-bit format!)
     * @param[in] csb Chip select "bar", defaults to NC (CS handled by SPI in this case)
     * @param[in] global_en Global enable pin, defaults to NC (unused)
     *
     * @note The SPI bus instance used must be configured for 16-bit format
     * to work properly!
     */
    NCV7608(mbed::SPI& spi, PinName csb = NC, PinName global_en = NC);

    /**
     * Instantiate an NCV7608 driver
     * @param[in] spi SPI bus instance to use for communication (16-bit format!)
     * @param[in] csb Chip select "bar" DigitalOut object
     * @param[in] global_en Global enable pin, DigitalOut object
     *
     * @note The SPI bus instance used must be configured for 16-bit format
     * to work properly!
     */
    NCV7608(mbed::SPI& spi, mbed::DigitalOut* csb,
            mbed::DigitalOut* global_en);

    /**
     * Destructor
     */
    ~NCV7608(void);

    /**
     * Globally enable (if global_en != NC)
     */
    void enable(void);

    /**
     * Globally disable (if global_en != NC)
     */
    void disable(void);

    /**
     * Convenience function to create a ChannelOut object for a given
     * channel
     *
     * @param[in] num Desired channel number. Allowed values: 1 thru 8
     * @retval channel Reference to channel
     */
    ChannelOut channel(int num);

    /**
     * Batch writes channel settings to the NCV7608
     *
     * If your application requires closely-timed output transitions,
     * this function ensures the output states are updated in the same
     * SPI transaction.
     *
     * This is useful if, for example, you are using two outputs in
     * parallel to achieve a higher current capacity or you are
     * using multiple outputs in a motor control application.
     *
     * @param[in] new_state Each desired channel state is represented by
     * a bit in the MSB of this number. The bit corresponds to channel ((15 - bit_pos) + 1).
     * (eg: bit 15 represents the desired state of channel 1, 1 = on, 0 = off)
     * Each bit in the LSB of this number represents whether open-load diagnostics are
     * desired on the given channel. 0 = not enabled, 1 = enabled.
     * Defaults to disabled.
     *
     * @retval diag_bits 16-bit output from NCV7608 representing the diagnostics
     * state of each channel. If you are using the ChannelOut API there are
     * convenience functions to interpret this information for you.
     *
     * @note The open-load diagnostics only work with the channel off. Due
     * to the way they work, enabling open-load diagnostics may sink enough
     * current to dimly illuminate LED loads. This is why it defaults to off.
     */
    uint16_t batch_write(uint16_t new_state);

    /**
     * Returns the cached channel state
     */
    uint16_t get_cached_state(void) {
        return _cached_state;
    }

protected:

    /**
     * Asserts the chip select line, if separate from SPI instance
     */
    void assert_cs(void);

    /**
     * Deasserts the chip select line, if separate from SPI instance
     */
    void deassert_cs(void);

    /**
     * Returns the cached diagnostics bits
     */
    uint16_t get_cached_diag(void) {
        return _cached_diag;
    }

    /**
     * Sync the cached state and diagnostic bits
     */
    uint16_t sync(void);

protected:

    mbed::SPI& _spi;
    mbed::DigitalOut* _cs;
    mbed::DigitalOut* _global_en;

    uint16_t _cached_state; /** Cached channel on/off state bits */
    uint16_t _cached_diag;  /** Cached diagnostics bits */

    PlatformMutex _mutex;

    bool _owns_csb;  /** Flag if this object owns/created the CSB output object */
    bool _owns_gen;  /** Flag if this object owns/created the global en output object */

};

}

#endif /* EP_OC_MCU_DEVICES_NCV7608_NCV7608_H_ */
