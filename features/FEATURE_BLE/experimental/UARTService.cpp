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

/**
 * TODO - test blocking/non-blocking implementations
 * TODO - test MTU updates
 * TODO - write test suite
 * TODO - throughput test
 */

#define BLE_UART_TRACE 0

#if BLE_FEATURE_GATT_SERVER

#include "UARTService.h"

#include "mbed_trace.h"

#include "Kernel.h"

#include "platform/mbed_assert.h"
#include "platform/mbed_thread.h"

#define TRACE_GROUP "btuart"

#define DURATION_ZERO rtos::Kernel::Clock::duration_u32::zero()

const uint8_t  UARTServiceBaseUUID[UUID::LENGTH_OF_LONG_UUID] = {
    0x6E, 0x40, 0x00, 0x00, 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};
const uint16_t UARTServiceShortUUID                 = 0x0001;
const uint16_t UARTServiceTXCharacteristicShortUUID = 0x0002;
const uint16_t UARTServiceRXCharacteristicShortUUID = 0x0003;
const uint8_t  UARTServiceUUID[UUID::LENGTH_OF_LONG_UUID] = {
    0x6E, 0x40, (uint8_t)(UARTServiceShortUUID >> 8), (uint8_t)(UARTServiceShortUUID & 0xFF), 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};
const uint8_t  UARTServiceUUID_reversed[UUID::LENGTH_OF_LONG_UUID] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, (uint8_t)(UARTServiceShortUUID & 0xFF), (uint8_t)(UARTServiceShortUUID >> 8), 0x40, 0x6E
};
const uint8_t  UARTServiceTXCharacteristicUUID[UUID::LENGTH_OF_LONG_UUID] = {
    0x6E, 0x40, (uint8_t)(UARTServiceTXCharacteristicShortUUID >> 8), (uint8_t)(UARTServiceTXCharacteristicShortUUID & 0xFF), 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};
const uint8_t  UARTServiceRXCharacteristicUUID[UUID::LENGTH_OF_LONG_UUID] = {
    0x6E, 0x40, (uint8_t)(UARTServiceRXCharacteristicShortUUID >> 8), (uint8_t)(UARTServiceRXCharacteristicShortUUID & 0xFF), 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};

UARTService::UARTService():
    txCharacteristic(UARTServiceTXCharacteristicUUID, receiveBuffer, 1, BLE_UART_SERVICE_MAX_DATA_LEN,
                     GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE),
    rxCharacteristic(UARTServiceRXCharacteristicUUID, sendBuffer, 1, BLE_UART_SERVICE_MAX_DATA_LEN, (GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)),
    rxCCCDHandle(0),
    _uart_service(UARTServiceUUID,
                _uart_characteristics,
                sizeof(_uart_characteristics)/
                sizeof(_uart_characteristics[0])),
   _server(nullptr) {
    _uart_characteristics[0] = &txCharacteristic;
    _uart_characteristics[1] = &rxCharacteristic;
}

void UARTService::start(BLE &ble_interface)
{
    _server = &ble_interface.gattServer();
    _server->addService(_uart_service);

    // Cache the CCCD handle
    for(int i = 0; i < rxCharacteristic.getDescriptorCount(); i++) {
        GattAttribute* desc = rxCharacteristic.getDescriptor(i);
        if(desc->getUUID() == UUID(0x2902)) {
            rxCCCDHandle = desc->getHandle();
#if BLE_UART_TRACE
            tr_info("uart service cccd handle: %u",
                    rxCCCDHandle);
#endif
        }
    }
}

void UARTService::onAttMtuChange(ble::connection_handle_t connectionHandle,
        uint16_t attMtuSize) {
#if BLE_UART_TRACE
    tr_debug("mtu changed to %u for connection handle %d", attMtuSize, connectionHandle);
#endif
    mbed::SharedPtr<BleSerial> ser = get_ble_serial_handle(connectionHandle);
    if(ser) {
        ser->set_mtu(attMtuSize);
    }
}

void UARTService::onDataWritten(const GattWriteCallbackParams &params)
{
    if(params.handle == txCharacteristic.getValueHandle()) {
        mbed::SharedPtr<BleSerial> ser = get_ble_serial_handle(params.connHandle);
        if(ser) {
            ser->on_data_written(mbed::make_const_Span(params.data, params.len));
        }
    }
}

void UARTService::onDataSent(const GattDataSentCallbackParams &params)
{
    if(params.attHandle == rxCharacteristic.getValueHandle()) {
        mbed::SharedPtr<BleSerial> ser = get_ble_serial_handle(params.connHandle);
        if(ser) {
            ser->on_data_sent();
        }
    }
}

void UARTService::onUpdatesEnabled(
        const GattUpdatesEnabledCallbackParams &params) {
    if(params.attHandle == rxCCCDHandle) {
        mbed::SharedPtr<BleSerial> ser = get_ble_serial_handle(params.connHandle);
        if(ser) {
            ser->on_updates_enabled();
        }
    }
}

void UARTService::onUpdatesDisabled(
        const GattUpdatesDisabledCallbackParams &params) {
    if(params.attHandle == rxCCCDHandle) {
        mbed::SharedPtr<BleSerial> ser = get_ble_serial_handle(params.connHandle);
        if(ser) {
            ser->on_updates_disabled();
        }
    }
}

void UARTService::onConnectionComplete(
        const ble::ConnectionCompleteEvent& event) {

    mbed::SharedPtr<BleSerial> *slot = get_next_available_slot();
    if(slot)
    {
        // Create a new BleSerial handle for this connection
        *slot = new BleSerial(*this, event.getConnectionHandle());
#if BLE_UART_TRACE
        tr_debug("serial handle (+): connection handle: %d", event.getConnectionHandle());
#endif
    } else {
#if BLE_UART_TRACE
        tr_warn("no serial slots available");
#endif
    }
}

void UARTService::onDisconnectionComplete(
        const ble::DisconnectionCompleteEvent& event) {

    // Remove the disconnected serial handle
    mbed::SharedPtr<BleSerial>* ser = get_ble_serial_handle_internal(
            event.getConnectionHandle());

    if(ser) {
        (*ser)->shutdown();
        *ser = nullptr; // Forget the reference and let SharedPtr clean up
#if BLE_UART_TRACE
        tr_debug("serial handle(-): connection handle: %d", event.getConnectionHandle());
#endif
    }
}

UARTService::~UARTService() {
    shutdown_all_serial_handles();
}

void UARTService::onShutdown(const GattServer &server) {
    shutdown_all_serial_handles();
}

void UARTService::shutdown_all_serial_handles(void) {

    for(int i = 0; i < MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS; i++) {
        auto handle = _serial_handles[i];
        if(handle) {
            handle->shutdown();
            handle = nullptr;
        }
    }
}

mbed::SharedPtr<UARTService::BleSerial> UARTService::get_ble_serial_handle(
        ble::connection_handle_t connection_handle) {
    return *(get_ble_serial_handle_internal(connection_handle));
}

mbed::SharedPtr<UARTService::BleSerial>* UARTService::get_ble_serial_handle_internal(
        ble::connection_handle_t connection_handle) {

    for(int i = 0; i < MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS; i++) {
        if(_serial_handles[i]->get_connection_handle() == connection_handle) {
            return &_serial_handles[i];
        }
    }

    // Return nullptr if an associated BleSerial object wasn't found
    return nullptr;
}

mbed::SharedPtr<UARTService::BleSerial>* UARTService::get_next_available_slot(void) {

    for(int i = 0; i < MBED_CONF_BLE_UART_SERVICE_MAX_SERIALS; i++) {
        if(!_serial_handles[i]) {
            return &_serial_handles[i];
        }
    }

    // Return nullptr if a free slot wasn't found
    return nullptr;
}

ble_error_t UARTService::write(BleSerial *ser, mbed::Span<const uint8_t> data) {

    return _server->write(ser->_connection_handle,
            rxCharacteristic.getValueHandle(),
            data.data(), data.size(), false);

}

UARTService::BleSerial::BleSerial(
        UARTService& service,
        ble::connection_handle_t connection_handle) :
                _service(service), _connection_handle(connection_handle),
                _mtu(BLE_UART_SERVICE_DEFAULT_MTU_SIZE) { }

UARTService::BleSerial::~BleSerial() {

    /** Deallocate TX buffer upon disconnection (destructor is called upon disconnection) */
    deallocate_tx_buffer();

    delete[] _gatt_tx_buf;
}

ssize_t UARTService::BleSerial::write(const void *_buffer, size_t length)
{
    // Ignore if the BleSerial is shutdown
    if(_shutdown) {
        return -ESHUTDOWN;
    }

    const uint8_t *buffer = static_cast<const uint8_t*>(_buffer);
    size_t data_written = 0;

    mutex.lock();

    // Ignore if the client hasn't subscribed yet
    if(_txbuf == nullptr) {
        mutex.unlock();
        return -EAGAIN;
    }

    while(data_written < length) {

        if(_txbuf->full()) {
            if(!_blocking) {
                break;
            }
            do {
                if(_shutdown) {
                    mutex.unlock();
                    return -ESHUTDOWN;
                }
                mutex.unlock();
                thread_sleep_for(1);
                // Check if _txbuf was deallocated while waiting
                if(!_txbuf) {
                    return -EAGAIN;
                }
                mutex.lock();

            } while(_txbuf->full());
        }

        while(data_written < length && !_txbuf->full()) {
            _txbuf->push(*buffer++);
            data_written++;
        }

        if(!_sending_data) {
            // Simulate a data sent event to start TX
            on_data_sent();
        }

        // Wait until we're done sending (blocking only)
        while(_sending_data) {
            if(!_blocking) {
                break;
            }

            if(_shutdown) {
                mutex.unlock();
                return -ESHUTDOWN;
            }
            mutex.unlock();
            // Check if _txbuf was deallocated while waiting
            thread_sleep_for(1);
            if(!_txbuf) {
                return -EAGAIN;
            }
            mutex.lock();
        }

    }

    mutex.unlock();

    return data_written != 0 ? (ssize_t) data_written : (ssize_t) - EAGAIN;

}

ssize_t UARTService::BleSerial::read(void *buffer, size_t size)
{
    // Ignore if the BleSerial is shutdown
    if(_shutdown) {
        return -ESHUTDOWN;
    }

    size_t data_read = 0;

    uint8_t *ptr = static_cast<uint8_t *>(buffer);

    if (size == 0) {
        return 0;
    }

    mutex.lock();
    while(_rxbuf.empty()) {
        if(!_blocking) {
            mutex.unlock();
            return -EAGAIN;
        }

        if(_shutdown) {
            mutex.unlock();
            return -ESHUTDOWN;
        }

        mutex.unlock();
        thread_sleep_for(1);
        mutex.lock();
    }

    while(data_read < size && !_rxbuf.empty()) {
        _rxbuf.pop(*ptr++);
        data_read++;
    }

    mutex.unlock();

    return data_read;
}

void UARTService::BleSerial::on_updates_enabled(void) {

    /** Allocate TX buffer now that someone is listening */
    _txbuf = new mbed::CircularBuffer<uint8_t, MBED_CONF_BLE_UART_SERVICE_TX_RX_BUFFER_SIZE>();

    /** Call application handler */
    if(_updates_changed_cb) {
        _updates_changed_cb(true);
    }
#if BLE_UART_TRACE
    tr_debug("updates enabled on connection handle %u", _connection_handle);
#endif
}

void UARTService::BleSerial::on_updates_disabled(void) {

    // TODO - what if the client unsubscribes during a write? We should mutex protect this

    /** Deallocate TX buffer */
    deallocate_tx_buffer();

    /* Call application handler */
    if(_updates_changed_cb) {
        _updates_changed_cb(false);
    }
#if BLE_UART_TRACE
    tr_debug("updates disabled on connection handle %u", _connection_handle);
#endif
}

void UARTService::BleSerial::on_data_sent(void) {

    mutex.lock();

    bool was_full = _txbuf->full();

    // Delete the old gatt buffer and allocate a new one
    delete[] _gatt_tx_buf;
    _gatt_tx_buf = nullptr;

    // Check if the tx buffer is empty or if we're shutdown
    if(_txbuf->empty() || _shutdown) {
        // We're done sending
        _sending_data = false;
        mutex.unlock();
        return;
    }

    _gatt_tx_buf = new uint8_t[_mtu];

    MBED_ASSERT(_gatt_tx_buf != nullptr);

    int i = 0;
    while((i < _mtu) && (!_txbuf->empty())) {
        _txbuf->pop(_gatt_tx_buf[i++]);
    }

    // Transmit the buffer now
    ble_error_t err = _service.write(this, mbed::make_Span(_gatt_tx_buf, i));

    if(err) {
#if BLE_UART_TRACE
        tr_error("writing to connection %d failed: %u", _connection_handle, err);
#endif
        // TODO do we need to set _sending_data to false here?
    } else {
#if BLE_UART_TRACE
        tr_info("wrote %d bytes to connection %d", i, _connection_handle);
#endif
        _sending_data = true;
        if(was_full && !_txbuf->full()) {
            wake();
        }
    }

    mutex.unlock();
}

void UARTService::BleSerial::on_data_written(mbed::Span<const uint8_t> data) {

    mutex.lock();

    bool was_empty = _rxbuf.empty();

    for(int i = 0; i < data.size(); i++) {
        _rxbuf.push(data[i]);
    }

    // Report that data is ready to be read from the buffer
    if(was_empty && !_rxbuf.empty()) {
        wake();
    }

    mutex.unlock();
}

void UARTService::BleSerial::shutdown(void) {
    // TODO need mutex here?
    _shutdown = true;
}

#endif // BLE_FEATURE_GATT_SERVER
