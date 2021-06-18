/**
 * ep-oc-mcu
 * Embedded Planet Open Core for Microcontrollers
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2019 Embedded Planet, Inc.
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

#ifndef SERVICES_BATTERY_VOLTAGE_SERVICE_H_
#define SERVICES_BATTERY_VOLTAGE_SERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define BATTERY_VOLTAGE_SERVICE_UUID UUID("00000009-8dd4-4087-a16a-04a7c8e01734")
#define BATTERY_VOLTAGE_CHAR_UUID UUID("00001009-8dd4-4087-a16a-04a7c8e01734")

class BatteryVoltageService {

public:

	BatteryVoltageService() :
		battery_voltage_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32,
				GattCharacteristic::BLE_GATT_UNIT_ELECTRIC_POTENTIAL_DIFFERENCE_VOLT), battery_voltage_desc_ptr(&battery_voltage_desc),
		battery_voltage_char(BATTERY_VOLTAGE_CHAR_UUID, &voltage,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&battery_voltage_desc_ptr), 1),
		voltage(0.0f),
		battery_voltage_service(
		/** UUID */					BATTERY_VOLTAGE_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &battery_voltage_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(battery_voltage_service);

		if(err) {
			debug("Error %u during BatteryVoltage service registration. \r\n", err);
			return;
		}

		debug("BatteryVoltage service registered\r\n");
		debug("service handle: %u\r\n", battery_voltage_service.getHandle());
		started = true;

	}

	float get_voltage() const {
		uint16_t len = sizeof(voltage);
		server->read(battery_voltage_char.getValueHandle(), (uint8_t*) &voltage, &len);
		return voltage;
	}

	void set_voltage(float voltage) {
		this->voltage = voltage;
		server->write(battery_voltage_char.getValueHandle(), (uint8_t*) &this->voltage,
				sizeof(this->voltage));
	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor battery_voltage_desc;
	GattPresentationFormatDescriptor* battery_voltage_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<float> battery_voltage_char;

	/** Underlying data */
	float voltage;

	GattCharacteristic* characteristics[1];

	GattService battery_voltage_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_BATTERYVOLTAGE_SERVICE_H_ */
