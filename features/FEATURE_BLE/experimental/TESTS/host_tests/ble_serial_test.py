
from typing import List, Optional, Any, Mapping

from mbed_host_tests import BaseHostTest
from mbed_host_tests.host_tests_logger import HtrunLogger

from mbed_ble_test_suite.common import fixtures
from mbed_ble_test_suite.common.ble_device import BleDevice

import json

import time

from threading import Thread, Event, Timer
import random
import array
import inspect

# UART Service UUID
uart_svc_uuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
# UART TX Characteristic UUID
uart_tx_uuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
# UART RX Characteristic UUID
uart_rx_uuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

# Cached handles (GattServer under test should not change after first run)
cached_rx_handle = None
cached_tx_handle = None
cached_rx_cccd_handle = None

address_type_map = {0: "PUBLIC", 1: "RANDOM", 2: "PUBLIC_IDENTITY", 3: "RANDOM_STATIC_IDENTITY", 0xFF: "ANONYMOUS"}


def lineno():
    """Returns the current line number in our program."""
    return inspect.currentframe().f_back.f_lineno


def raise_if_different(expected, actual, line, text=''):
    """Raise a RuntimeError if actual is different than expected."""
    if expected != actual:
        raise RuntimeError('[{}]:{}, {} Got {!r}, expected {!r}'.format(__file__, line, text, actual, expected))


def raise_unconditionally(line, text=''):
    """Raise a RuntimeError unconditionally."""
    raise RuntimeError('[{}]:{}, {}'.format(__file__, line, text))


class UARTSerialAllocator(fixtures.BoardAllocator):

    def __init__(self, ht : BaseHostTest, platforms_supported: List[str], binaries: Mapping[str, str],
                 serial_inter_byte_delay: float, baudrate: int, command_delay: float):

        # Call the BoardAllocator init
        super(UARTSerialAllocator, self).__init__(platforms_supported, binaries, serial_inter_byte_delay, baudrate,
                                                  command_delay)

        self.port = ht.get_config_item("port")

        # Remove the serial port allocated to the host test
        self.allocation = [x for x in self.allocation if self.port not in x.description['serial_port']]


class BleSerialClient:

    def __init__(self, ble_dev):
        self.ble_dev = ble_dev  # type: BleDevice
        self.connection_info = None
        self.default_mtu = 20
        self.mtu = self.default_mtu

        self.logger = HtrunLogger('TEST')

    def discover_uart_handles(self):

        global cached_rx_handle, cached_tx_handle, cached_rx_cccd_handle

        r = self.ble_dev.gattClient.discoverAllServicesAndCharacteristics(
            self.connection_info["connection_handle"])

        if not r.success():
            self.logger.prn_err("failed to discover services and characteristics")
            return False

        for service in r.result:
            self.logger.prn_inf("service uuid: {}".format(service["UUID"]))
            # Found UART service
            if uart_svc_uuid in str(service["UUID"]):  # some of the uuids (short ones) are ints instead of strings
                for c in service['characteristics']:
                    if uart_rx_uuid in str(c['UUID']):
                        cached_rx_handle = c
                    elif uart_tx_uuid in str(c['UUID']):
                        cached_tx_handle = c

        if not cached_rx_handle:
            self.logger.prn_err("failed to discover uart rx characteristic")
            return False
        if not cached_tx_handle:
            self.logger.prn_err("failed to discover uart tx characteristic")
            return False

        # Discover RX char descriptors
        r = self.ble_dev.gattClient.discoverAllCharacteristicsDescriptors(
            self.connection_info['connection_handle'],
            cached_rx_handle['start_handle'],
            cached_rx_handle['end_handle'])

        if not r.success():
            self.logger.prn_err("failed to discover uart rx descriptors")
            return False

        for desc in r.result:
            try:
                if desc['UUID'] == 0x2902:  # CCCD UUID
                    cached_rx_cccd_handle = desc
                    break

            except TypeError:  # ignore if the UUID is a string we're looking for a short UUID
                continue
                
        if not cached_rx_cccd_handle:
            self.logger.prn_err("failed to discover uart rx CCCD")
            return False

        # Okay we're connected and have discovered what we need to
        return True

    def connect_to(self, mac, addr_type):

        global cached_rx_handle, cached_tx_handle, cached_rx_cccd_handle

        # Initialize BLE
        if not self.ble_dev.ble.init().success():
            return False

        self.logger.prn_inf("ble initialized")

        # Connect to given device MAC
        r = self.ble_dev.gap.connect(addr_type, mac)
        if not r.success():
            return False

        self.logger.prn_inf("connection to {} succeeded".format(mac))

        self.connection_info = r.result

        # Get the UART RX/TX handles and TX CCCD handle if not already cached
        if not cached_rx_handle:
            r = self.discover_uart_handles()
            if not r:
                return False

        # Set the connection parameters to increase test speed
        # Connection interval min = 7ms, Connection interval max = 30ms
        # Slave latency = 5, Supervision timeout = 2.5 seconds
        r = self.ble_dev.gap.updateConnectionParameters(self.connection_info["connection_handle"],
                                                        6, 24, 5, 250)
        if not r.success():
            raise_unconditionally(lineno(), "could not update connection parameters")

        # Wait for connection parameters to complete being updated
        self.ble_dev.wait_for_output("<<<", timeout=10, assert_timeout=False)
        event = self.ble_dev.events.get(block=True, timeout=10)
        d = json.loads(event)
        info = d['value']

        if 'on_connection_parameters_update_complete' in d['name']:
            if self.connection_info['connection_handle'] == info['connection_handle']:
                if "BLE_ERROR_NONE" != info['status']:
                    raise_unconditionally(lineno(), "failed to update connection parameters")

    def subscribe(self):
        # Write the CCCD to 01 to subscribe to notifications
        r = self.ble_dev.gattClient.writeCharacteristicDescriptor(
            self.connection_info["connection_handle"],
            cached_rx_cccd_handle["handle"], "0100"
        )

        if not r.success():
            self.logger.prn_err("could not write rx char CCCD")
            return False

        # Now listen for HVX
        r = self.ble_dev.gattClient.enableUnsolicitedHVX(1)
        if not r.success():
            self.logger.prn_err("Could not enable unsolicited HVX")
            return False

        return True

    def unsubscribe(self):

        # Disable HVX
        r = self.ble_dev.gattClient.enableUnsolicitedHVX(0)
        if not r.success():
            self.logger.prn_err("Could not disable unsolicited HVX")
            return False

        # Write the CCCD back to 00
        r = self.ble_dev.gattClient.writeCharacteristicDescriptor(
            self.connection_info["connection_handle"],
            cached_rx_cccd_handle["handle"], "0000"
        )
        if not r.success():
            self.logger.prn_err("could not write rx char CCCD")
            return False

        return True

    def update_mtu(self, mtu):
        if self.mtu != self.default_mtu:
            self.logger.prn_err("cannot update MTU twice in one connection")
            return False

        self.mtu = mtu

        # TODO - set the MTU of the connection

    def echo_test(self, data: bytearray):

        data_str = "".join("{:02x}".format(x) for x in data).upper()

        # Write the byte array to device
        # Format to hex string ([0x01, 0x02, ...] = "0102...")
        r = self.ble_dev.gattClient.writeWithoutResponse(
            self.connection_info["connection_handle"],
            cached_tx_handle["value_handle"],
            data_str
        )

        if not r.success():
            raise_unconditionally(lineno(), "could not write to tx char")


        # Wait for an HVX
        self.ble_dev.wait_for_output("<<<", timeout=10, assert_timeout=False)
        event = self.ble_dev.events.get(block=True, timeout=10)

        d = json.loads(event)
        if 'BLE_HVX_NOTIFICATION' in d['type']:
            if self.connection_info['connection_handle'] == d['connHandle']:
                if cached_rx_handle['value_handle'] == d['handle']:
                    # We received data, make sure it matches what we sent
                    self.logger.prn_inf("received data: {}".format(d['data']))
                    raise_if_different(data_str, d['data'].upper(), lineno(),
                                       'Payloads mismatch on connection {}'.format(
                                           self.connection_info['connection_handle']))
                    return

        raise_unconditionally(lineno(), "No echo payload received within timeout on connection {}".format(
            self.connection_info['connection_handle']
        ))

    def reset(self):
        self.ble_dev.ble.reset()


def echo_test(ble_ser : BleSerialClient, payload_size):
    """
    Send and receive random data using BleSerialClient.

    Verify the data received from the ble_ser matches the data sent to it
    Raise a RuntimeError if data does not match.

    :param ble_ser: client to use
    :param payload_size: size of random payload to echo
    :return: None
    """

    payload_out = array.array('B', (random.randint(0x00, 0xff) for _ in range(payload_size)))
    ble_ser.echo_test(payload_out)


# Thank you pyusb_basic host test :)
def random_size_echo_test(ble_ser : BleSerialClient, failure  : Event, error : Event, seconds, log, min_payload_size=1):
    """
    Repeat data echo test for given BLESerialClient
    :param ble_ser: BleSerialClient to use for the test
    :param failure: Set a failure Event if data verification fails.
    :param error: Set an error Event if unexpected error occurs.
    :param seconds: length of echo test
    :param log: Logger instance
    :param min_payload_size: Minimum payload size used (maximum is MTU)
    :return: None
    """

    end_ts = time.time() + seconds
    while time.time() < end_ts and not failure.is_set() and not error.is_set():
        payload_size = random.randint(min_payload_size, ble_ser.mtu)
        try:
            echo_test(ble_ser, payload_size)
        except RuntimeError as err:
            log(err)
            failure.set()

        time.sleep(0.01)


class BleSerialTest(BaseHostTest):

    def _callback_echo_single_connection_blocking(self, key, value, timestamp):

        # Get MAC address and address type from value
        print(value.split(","))
        mac = value.split(",")[0]
        addr_type = address_type_map[int(value.split(",")[1])]

        self.logger.prn_inf('Received MAC Addr: %s' % value)

        # Connect with a single client and perform the echo test
        client = BleSerialClient(self.allocator.allocate())

        client.reset()

        if not client.connect_to(mac, addr_type):
            self.notify_complete(False)

        # Subscribe to RX characteristic
        client.subscribe()

        # Run the echo test for a few seconds
        test_error = Event()
        test_failure = Event()
        test_kwargs = {
            'ble_ser': client,
            'failure': test_failure,
            'error': test_error,
            'secods': 3.0,
            'log': self.logger
        }

        random_size_echo_test(client, test_failure, test_error, 10.0, self.logger)

        if test_failure.is_set():
            raise_unconditionally(lineno(), 'Payload mismatch')

        # Unsubscribe from client and reset
        client.unsubscribe()
        client.reset()

        # Release client
        self.allocator.release(client.ble_dev)

        self.notify_complete(True)

    def _callback_echo_multi_connection_blocking(self, key, value, timestamp):
        pass

    def _callback_echo_single_connection_nonblocking(self, key, value, timestamp):
        pass

    def _callback_echo_multi_connection_nonblocking(self, key, value, timestamp):
        pass

    def _callback_disconnect_while_reading(self, key, value, timestamp):
        pass

    def _callback_disconnect_while_writing(self, key, value, timestamp):
        pass

    def _callback_memory_leak(self, key, value, timestamp):
        pass

    def setup(self):

        # Set up board allocator
        self.allocator = UARTSerialAllocator(self, ["NRF52_DK"],
                                             {"NRF52_DK": None},
                                             0.001, 115200, 0)

        # Register callbacks
        self.register_callback('echo_single_connection_blocking',
                               self._callback_echo_single_connection_blocking)
        self.register_callback('echo_multi_connection_blocking',
                               self._callback_echo_multi_connection_blocking)

    def result(self):
        pass

    def teardown(self):
        pass

    def __init__(self):
        super(BleSerialTest, self).__init__()

        self.logger = HtrunLogger('TEST')
        self.allocator = None


if __name__ == '__main__':
    test = BleSerialTest()
    test._callback_echo_single_connection_blocking(None, "eb:59:b8:48:1d:4d", 0)