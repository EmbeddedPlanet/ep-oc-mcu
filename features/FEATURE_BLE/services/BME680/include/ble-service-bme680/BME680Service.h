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

#ifndef SERVICES_BME680SERVICE_H_
#define SERVICES_BME680SERVICE_H_

#include "platform/mbed_debug.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define BME680_SERVICE_UUID UUID("00000001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_EST_CO2_CHAR_UUID UUID("00001001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_EST_BVOC_CHAR_UUID UUID("00002001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_IAQ_SCORE_CHAR_UUID UUID("00003001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_IAQ_ACCURACY_CHAR_UUID UUID("00004001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_GAS_RESISTANCE_CHAR_UUID UUID("00005001-8dd4-4087-a16a-04a7c8e01734")

class BME680Service {

public:

	BME680Service() :
		estimated_co2_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32, BLE_GATT_UNIT_CONCENTRATION_PPM), estimated_co2_desc_ptr(&estimated_co2_desc),
		estimated_bVOC_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32, BLE_GATT_UNIT_CONCENTRATION_PPM), estimated_bVOC_desc_ptr(&estimated_bVOC_desc),
		iaq_score_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT16), iaq_score_desc_ptr(&iaq_score_desc),
		iaq_accuracy_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT8), iaq_accuracy_desc_ptr(&iaq_accuracy_desc),
		gas_resistance_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT32, GattCharacteristic::BLE_GATT_UNIT_ELECTRIC_RESISTANCE_OHM), gas_resistance_desc_ptr(&gas_resistance_desc),
		temp_c_char(GattCharacteristic::UUID_TEMPERATURE_CHAR, &temp_c,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		rel_humidity_char(GattCharacteristic::UUID_HUMIDITY_CHAR, &rel_humidity,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		pressure_char(GattCharacteristic::UUID_PRESSURE_CHAR, &pressure,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		estimated_co2_char(BME680_EST_CO2_CHAR_UUID, &estimated_co2,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&estimated_co2_desc_ptr), 1),
		estimated_bVOC_char(BME680_EST_BVOC_CHAR_UUID, &estimated_bVOC,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&estimated_bVOC_desc_ptr), 1),
		iaq_score_char(BME680_IAQ_SCORE_CHAR_UUID, &iaq_score,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&iaq_score_desc_ptr), 1),
		iaq_accuracy_char(BME680_IAQ_ACCURACY_CHAR_UUID, &iaq_accuracy,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&iaq_accuracy_desc_ptr), 1),
		gas_resistance_char(BME680_GAS_RESISTANCE_CHAR_UUID, &gas_resistance,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&gas_resistance_desc_ptr), 1),
		temp_c(0), rel_humidity(0), pressure(0), estimated_co2(0.0f), estimated_bVOC(0.0f), iaq_score(0),
		iaq_accuracy(0), gas_resistance(0),
		bme680_service(
		/** UUID */					BME680_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &temp_c_char;
			characteristics[1] = &rel_humidity_char;
			characteristics[2] = &pressure_char;
			characteristics[3] = &estimated_co2_char;
			characteristics[4] = &estimated_bVOC_char;
			characteristics[5] = &iaq_score_char;
			characteristics[6] = &iaq_accuracy_char;
			characteristics[7] = &gas_resistance_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(bme680_service);

		if(err) {
			debug("Error %u during bme680 service registration. \r\n", err);
			return;
		}

		debug("bme680 service registered\r\n");
		debug("service handle: %u\r\n", bme680_service.getHandle());
		started = true;

	}

	float get_estimated_b_voc() const {
		uint16_t len = sizeof(estimated_bVOC);
		server->read(estimated_bVOC_char.getValueHandle(), (uint8_t*) &estimated_bVOC, &len);
		return estimated_bVOC;
	}

	void set_estimated_b_voc(float estimated_b_voc) {
		estimated_bVOC = estimated_b_voc;
		server->write(estimated_bVOC_char.getValueHandle(), (uint8_t*) &estimated_bVOC, sizeof(estimated_bVOC));
	}

	float get_estimated_co2() const {
		uint16_t len = sizeof(estimated_co2);
		server->read(estimated_co2_char.getValueHandle(), (uint8_t*) &estimated_co2, &len);
		return estimated_co2;
	}

	void set_estimated_co2(float estimated_co2) {
		this->estimated_co2 = estimated_co2;
		server->write(estimated_co2_char.getValueHandle(), (uint8_t*) &this->estimated_co2, sizeof(this->estimated_co2));
	}

	uint32_t get_gas_resistance() const {
		uint16_t len = sizeof(gas_resistance);
		server->read(gas_resistance_char.getValueHandle(), (uint8_t*) &gas_resistance, &len);
		return gas_resistance;
	}

	void set_gas_resistance(uint32_t gas_resistance) {
		this->gas_resistance = gas_resistance;
		server->write(gas_resistance_char.getValueHandle(), (uint8_t*) &this->gas_resistance, sizeof(this->gas_resistance));
	}

	uint8_t get_iaq_accuracy() const {
		uint16_t len = sizeof(iaq_accuracy);
		server->read(iaq_accuracy_char.getValueHandle(), (uint8_t*) &iaq_accuracy, &len);
		return iaq_accuracy;
	}

	void set_iaq_accuracy(uint8_t iaq_accuracy) {
		this->iaq_accuracy = iaq_accuracy;
		server->write(iaq_accuracy_char.getValueHandle(), (uint8_t*) &this->iaq_accuracy, sizeof(this->iaq_accuracy));
	}

	uint16_t get_iaq_score() const {
		uint16_t len = sizeof(iaq_score);
		server->read(iaq_score_char.getValueHandle(), (uint8_t*) &iaq_score, &len);
		return iaq_score;
	}

	void set_iaq_score(uint16_t iaq_score) {
		this->iaq_score = iaq_score;
		server->write(iaq_score_char.getValueHandle(), (uint8_t*) &this->iaq_score, sizeof(this->iaq_score));
	}

	uint32_t get_pressure() const {
		uint16_t len = sizeof(pressure);
		server->read(pressure_char.getValueHandle(), (uint8_t*) &pressure, &len);
		return pressure;
	}

	void set_pressure(uint32_t pressure) {
		this->pressure = pressure;
		server->write(pressure_char.getValueHandle(), (uint8_t*) &this->pressure, sizeof(this->pressure));
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
		server->write(temp_c_char.getValueHandle(), (uint8_t*) &this->temp_c, sizeof(this->temp_c));
	}

protected:



	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor estimated_co2_desc;
	GattPresentationFormatDescriptor* estimated_co2_desc_ptr;
	GattPresentationFormatDescriptor estimated_bVOC_desc;
	GattPresentationFormatDescriptor* estimated_bVOC_desc_ptr;
	GattPresentationFormatDescriptor iaq_score_desc;
	GattPresentationFormatDescriptor* iaq_score_desc_ptr;
	GattPresentationFormatDescriptor iaq_accuracy_desc;
	GattPresentationFormatDescriptor* iaq_accuracy_desc_ptr;
	GattPresentationFormatDescriptor gas_resistance_desc;
	GattPresentationFormatDescriptor* gas_resistance_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<int16_t> temp_c_char;
	ReadOnlyGattCharacteristic<uint16_t> rel_humidity_char;
	ReadOnlyGattCharacteristic<uint32_t> pressure_char;

	/** Custom Characteristics */
	ReadOnlyGattCharacteristic<float> estimated_co2_char;
	ReadOnlyGattCharacteristic<float> estimated_bVOC_char;
	ReadOnlyGattCharacteristic<uint16_t> iaq_score_char;
	ReadOnlyGattCharacteristic<uint8_t> iaq_accuracy_char;
	ReadOnlyGattCharacteristic<uint32_t> gas_resistance_char;

	/** Underlying data */
	int16_t temp_c;
	uint16_t rel_humidity;
	uint32_t pressure;
	float estimated_co2;
	float estimated_bVOC;
	uint16_t iaq_score;
	uint8_t iaq_accuracy;
	uint32_t gas_resistance;

	GattCharacteristic* characteristics[8];

	GattService bme680_service;

	GattServer* server;

	bool started;

};



#endif /* SERVICES_BME680SERVICE_H_ */
