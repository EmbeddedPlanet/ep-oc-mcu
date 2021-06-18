/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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

#ifndef __BLE_UART_SERVICE_H__
#define __BLE_UART_SERVICE_H__

#include "platform/FileHandle.h"
#include "platform/PlatformMutex.h"
#include "platform/CircularBuffer.h"
#include "platform/NonCopyable.h"
#include "platform/Span.h"
#include "platform/SharedPtr.h"

#include "ble/common/UUID.h"
#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/Gap.h"

#include "rtos/Queue.h"

#include <string>

#include <array>

#if BLE_FEATURE_GATT_SERVER

extern const uint8_t  UARTServiceBaseUUID[UUID::LENGTH_OF_LONG_UUID];
extern const uint16_t UARTServiceShortUUID;
extern const uint16_t UARTServiceTXCharacteristicShortUUID;
extern const uint16_t UARTServiceRXCharacteristicShortUUID;

extern const uint8_t  UARTServiceUUID[UUID::LENGTH_OF_LONG_UUID];
extern const uint8_t  UARTServiceUUID_reversed[UUID::LENGTH_OF_LONG_UUID];

extern const uint8_t  UARTServiceTXCharacteristicUUID[UUID::LENGTH_OF_LONG_UUID];
extern const uint8_t  UARTServiceRXCharacteristicUUID[UUID::LENGTH_OF_LONG_UUID];


/**
 * Maximum length of data (in bytes)
 * that the UART service module can transmit/receive
 * to/from the peer at one time.
 *
 * Typically MTU - 3 bytes for overhead
 */
#ifndef MBED_CONF_CORDIO_DESIRED_ATT_MTU
#define BLE_UART_SERVICE_MAX_DATA_LEN MBED_CONF_BLE_UART_SERVICE_BUFFER_SIZE
#else
#define BLE_UART_SERVICE_MAX_DATA_LEN (MBED_CONF_CORDIO_DESIRED_ATT_MTU - 3)
#endif

/**
 * Maximum number of individual BLE serial connections
 * This is usually equal to the maximum number of connections the BLE
 * stack is configured to use.
 *
 * The application can adjust this to save memory.
 */
#ifndef MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS
#ifndef DM_CONN_MAX
#define MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS 3
#else
#define MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS DM_CONN_MAX
#endif
#endif

/**
 * BLE defaults to 23 bytes for MTU size, subtract 3 for overhead and you get 20 bytes
 * per connection interval by default
 */
#define BLE_UART_SERVICE_DEFAULT_MTU_SIZE 20

/**
 * TX and RX Buffer Sizes
 */
#ifndef MBED_CONF_BLE_UART_SERVICE_TX_RX_QUEUE_SIZE
#define MBED_CONF_BLE_UART_SERVICE_TX_RX_BUFFER_SIZE 256
#endif

/**
 * @class UARTService.
 * @brief BLE Service to enable UART over BLE.
 *
 */
class UARTService : public ble::GattServer::EventHandler,
                    public ble::Gap::EventHandler,
                    private mbed::NonCopyable<UARTService> {

public:

    /**
     * Serial representing a transfer for a single
     * BLE connection handle.
     */
    class BleSerial :
            public mbed::FileHandle,
            private mbed::NonCopyable<BleSerial> {

    /** Allow UARTService to construct this class */
    friend UARTService;

    protected:

        BleSerial(UARTService& service,
                ble::connection_handle_t connection_handle);

    public:

        ~BleSerial();

        ssize_t write(const void *_buffer, size_t length) override;

        ssize_t read(void *buffer, size_t size) override;

        off_t seek(off_t offset, int whence = SEEK_SET) override {
            /* Seeking is not support by this file handler */
            return -ESPIPE;
        }

        off_t size() override {
            /* Size is not defined for this file handle */
            return -EINVAL;
        }

        int isatty() override {
            /* File handle is used for terminal output */
            return true;
        }

        int close() override {
            return 0;
        }

        int set_blocking(bool blocking) override {
            _blocking = blocking;
            return 0;
        }

        bool is_blocking() const override {
            return _blocking;
        }

        short poll(short events) const override {
            short result = _rxbuf.empty()? 0 : POLLIN;
            if(_txbuf) {
                result |= _txbuf->full()? 0 : POLLOUT;
            }
            return (result & events);
        }

        void sigio(mbed::Callback<void()> func) override {
            _sigio_cb = func;
        }

        ble::connection_handle_t get_connection_handle(void) {
            return _connection_handle;
        }

        bool operator==(const BleSerial& other) {
            return (_connection_handle == other._connection_handle);
        }

        /**
         * Set the application callback for when updates (notifications/indications)
         * are enabled/disabled.
         */
        void on_updates_changed(mbed::Callback<void(bool)> updates_changed_cb) {
            _updates_changed_cb = updates_changed_cb;
        }

        uint16_t get_mtu(void) {
            return _mtu;
        }

        bool is_shutdown() const {
            return _shutdown;
        }

    protected:

        /** Handler for when updates are enabled by this connection */
        void on_updates_enabled(void);

        /** Handler for when updates are disabled by this connection */
        void on_updates_disabled(void);

        /**
         * Handler for when data has been sent to a client as part
         * of a notification/indication
         */
        void on_data_sent(void);

        /**
         * Handler for when data has been written to the RX characteristic
         * by the associated connection handle
         */
        void on_data_written(mbed::Span<const uint8_t> data);

        /**
         * Deallocates the tx buffer, called upon both disconnection
         * and if the client disables updates
         */
        void deallocate_tx_buffer(void) {
            mutex.lock();
            delete _txbuf;
            _txbuf = nullptr;
            mutex.unlock();
        }

        void wake(void) {
            if(_sigio_cb) {
                _sigio_cb();
            }
        }

        void set_mtu(uint16_t mtu) {
            _mtu = mtu;
        }

        /**
         * Shutdown this BleSerial
         *
         * This could be caused by a disconnection or BLE shutdown
         */
        void shutdown(void);

    protected:

        UARTService& _service;
        ble::connection_handle_t _connection_handle = 0;
        bool _blocking = true;

        mbed::CircularBuffer<uint8_t, MBED_CONF_BLE_UART_SERVICE_TX_RX_BUFFER_SIZE> _rxbuf;

        /** The TX buffer is only allocated if the client subscribes to the characteristic */
        mbed::CircularBuffer<uint8_t, MBED_CONF_BLE_UART_SERVICE_TX_RX_BUFFER_SIZE>* _txbuf = nullptr;

        mbed::Callback<void(void)> _sigio_cb = nullptr;

        /**
         * Application callback for when updates are enabled/disabled on
         * this serial connection handle
         *
         * @param[in] enabled True if updates have been enabled, false if disabled
         */
        mbed::Callback<void(bool)> _updates_changed_cb = nullptr;

        PlatformMutex mutex;

        /** Cached MTU of the connection */
        uint16_t _mtu;

        /** Gatt TX buffer */
        uint8_t *_gatt_tx_buf = nullptr;

        /** Actively writing from tx buffer flag */
        bool _sending_data = false;

        /** Shutdown flag */
        bool _shutdown = false;

    };

public:

    /**
     * Allow BleSerial to access write
     * TODO - is friendship of an internal class intrinsic in C++11?
     */
    friend BleSerial;

    /**
    * @param _ble
    *               BLE object for the underlying controller.
    */
    UARTService();

    virtual ~UARTService();

    virtual void start(BLE &ble_interface);

    /** GattServer::EventHandler overrides */
    void onAttMtuChange(
            ble::connection_handle_t connectionHandle,
            uint16_t attMtuSize) override;

    void onDataSent(const GattDataSentCallbackParams &params) override;

    void onDataWritten(const GattWriteCallbackParams &params) override;

    void onShutdown(const GattServer &server) override;

    void onUpdatesEnabled(const GattUpdatesEnabledCallbackParams &params) override;

    void onUpdatesDisabled(const GattUpdatesDisabledCallbackParams &params) override;

    /** Gap::EventHandler overrides */

    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override;

    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override;

    /**
     * Gets a SharedPtr to a BleSerial with a given connection handle
     *
     *      * @param[in] connection_handle Connection handle to get the associated BleSerial handle of
     * @retval SharedPtr to associated BleSerial object, can be nullptr if not found
     */
    mbed::SharedPtr<BleSerial> get_ble_serial_handle(ble::connection_handle_t connection_handle);

protected:

    void shutdown_all_serial_handles(void);

    ble_error_t write(BleSerial* ser, mbed::Span<const uint8_t> data);

    /**
     * Returns the next available slot in the array of serial handles
     * @retval pointer to a shared pointer if an empty slot is available, nullptr otherwise
     */
    mbed::SharedPtr<BleSerial>* get_next_available_slot(void);

    /**
     * Gets an internal SharedPtr given a connection handle.
     *
     * TODO is it necessary to pass by pointer internally? We want to access the same
     * SharedPtr object that is in our list, not a copy of it...
     *
     * @param[in] connection_handle Connection handle to get the associated BleSerial handle of
     * @retval Pointer to SharedPtr to associated BleSerial object, nullptr if not found
     */
    mbed::SharedPtr<BleSerial>* get_ble_serial_handle_internal(ble::connection_handle_t connection_handle);

protected:

    uint8_t receiveBuffer[BLE_UART_SERVICE_MAX_DATA_LEN]; /**< The local buffer into which we receive
                                                           *   inbound data before forwarding it to the
                                                           *   application. */

    uint8_t sendBuffer[BLE_UART_SERVICE_MAX_DATA_LEN];    /**< The local buffer into which outbound data is
                                                           *   accumulated before being pushed to the
                                                           *   rxCharacteristic. */

    GattCharacteristic  txCharacteristic; /**< From the point of view of the external client, this is the characteristic
                                           *   they'd write into in order to communicate with this application. */
    GattCharacteristic  rxCharacteristic; /**< From the point of view of the external client, this is the characteristic
                                           *   they'd read from in order to receive the bytes transmitted by this
                                           *   application. */

    GattAttribute::Handle_t rxCCCDHandle; /** Cached RX Characteristic CCCD handle */

    GattCharacteristic* _uart_characteristics[2];

    GattService _uart_service;

    GattServer* _server;

    /** Array of serial handles for each connection */
    mbed::SharedPtr<BleSerial> _serial_handles[MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS];

};

#endif // BLE_FEATURE_GATT_SERVER

#endif /* #ifndef __BLE_UART_SERVICE_H__*/
