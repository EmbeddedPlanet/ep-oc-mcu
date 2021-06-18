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

#ifndef SERVICES_SI7021SERVICE_H_
#define SERVICES_SI7021SERVICE_H_

#include "platform/mbed_debug.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define Si7021_SERVICE_UUID UUID("00000002-8dd4-4087-a16a-04a7c8e01734")

class Si7021Service {

public:

	Si7021Service() :
		temp_c_char(GattCharacteristic::UUID_TEMPERATURE_CHAR, &temp_c,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		rel_humidity_char(GattCharacteristic::UUID_HUMIDITY_CHAR, &rel_humidity,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		temp_c(0), rel_humidity(0),
		Si7021_service(
		/** UUID */					Si7021_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &temp_c_char;
			characteristics[1] = &rel_humidity_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(Si7021_service);

		if(err) {
			debug("Error %u during Si7021 service registration. \r\n", err);
			return;
		}

		debug("Si7021 service registered\r\n");
		debug("service handle: %u\r\n", Si7021_service.getHandle());
		started = true;

	}

	uint16_t get_rel_humidity() const {
		uint16_t len = sizeof(rel_humidity);
		server->read(rel_humidity_char.getValueHandle(), (uint8_t*) &rel_humidity, &len);
		return rel_humidity;
	}

	void set_rel_humidity(uint16_t rel_humidity) {
		this->rel_humidity = rel_humidity;
		server->write(rel_humidity_char.getValueHandle(), (uint8_t*) &this->rel_humidity, sizeof(this->rel_humidity));
	}

	int16_t get_temp_c() const {
		uint16_t len = sizeof(temp_c);
		server->read(temp_c_char.getValueHandle(), (uint8_t*) &temp_c, &len);
		return temp_c;
	}

	void set_temp_c(int16_t temp_c) {
		this->temp_c = temp_c;
		server->write(temp_c_char.getValueHandle(), (uint8_t*) &this->temp_c,
				sizeof(this->temp_c));
	}

protected:

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<int16_t> temp_c_char;
	ReadOnlyGattCharacteristic<uint16_t> rel_humidity_char;

	/** Underlying data */
	int16_t temp_c;
	uint16_t rel_humidity;

	GattCharacteristic* characteristics[2];

	GattService Si7021_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_SI7021SERVICE_H_ */
