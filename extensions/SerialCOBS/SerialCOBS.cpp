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

#include "SerialCOBS.h"

#include "platform/ScopedLock.h"
#include "platform/mbed_thread.h"

#include "cobsr.h"

SerialCOBS::SerialCOBS(mbed::FileHandle& fh) : _fh(fh), _staging_index(0) {
}

ssize_t SerialCOBS::write(const void* buffer, size_t size) {

    char txbuf[MBED_CONF_SERIALCOBS_TXBUF_SIZE];

    /** Mutex will be unlocked at the end of this variable's scope */
    mbed::ScopedLock<PlatformMutex> lock(_mutex);

    /** Encode the buffer with COBS */
    cobsr_encode_result result = cobsr_encode(txbuf,
            MBED_CONF_SERIALCOBS_TXBUF_SIZE-1, buffer, size);

    if(result.status == COBSR_ENCODE_NULL_POINTER) {
        return -EINVAL;
    }

    if(result.status == COBSR_ENCODE_OUT_BUFFER_OVERFLOW) {
        return -EOVERFLOW;
    }

    /** Set the delimiter */
    txbuf[result.out_len] = '\0';

    /** Pass on to given file handle */
    size_t total_size = result.out_len+1;
    size_t write_result = _fh.write(txbuf, total_size);

    /** Propagate error codes up */
    if(write_result != total_size) {
        return write_result;
    }

    // Return the original size expected by the caller
    return size;
}

ssize_t SerialCOBS::read(void* buffer, size_t size) {

    size_t data_read = 0;

    char *ptr = static_cast<char *>(buffer);

    if (size == 0) {
        return 0;
    }

    _mutex.lock();

    // If empty, attempt to read and decode underlying file handle
    if(_output_buf.empty()) {
        read_and_decode();
    }

    while (_output_buf.empty()) {
        if (!_fh.is_blocking()) {
            _mutex.unlock();
            return -EAGAIN;
        }
        _mutex.unlock();
        thread_sleep_for(1);
        _mutex.lock();
        read_and_decode();
    }

    while (data_read < size && !_output_buf.empty()) {
        _output_buf.pop(*ptr++);
        data_read++;
    }

    _mutex.unlock();

    return data_read;
}

void SerialCOBS::read_and_decode() {

    /** Read underlying file handle into staging buffer */
    char rx_byte;
    _fh.read(&rx_byte, 1);

    // If this is a 0-byte delimeter, stop here and decode
    if(rx_byte == '\0') {
        char decode_buf[MBED_CONF_SERIALCOBS_RXBUF_SIZE];
        cobsr_decode_result result = cobsr_decode(
                decode_buf, MBED_CONF_SERIALCOBS_RXBUF_SIZE,
                _staging_buf, _staging_index);

        if(result.status != COBSR_DECODE_OK) {
            _staging_index = 0;
            return;
        }

        // Push the decoded buffer to the output buffer
        for(unsigned int i = 0; i < result.out_len; i++) {
            _output_buf.push(decode_buf[i]);
        }

        // Reset index
        _staging_index = 0;

    } else {
        // Otherwise, just add it into the staging buffer
        _staging_buf[_staging_index++] = rx_byte;
        if(_staging_index > MBED_CONF_SERIALCOBS_RXBUF_SIZE) {
            // Overflow! TODO - trace
            _staging_index = 0;
        }
    }
}

//short SerialCOBS::poll(short events) const {
//
//    short revents = 0;
//    // Check the Circular Buffer if space available for writing out
//
//    if(!_output_buf.empty()) {
//        revents |= POLLIN;
//    }
//
//    // POLLHUP and POLLOUT, get from underlying file handle
////    short underlying_events = _fh.poll(events);
////    // Clear the POLLIN flag
////    underlying_events &= ~(POLLIN);
////    // OR underlying events in
////    revents |= underlying_events;
//    // mask with given mask
//    revents &= events;
//
//    return revents;
//
//}
