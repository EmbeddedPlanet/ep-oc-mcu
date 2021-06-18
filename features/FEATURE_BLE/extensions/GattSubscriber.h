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

#ifndef EP_GATTSUBSCRIBER_H_
#define EP_GATTSUBSCRIBER_H_

#include "platform/Callback.h"

#include "drivers/Timeout.h"

#include "ble/BLE.h"
#include "ble/GattClient.h"
#include "ble/Gap.h"
#include "ble/UUID.h"
#include "ble/DiscoveredCharacteristic.h"
#include "ble/DiscoveredCharacteristicDescriptor.h"

#include "events/EventQueue.h"

namespace ep {

	class GattSubscriber
	{
		public:

			typedef enum subscriber_state_t
			{
				STATE_INITIALIZED,
				STATE_DISCOVERING_SERVICE,
				STATE_DISCOVERING_CHARACTERISTICS,
				STATE_DISCOVERING_DESCRIPTORS,
				STATE_SUBSCRIBING,
				STATE_SUBSCRIBED,
				STATE_FAILED,
				STATE_SHUTTING_DOWN
			} subscriber_state_t;

			typedef enum subscription_t
			{
				NO_SUBSCRIPTION			= 0x00,
				SUBSCRIPTION_NOTIFY		= BLE_HVX_NOTIFICATION,
				SUBSCRIPTION_INDICATE	= BLE_HVX_INDICATION
			} subscription_t;

			// Errors match up with the state in which they happened
			typedef enum result_status_t
			{
				SUBSCRIBE_SERVICE_DISCOVERY_FAILED			= STATE_DISCOVERING_SERVICE,			/** Service may not exist on connected GattServer */
				SUBSCRIBE_CHARACTERISTIC_DISCOVERY_FAILED	= STATE_DISCOVERING_CHARACTERISTICS,	/** Characteristics may not exist in service */
				SUBSCRIBE_DESCRIPTOR_DISCOVERY_FAILED		= STATE_DISCOVERING_DESCRIPTORS,		/** Descriptors may not be accessible -- elevate security by pairing */
				SUBSCRIBE_FAILED_INSUFFICIENT_SECURITY		= STATE_SUBSCRIBING,					/** Writing to descriptors failed -- upgrade link security by pairing */
				SUBSCRIBE_SUCCESS
			} result_status_t;

			typedef struct char_query_t
			{
				UUID uuid;	/**!< Associated BLE UUID for the desired GattCharacteristic */
				subscription_t subscription;	/**!< Desired type of subscription */
			} char_query_t;

			typedef struct subscription_spec_t
			{
					UUID service_uuid;		/**!< Associated BLE service UUID */
					const char_query_t* characteristics;	/**!< BLE Characteristics queries */
					uint8_t num_characteristics;	/**!< Number of characteristics in query */
			} subscription_spec_t;

			typedef struct subscription_char_t
			{
					DiscoveredCharacteristic characteristic;
					GattAttribute::Handle_t descriptor;
			} subscription_char_t;

			typedef struct result_t
			{
					GattSubscriber* subscriber; /*!< Related GattSubscriber instance */
					result_status_t status; /*!< Status of the result */
					UUID service_uuid;	/*!< UUID of the associated Gatt Service */
					subscription_char_t* discovered_chars; /*!< Discovered characteristics and their descriptors */
					uint8_t num_chars; /*!< Number of discovered characteristics */
			} result_t;

		public:

			GattSubscriber(events::EventQueue& queue);

			~GattSubscriber();

			/**
			 * Starts discovery and subscription process.
			 * The GattSubscriber will take the provided subscription spec
			 * and first try to discover the specified Gatt Service
			 * on the connected peer (specified by connection_handle).
			 *
			 * If the service is found, the GattSubscriber will then attempt
			 * to discover any specified characteristics within that service.
			 * If any of the characteristics aren't found, failure will
			 * be reported.
			 *
			 * Once all characteristics have been discovered, the GattSubscriber
			 * will attempt to subscribe to any characteristics specified. To that end,
			 * the GattSubscriber will attempt to discover the descriptors of these
			 * specified characteristics.
			 *
			 * Once the descriptors are discovered, the GattSubscriber will attempt to write
			 * the appropriate flags to the CCCD. (https://www.oreilly.com/library/view/getting-started-with/9781491900550/ch04.html#gatt_cccd)
			 *
			 * If the write requests are rejected, the subscriber will return failure. This usually indicates
			 * that the link does not have sufficient security to subscribe. In this case, the app should
			 * attempt to initiate pairing with the peer before retrying the subscribe operation.
			 *
			 * This is especially important with iOS devices. Pairing MUST be deferred until a GATT operation
			 * fails due to insufficient security. If a peripheral attempts to pair before a GATT operation fails,
			 * the iOS device may immediately terminate the connection.
			 *
			 * If all subscriptions are made successfully, success will be returned.
			 *
			 * Results are communicated to the application via the result_cb argument.
			 *
			 * @note this is an asynchronous call and will return immediately
			 *
			 * @param[in] result_cb Application callback to communicate discovery results
			 * @param[in] spec Struct specifying services and characteristics to discover/subscribe
			 * @param[in] connection_handle Handle of the peer connection to perform discovery on
			 * @param[in] max_retries (default: 3) max number of operation retries before reporting failure
			 * @param[in] timeout_ms (default: 5 seconds) Milliseconds before discover/subscribe operation fails due to timeout
			 * @param[in] retry_delay_ms (default: 2 seconds) Delay before retrying failed operations
			 */
			void discover_and_subscribe(mbed::Callback<void(result_t)> result_cb,
					subscription_spec_t* spec, Gap::Handle_t* connection_handle,
					unsigned int max_retries = 3, unsigned int timeout_ms = 5000,
					unsigned int retry_delay_ms = 500);

			/**
			 * Gets the state of the discovery and subscription process
			 *
			 * @retval state Internal state of discovery process
			 */
			subscriber_state_t get_state() { return _state; }

			/** Resets the subscriber state machine to its initial state */
			void reset(void);

			Gap::Handle_t* get_connection_handle(void) {
				return _connection_handle;
			}


		private:

			/**
			 * Updates the internal state based on callback events
			 * from the BLE stack
			 *
			 * @param[in] success true if last state's operation was successful
			 */
			void update_state(bool success);

			/** Starts service discovery */
			void start_service_discovery(void);

			/** Starts characteristic discovery */
			void start_characteristic_discovery(void);

			/** Start descriptor discovery */
			void start_descriptor_discovery(void);

			/* Initiates subscribing */
			void subscribe(void);

			/***** BLE Stack Callbacks ******/

			void service_discovered_cb(const DiscoveredService* service);

			void discovery_termination_cb(Gap::Handle_t handle);

			void characteristic_discovered_cb(const DiscoveredCharacteristic* characteristic);

			void descriptor_discovery_cb(
					const CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t* params);

			void descriptor_discovery_termination_cb(
					const CharacteristicDescriptorDiscovery::TerminationCallbackParams_t* params);

			void on_written_cb(const GattWriteCallbackParams* params);

			void on_connect(const Gap::ConnectionCallbackParams_t* params);

			/***** Timeout *******/

			void timeout_cb(void);

			void reset_timeout(void);

		private:

			/** EventQueue context to execute on */
			events::EventQueue& _queue;

			/** State of the subscriber state machine */
			subscriber_state_t _state;

			/** Application result callback */
			mbed::Callback<void(result_t)> _result_cb;

			/** Subscription specifications */
			subscription_spec_t* _spec;

			/** Handle to peer connection */
			Gap::Handle_t* _connection_handle;

			/** Maximum number of retries */
			unsigned int _max_retries;

			/** Number of retries left before failure */
			unsigned int _retries_left;

			/** Milliseconds before failure due to timeout */
			unsigned int _timeout_ms;

			/** Delay before retrying failed operations */
			unsigned int _retry_delay_ms;

			/** Timeout timer for retries */
			mbed::Timeout _timeout;

			/** Local result variable */
			result_t _result;

			/** Character array for result */
			subscription_char_t* _discovered_chars;

			/**
			 * Ignore next discovery termination callback
			 * (Even on success, discovery termination cb is executed)
			 */
			volatile bool _ignore_termination_cb;

			/** Dynamic array of flags for characteristics */
			bool* _char_flags;

			/** Index of current characteristic for descriptor discovery */
			uint8_t _idx_current_char;

	};

}

#endif /* INCLUDE_BLE_CLIENTS_GATTSUBSCRIBER_H_ */
