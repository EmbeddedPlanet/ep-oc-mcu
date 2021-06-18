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

#ifndef SERVICES_LED_SERVICE_H_
#define SERVICES_LED_SERVICE_H_

#include "drivers/DigitalOut.h"

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define LED_SERVICE_UUID UUID("00000008-8dd4-4087-a16a-04a7c8e01734")
#define LED_STATUS_CHAR_UUID UUID("00001008-8dd4-4087-a16a-04a7c8e01734")

class LEDService {

public:

	LEDService(bool active_low = false) :
		led_status_desc(GattCharacteristic::BLE_GATT_FORMAT_BOOLEAN), led_status_desc_ptr(&led_status_desc),
		led_status_char(LED_STATUS_CHAR_UUID, &led_status,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&led_status_desc_ptr), 1),
		led_status(0),
		led_service(
		/** UUID */					LED_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false),
		out(NULL),
		active_low(active_low)
		{
			characteristics[0] = &led_status_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(led_service);

		if(err) {
			debug("Error %u during LED service registration. \r\n", err);
			return;
		}

		server->onDataWritten().add(this, &LEDService::on_data_written);

		debug("LED service registered\r\n");
		debug("service handle: %u\r\n", led_service.getHandle());
		started = true;

	}

	void bind(mbed::DigitalOut* output) {
		out = output;
	}

	bool get_led_status() const {
		uint16_t len = sizeof(led_status);
		server->read(led_status_char.getValueHandle(), (uint8_t*) &led_status, &len);
		return led_status;
	}

	void set_led_status(bool led_status) {
		this->led_status = led_status;
		server->write(led_status_char.getValueHandle(), (uint8_t*) &this->led_status,
				sizeof(this->led_status));
		if(out != NULL) {
			out->write((this->active_low? !led_status : led_status));
		}
	}

	/**
	 * Handler for when a characteristic in this service gets written to
	 */
	void on_data_written(const GattWriteCallbackParams* params) {
		if((params->handle == led_status_char.getValueHandle()) && params->len == 1) {
			if(out != NULL) {
				out->write((this->active_low? !(*params->data) : (*params->data)));
			}
		}
	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor led_status_desc;
	GattPresentationFormatDescriptor* led_status_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadWriteGattCharacteristic<bool> led_status_char;

	/** Underlying data */
	bool led_status;

	GattCharacteristic* characteristics[1];

	GattService led_service;

	GattServer* server;

	bool started;

	mbed::DigitalOut* out;

	bool active_low;

};


#endif /* SERVICES_LED_SERVICE_H_ */
