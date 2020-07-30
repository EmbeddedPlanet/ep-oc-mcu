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

#ifndef SERIALCOBS_H_
#define SERIALCOBS_H_

#include "platform/FileHandle.h"
#include "platform/PlatformMutex.h"
#include "platform/CircularBuffer.h"

#ifndef MBED_CONF_SERIALCOBS_RXBUF_SIZE
#define MBED_CONF_SERIALCOBS_RXBUF_SIZE  256
#endif

#ifndef MBED_CONF_SERIALCOBS_TXBUF_SIZE
#define MBED_CONF_SERIALCOBS_TXBUF_SIZE  256
#endif

/**
 * Encodes and decodes a given serial stream using COBS
 */
class SerialCOBS : public mbed::FileHandle
{

public:

    /**
     * Instantiate a SerialCOBS instance
     * @param[in] fh FileHandle to wrap with COBS encoding/decoding
     */
    SerialCOBS(mbed::FileHandle& fh);

    ssize_t write(const void *buffer, size_t size) override;

    ssize_t read(void *buffer, size_t size) override;

    off_t seek(off_t offset, int whence = SEEK_SET) override
    {
        /* Seeking is not support by this file handler */
        return _fh.seek(offset, whence);
    }

    off_t size() override
    {
        /* Size is not defined for this file handle */
        return _fh.size();
    }

    int isatty() override
    {
        /* File handle is used for terminal output */
        return _fh.isatty();
    }

    int close() override
    {
        return _fh.close();
    }

    int sync() override
    {
        return _fh.sync();
    }

    off_t tell() override
    {
        return _fh.tell();
    }

    void rewind() override
    {
        return _fh.rewind();
    }

    int truncate(off_t length) override
    {
        return _fh.truncate(length);
    }

    int enable_input(bool enabled) override
    {
        return _fh.enable_input(enabled);
    }

    int enable_output(bool enabled) override
    {
        return _fh.enable_output(enabled);
    }

    short poll(short events) const override
    {
        return _fh.poll(events);
    }

    void sigio(mbed::Callback<void()> func) override
    {
        _fh.sigio(func);
    }

    /** Set blocking or non-blocking mode
     *  The default is blocking.
     *
     *  @param blocking true for blocking mode, false for non-blocking mode.
     */
    int set_blocking(bool blocking) override
    {
        return _fh.set_blocking(blocking);
    }

    /** Check current blocking or non-blocking mode for file operations.
     *
     *  @return true for blocking mode, false for non-blocking mode.
     */
    bool is_blocking() const override
    {
        return _fh.is_blocking();
    }

protected:

    void read_and_decode(void);

protected:

    /** Internal file handle that is wrapped with COBS encoding/decoding */
    mbed::FileHandle& _fh;

    /** Software serial buffers
     *  By default buffer size is 256 for TX and 256 for RX. Configurable
     *  through mbed_app.json
     */

    /** Staging buffer for reading in complete COBS-encoded packets */
    char _staging_buf[MBED_CONF_SERIALCOBS_RXBUF_SIZE];

    /** Staging buffer write index */
    size_t _staging_index;

    /** Output buffer to pass on decoded COBS packets */
    mbed::CircularBuffer<char, MBED_CONF_SERIALCOBS_RXBUF_SIZE> _output_buf;

    /** Mutex */
    PlatformMutex _mutex;

};

#endif /* SERIALCOBS_H_ */
