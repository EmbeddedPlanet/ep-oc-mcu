/*
 * Copyright (c) 2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#if !MBED_TEST_MODE

#include "rtos/Thread.h"
#include "rtos/ThisThread.h"
#include "rtos/EventFlags.h"

#include "platform/SharedPtr.h"

#include "BleSerialTestServer.h"

#include "greentea-client/test_env.h"
#include "unity/unity.h"

#include <chrono>

#define DEFAULT_TIMEOUT 30s
#define READ_WRITE_DELAY 0

#define TEST_ASSERT_NO_TIMEOUT(x) TEST_ASSERT(!(x & osFlagsError))

#define EVENT_FLAGS_BLE_READY           0x0001
#define EVENT_FLAGS_BLE_CONNECTED       0x0002
#define EVENT_FLAGS_BLE_DISCONNECTED    0x0004

using namespace std::chrono;
using namespace utest::v1;

static rtos::EventFlags flags;
static BleSerialTestServer test_srv;
static rtos::Thread ble_thread(osPriorityNormal,
        MBED_CONF_RTOS_THREAD_STACK_SIZE,
        NULL, "ble-thread");

static ble::connection_handle_t c_handle = 0;

static control_t echo_single_connection_blocking(const size_t call_count)
{

    // Send callback and parameter to the host runner
    greentea_send_kv("echo_single_connection_blocking",
            test_srv.get_mac_addr_and_type());

    // Wait until we get a connection
    TEST_ASSERT_NO_TIMEOUT(flags.wait_all_for(EVENT_FLAGS_BLE_CONNECTED, DEFAULT_TIMEOUT));

    // We are connected, get the BleSerial connection
    mbed::SharedPtr<UARTService::BleSerial> ser = test_srv.uart_svc.get_ble_serial_handle(c_handle);

    uint16_t mtu = ser->get_mtu();

    uint8_t* buf = new uint8_t[mtu];

    TEST_ASSERT(buf != nullptr);

    // Echo loop
    while(!ser->is_shutdown()) {
        ssize_t result = ser->read(buf, mtu);
        // Serial shutdown during read
        if(result == -ESHUTDOWN) {
            ser = nullptr;
            break;
        }

#if READ_WRITE_DELAY
        // Small delay between read/write
        rtos::ThisThread::sleep_for(READ_WRITE_DELAY);
#endif

        result = ser->write(buf, result);
        // Serial shutdown during write
        if(result == -ESHUTDOWN) {
            ser = nullptr;
            break;
        }
    }

    return CaseNext;
}

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(60, "ble_serial_test");

    return greentea_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Echo single connection blocking", echo_single_connection_blocking),
//    Case("Echo multi connection blocking", echo_multi_connection),
//    Case("Echo single connection non-blocking", echo_non_blocking),
//    Case("Echo multi connection non-blocking", echo_non_blocking),
//    Case("Disconnect while reading", dc_while_reading),
//    Case("Disconnect while writing", dc_while_writing),
//    Case("Memory leak", memory_leak)
};

Specification specification(greentea_setup, cases);

class TestGapEventHandler : public ble::Gap::EventHandler {

public:

    TestGapEventHandler() { }

    virtual ~TestGapEventHandler() { }

    void onConnectionComplete(const ConnectionCompleteEvent &event) override {
        // TODO add to list of multiple connection handles
        c_handle = event.getConnectionHandle();
        flags.set(EVENT_FLAGS_BLE_CONNECTED);
    }

    void onDisconnectionComplete(const DisconnectionCompleteEvent &event) override {
        flags.set(EVENT_FLAGS_BLE_DISCONNECTED);
    }
};

static void ble_thread_main(void) {

    // Add the TestGapEventHandler
    static TestGapEventHandler test_eh;
    test_srv.gap_eh_chain.addEventHandler(&test_eh);

    // This should never return
    test_srv.start();
}

static void on_ble_ready(void) {
    flags.set(EVENT_FLAGS_BLE_READY);
}

int main()
{
    // Setup BleSerialTestServer
    test_srv.on_ready(on_ble_ready);

    // Spin off BLE thread
    ble_thread.start(ble_thread_main);

    // Wait for BLE system to be ready
    TEST_ASSERT_NO_TIMEOUT(flags.wait_all_for(EVENT_FLAGS_BLE_READY, DEFAULT_TIMEOUT));

    return !Harness::run(specification);
}


#endif
