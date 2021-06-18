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

#ifndef SERVICES_VL53L0XSERVICE_H_
#define SERVICES_VL53L0XSERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define VL53L0X_SERVICE_UUID UUID("00000006-8dd4-4087-a16a-04a7c8e01734")
#define VL53L0X_DISTANCE_CHAR_UUID UUID("00001006-8dd4-4087-a16a-04a7c8e01734")

class VL53L0XService {

public:

	VL53L0XService() :
		distance_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT16,
				GattCharacteristic::BLE_GATT_UNIT_LENGTH_METRE, -3), distance_desc_ptr(&distance_desc),
		distance_char(VL53L0X_DISTANCE_CHAR_UUID, &distance,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&distance_desc_ptr), 1),
		distance(0),
		VL53L0X_service(
		/** UUID */					VL53L0X_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &distance_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(VL53L0X_service);

		if(err) {
			debug("Error %u during VL53L0X service registration. \r\n", err);
			return;
		}

		debug("VL53L0X service registered\r\n");
		debug("service handle: %u\r\n", VL53L0X_service.getHandle());
		started = true;

	}

	uint16_t get_distance() const {
		uint16_t len = sizeof(distance);
		server->read(distance_char.getValueHandle(), (uint8_t*) &distance, &len);
		return distance;
	}

	void set_distance(uint16_t distance) {
		this->distance = distance;
		server->write(distance_char.getValueHandle(), (uint8_t*) &this->distance,
				sizeof(this->distance));
	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor distance_desc;
	GattPresentationFormatDescriptor* distance_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<uint16_t> distance_char;

	/** Underlying data */
	uint16_t distance;

	GattCharacteristic* characteristics[1];

	GattService VL53L0X_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_VL53L0XSERVICE_H_ */
