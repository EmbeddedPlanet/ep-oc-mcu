/*
 * BleSerialTestServer.h
 *
 *  Created on: Oct 21, 2020
 *      Author: gdbeckstein
 */

#ifndef BLESERIALTESTSERVER_H_
#define BLESERIALTESTSERVER_H_

#include "utest/utest.h"
#include "unity/unity.h"

#include "events/EventQueue.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "gap/ChainableGapEventHandler.h"
#include "gatt/ChainableGattServerEventHandler.h"

#include "UARTService.h"

static const uint16_t MAX_ADVERTISING_PAYLOAD_SIZE = 50;
static const char DEVICE_NAME[] = "TestUART";

using namespace utest::v1;

using namespace ble;

class BleSerialTestServer : public Gap::EventHandler,
                      public GattServer::EventHandler
{

public:

    BleSerialTestServer() :
        _ble_interface(BLE::Instance()),
        _gap(_ble_interface.gap()),
        _gatt_srv(_ble_interface.gattServer()),
        _adv_data_builder(_adv_buffer),
        _adv_handle(ble::LEGACY_ADVERTISING_HANDLE) {

    }

    virtual ~BleSerialTestServer() {

    }

    /**
     * Initialize the ble interface, configure it and start advertising.
     */
    void start() {

        /* Problem with test configuration if we reinitilize... */
        TEST_ASSERT(_ble_interface.hasInitialized());

        /* Setup event handlers */
        gap_eh_chain.addEventHandler(this);
        gatt_srv_eh_chain.addEventHandler(this);
        gap_eh_chain.addEventHandler(&uart_svc);
        gatt_srv_eh_chain.addEventHandler(&uart_svc);
        _gap.setEventHandler(&gap_eh_chain);
        _gatt_srv.setEventHandler(&gatt_srv_eh_chain);

        _ble_interface.onEventsToProcess(
                makeFunctionPointer(this,  &BleSerialTestServer::schedule_ble_events));

        ble_error_t error = _ble_interface.init(
                this, &BleSerialTestServer::on_init_complete);

        TEST_ASSERT_EQUAL(BLE_ERROR_NONE, error);

        _event_queue.dispatch_forever();

        // We should never reach this
        TEST_ASSERT(1);

    }

    void on_ready(mbed::Callback<void(void)> ready_cb) {
        _ready_cb = ready_cb;
    }

    const char* get_mac_addr_and_type(void) {
        return _ble_mac_addr_and_type;
    }

private:

    void on_init_complete(BLE::InitializationCompleteCallbackContext *event) {

        TEST_ASSERT_EQUAL(BLE_ERROR_NONE, event->error);

        uart_svc.start(_ble_interface);

        _event_queue.call(this, &BleSerialTestServer::start_advertising);

    }

    void start_advertising() {

        ble_error_t error;

        ble::AdvertisingParameters adv_params;

        error = _gap.setAdvertisingParameters(_adv_handle,  adv_params);

        TEST_ASSERT_EQUAL(BLE_ERROR_NONE, error);

        _adv_data_builder.clear();
        _adv_data_builder.setFlags();
        _adv_data_builder.setName(DEVICE_NAME);

        error = _gap.setAdvertisingPayload(
                _adv_handle, _adv_data_builder.getAdvertisingData());

        TEST_ASSERT_EQUAL(BLE_ERROR_NONE, error);

        error = _gap.startAdvertising(_adv_handle);

        TEST_ASSERT_EQUAL(BLE_ERROR_NONE, error);

        // Populate the BLE MAC address string
        sprint_mac_address_and_type(_ble_mac_addr_and_type);

        if(_ready_cb) {
            _ready_cb();
        }

    }

    void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *event) {
        _event_queue.call(mbed::callback(&event->ble, &BLE::processEvents));
    }

    /**
     * Prints our own BLE MAC Address into given dest buffer
     * @param[in] dest Destination buffer. Must hold at least 18 bytes
     */
    inline void sprint_mac_address_and_type(char* dest) {

        /* Print out device MAC address to the destination buffer */
        ble::own_address_type_t addr_type;
        ble::address_t addr;
        BLE::Instance().gap().getAddress(addr_type, addr);
        snprintf(dest, 32, "%02x:%02x:%02x:%02x:%02x:%02x,%u",
                addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
                addr_type.value());

    }

public:

    ChainableGapEventHandler gap_eh_chain;
    ChainableGattServerEventHandler gatt_srv_eh_chain;

    UARTService uart_svc;

private:

    events::EventQueue _event_queue;
    BLE &_ble_interface;
    ble::Gap &_gap;
    ble::GattServer &_gatt_srv;

    uint8_t _adv_buffer[MAX_ADVERTISING_PAYLOAD_SIZE];

    ble::AdvertisingDataBuilder _adv_data_builder;
    ble::advertising_handle_t _adv_handle;

    // Callback to be executed when BLE system is ready for test
    mbed::Callback<void(void)> _ready_cb = nullptr;

    char _ble_mac_addr_and_type[32];

};



#endif /* BLESERIALTESTSERVER_H_ */
