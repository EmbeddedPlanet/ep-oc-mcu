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

#include "GattSubscriber.h"

#include "platform/mbed_debug.h"

// Surprisingly not part of Mbed's BLE headers
#define CCCD_UUID UUID(0x2902)

using namespace ep;

GattSubscriber::GattSubscriber(events::EventQueue& queue) : _queue(queue),
	_state(STATE_INITIALIZED), _spec(NULL), _connection_handle(NULL),
	_max_retries(0), _retries_left(0), _timeout_ms(5000), _retry_delay_ms(500),_timeout(),
	_result(), _discovered_chars(NULL),
	_ignore_termination_cb(false), _char_flags(NULL),
	_idx_current_char(0)
{

	BLE& ble = BLE::Instance();

	// Reset the subscriber on connection
	ble.gap().onConnection(
			Gap::ConnectionEventCallback_t(
					this, &GattSubscriber::on_connect));

	// Attach a service discovery terminated callback
	ble.gattClient().onServiceDiscoveryTermination(
			FunctionPointerWithContext<Gap::Handle_t>
	(this, &GattSubscriber::discovery_termination_cb));

	// Attach an on_written callback
	// Used to determine if writes to the peer succeeded
	ble.gattClient().onDataWritten(
			FunctionPointerWithContext<const GattWriteCallbackParams*>
	(this, &GattSubscriber::on_written_cb));

}

GattSubscriber::~GattSubscriber()
{
	// Shut down and delete dynamic variables
	_state = STATE_SHUTTING_DOWN;
	reset();
}

void GattSubscriber::discover_and_subscribe(
		mbed::Callback<void(result_t)> result_cb, subscription_spec_t* spec,
		Gap::Handle_t* connection_handle, unsigned int max_retries, unsigned int timeout_ms,
		unsigned int retry_delay_ms)
{

	if(_state == STATE_INITIALIZED || _state == STATE_FAILED)
	{
		// Store settings
		_result_cb = result_cb;
		_spec = spec;
		_connection_handle = connection_handle;
		_max_retries = max_retries;
		_timeout_ms = timeout_ms;
		_retry_delay_ms = retry_delay_ms;

		_result.subscriber = this;
		_result.service_uuid = _spec->service_uuid;
		_result.num_chars = _spec->num_characteristics;

		// Start with discovering the service
		debug("gatt subscriber: service discovery begin\r\n");
		_state = STATE_DISCOVERING_SERVICE;
		_retries_left = _max_retries;
		start_service_discovery();
	}
	else
	{
		debug("gatt subscriber: cannot start discovery in current state!");
	}
}

void GattSubscriber::reset(void)
{
	if(_discovered_chars)
	{
		delete[] _discovered_chars;
		_discovered_chars = 0;
	}


	if(_char_flags)
	{
		delete[] _char_flags;
		_char_flags = 0;
	}

	_state = STATE_INITIALIZED;
}

void GattSubscriber::update_state(bool success)
{
	if(!success)
	{
		if(--_retries_left == 0)
		{
			// We're out of retries, report failure
			_result.status = (result_status_t)(_state);
			_state = STATE_FAILED;
		}
	}

	switch(_state)
	{

	case STATE_INITIALIZED:
		break;

	case STATE_DISCOVERING_SERVICE:
		if(success)
		{
			debug("gatt subscriber: found service\r\n");

			// Terminate service discovery
			_ignore_termination_cb = true;
			BLE::Instance().gattClient().terminateServiceDiscovery();

			// If any characteristics were specified, start discovery

			if(_spec->num_characteristics != 0)
			{
				// Allocate flags if not already
				if(!_discovered_chars)
				{
					_discovered_chars =
							new subscription_char_t[_spec->num_characteristics];
				}

				if(!_char_flags)
					_char_flags = new bool[_spec->num_characteristics];

				memset(_discovered_chars, 0,
						(_spec->num_characteristics * sizeof(subscription_char_t)));

				memset(_char_flags, false, _spec->num_characteristics);

				_result.discovered_chars = _discovered_chars;

				_state = STATE_DISCOVERING_CHARACTERISTICS;
				_retries_left = _max_retries;
				start_characteristic_discovery();
			}
			else
			{
				// TODO - be done
			}
		}
		else
		{
			debug("gatt subscriber: service discovery failed, retrying...\r\n");
			_queue.call_in(_retry_delay_ms,
					mbed::callback(this, &GattSubscriber::start_service_discovery));
		}
		break;

	case STATE_DISCOVERING_CHARACTERISTICS:
		if(success)
		{
			debug("gatt subscriber: discovered all characteristics\r\n");

			// Start discovering descriptors
			_state = STATE_DISCOVERING_DESCRIPTORS;
			_retries_left = _max_retries;

			// Clear the flags
			memset(_char_flags, false, _spec->num_characteristics);

			// Flags in this case will keep track of characteristics
			// for which DescriptorDiscovery is done (either skipped or successful)
			debug("gatt subscriber: discovering descriptors...\r\n");
			_queue.call_in(300,
					mbed::callback(this, &GattSubscriber::start_descriptor_discovery));
		}
		else
		{
			debug("gatt subscriber: characteristic discovery failed, retrying...\r\n");
			_queue.call_in(_retry_delay_ms,
					mbed::callback(this, &GattSubscriber::start_characteristic_discovery));
		}
		break;

	case STATE_DISCOVERING_DESCRIPTORS:
		if(success)
		{
			// Initiate subscribing
			debug("gatt subscriber: all descriptors found, subscribing...\r\n");

			_state = STATE_SUBSCRIBING;
			_retries_left = _max_retries;

			// Clear the flags
			memset(_char_flags, false, _spec->num_characteristics);

			// Flags in this case will keep track of descriptors
			// that have been successfully written (or skipped)

			subscribe();
		}
		else
		{
			debug("gatt subscriber: descriptor discovery failed, retrying...\r\n");
			// Retry
			_queue.call_in(_retry_delay_ms,
					mbed::callback(this, &GattSubscriber::start_descriptor_discovery));
		}
		break;

	case STATE_SUBSCRIBING:
		if(success)
		{
			// We're all done, tell the application everything succeeded
			debug("gatt subscriber: done subscribing...\r\n");

			_state = STATE_SUBSCRIBED;
			_result.status = SUBSCRIBE_SUCCESS;
			_result_cb(_result);
		} else {
			// Retry subscribing
			debug("gatt subscriber: subscribing failed, retrying...\r\n");
			_queue.call_in(_retry_delay_ms,
					mbed::callback(this, &GattSubscriber::subscribe));
		}
		break;

	case STATE_SUBSCRIBED:
		// Do nothing
		break;

	case STATE_FAILED:
		// Call the application's result handler with failure
		_result_cb(_result);
		return;
		break;

	}

	// If the above switch doesn't return, reset the timeout
	//reset_timeout();
}

void GattSubscriber::start_service_discovery(void)
{
	BLE& ble = BLE::Instance();

	// If we're not already discovering services
	if(!ble.gattClient().isServiceDiscoveryActive())
	{
		// Attempt to discover the specified service
		ble_error_t error = ble.gattClient().launchServiceDiscovery(
				*_connection_handle,
				ServiceDiscovery::ServiceCallback_t
				(this, &GattSubscriber::service_discovered_cb),
				NULL, _spec->service_uuid);

		if(error) {
			debug("gatt subscriber: failed to launch service discovery\r\n");
		}

		/*
		 * From here, one of two things will happen:
		 * 1.) service_discovered_cb will be called, indicating
		 *		 the service was found on the peer
		 *
		 * 2.) discovery_termination_cb will be called, indicating
		 * 	 the service was not found or some other error occurred
		 */

	}
	else
	{
		// Someone else is discovering, report failure (and retry later)
		_queue.call(mbed::callback(this, &GattSubscriber::update_state), false);
	}
}

void GattSubscriber::start_characteristic_discovery(void)
{
	debug("gatt_subscriber: characteristic discovery begin\r\n");

	BLE& ble = BLE::Instance();

	if(!ble.gattClient().isServiceDiscoveryActive())
	{
		ble.gattClient().launchServiceDiscovery(*_connection_handle, NULL,
				ServiceDiscovery::CharacteristicCallback_t
				(this, &GattSubscriber::characteristic_discovered_cb),
				 _spec->service_uuid);

		/*
		 * From here, one of three things will happen:
		 * 1.) characteristic_discovered_cb will be called, indicating
		 *		 a characteristic was found on the peer
		 *
		 * 2.) discovery_termination_cb will be called, indicating
		 * 	 characteristic discovery is over
		 *
		 * Hopefully when 2 happens, all required characteristics have been found
		 */
	}
	else
	{
		// Someone else is discovering, report failure (and retry later)
		_queue.call(mbed::callback(this, &GattSubscriber::update_state), false);
	}
}

void GattSubscriber::start_descriptor_discovery(void)
{

	// Loop through until an unflagged char is found
	for(_idx_current_char = 0;
			_idx_current_char < _spec->num_characteristics;
			_idx_current_char++)
	{
		// If the character doesn't need to subscribe, skip it
		if(_spec->characteristics[_idx_current_char].subscription == NO_SUBSCRIPTION)
			_char_flags[_idx_current_char] = true;

		if(!_char_flags[_idx_current_char])
			break;
	}

	if(_idx_current_char == _spec->num_characteristics)
	{
		// Looped all the way through, all descriptors found
		// Notify the state machine
		_queue.call(mbed::callback(this, &GattSubscriber::update_state), true);
		return;
	}
	else
	{
		// Start descriptor discovery for
		// the characteristic that broke the loop
		// (if the application wants to subscribe)
		DiscoveredCharacteristic characteristic =
				_discovered_chars[_idx_current_char].characteristic;

		debug("gatt subscriber: discovering descriptor for characteristic: 0x%04X\r\n",
				characteristic.getUUID().getShortUUID());

		BLE& ble = BLE::Instance();

		if(!ble.gattClient().isCharacteristicDescriptorDiscoveryActive(
				characteristic))
		{
			// This is one GIGANTIC LINE OF CODE
			// It just instantiates function pointers in the
			// format the BLE stack likes...
			characteristic.discoverDescriptors(
					CharacteristicDescriptorDiscovery::DiscoveryCallback_t(
							this, &GattSubscriber::descriptor_discovery_cb),
					CharacteristicDescriptorDiscovery::TerminationCallback_t(
							this, &GattSubscriber::descriptor_discovery_termination_cb));
		}
	}
}

void GattSubscriber::subscribe(void)
{
	// Loop through until an unflagged char is found
	for(_idx_current_char = 0;
			_idx_current_char < _spec->num_characteristics;
			_idx_current_char++)
	{
		// If the character doesn't need to subscribe, skip it
		if(_spec->characteristics[_idx_current_char].subscription == NO_SUBSCRIPTION)
			_char_flags[_idx_current_char] = true;

		if(!_char_flags[_idx_current_char])
			break;
	}

	if(_idx_current_char == _spec->num_characteristics)
	{
		// Looped all the way through, all descriptors found
		// Notify the state machine
		_queue.call(mbed::callback(this, &GattSubscriber::update_state), true);
		return;
	}
	else
	{
		// Initiate write to associated descriptor
		GattAttribute::Handle_t descriptor_handle =
				_discovered_chars[_idx_current_char].descriptor;

		BLE& ble = BLE::Instance();

		ble.gattClient().write(GattClient::GATT_OP_WRITE_REQ,
				*_connection_handle, descriptor_handle,
				sizeof(uint16_t),
				reinterpret_cast<const uint8_t*>(
						&_spec->characteristics[_idx_current_char].subscription));
	}
}

void GattSubscriber::service_discovered_cb(const DiscoveredService* service)
{
	// Tell the state machine this step succeeded
	_ignore_termination_cb = true;
	_queue.call(mbed::callback(this, &GattSubscriber::update_state), true);
}

void GattSubscriber::characteristic_discovered_cb(
			const DiscoveredCharacteristic* characteristic)
{
	debug("gatt subscriber: discovered characteristic\r\n");
	debug("                 uuid: %04X\r\n",
			characteristic->getUUID().getShortUUID());
	debug("                 handle: %02X\r\n", characteristic->getValueHandle());
	debug("                 properties: %02X\r\n",
			*((uint8_t*)&(characteristic->getProperties())));

	// Check discovered characteristic against those in spec
	for(int i = 0; i < _spec->num_characteristics; i++)
	{
		char_query_t query = _spec->characteristics[i];
		// Matching UUID
		if(query.uuid == characteristic->getUUID())
		{
			// Collect the pointer to its handle
			_discovered_chars[i].characteristic = (DiscoveredCharacteristic) *characteristic;

			// Flag it
			_char_flags[i] = true;
			debug("gatt subscriber: characteristic %i flagged\r\n", i);
			break;
		}
	}

	// Check all flags
	for(int i = 0; i < _spec->num_characteristics; i++)
	{
		if(!_char_flags[i])
			return;
	}

	// If we get here then all characteristics have been found
	// Terminate characteristic discovery
	_ignore_termination_cb = true;
	BLE::Instance().gattClient().terminateServiceDiscovery();

	// Tell the state machine we completed successfully
	_queue.call(mbed::callback(this, &GattSubscriber::update_state), true);


}

void GattSubscriber::discovery_termination_cb(Gap::Handle_t handle)
{
	if(handle == *_connection_handle)
	{
		// Since this is also called on success, we ignore
		// the discovery termination callback after a success
		if(!_ignore_termination_cb)
		{
			debug("discovery_termination_cb\r\n");
			// This is called when either service OR characteristic discovery is terminated
			// Tell the state machine this step failed
			_queue.call(mbed::callback(this, &GattSubscriber::update_state), false);
		}
		else
			_ignore_termination_cb = false;
	}
}

void GattSubscriber::descriptor_discovery_cb(
		const CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t* params)
{
	debug("gatt subscriber: discovered descriptor\r\n");
	debug("                 uuid: %04X\r\n",
			params->descriptor.getUUID().getShortUUID());
	debug("                 handle: %02X\r\n", params->descriptor.getAttributeHandle());


	if(params->descriptor.getUUID() != CCCD_UUID) {
		debug("gatt subscriber: not a CCCD, skipping\r\n");
		return;
	}

	// Save a handle to it
	_discovered_chars[_idx_current_char].descriptor = params->descriptor.getAttributeHandle();

	// Flag it
	_char_flags[_idx_current_char] = true;

	// Terminate discovery for this characteristic
	_ignore_termination_cb = true;
	BLE::Instance().gattClient().terminateCharacteristicDescriptorDiscovery(
			params->characteristic);

	// Discover the next descriptor or advance the state machine
	// Needs to be delayed so the BLE stack state can update first
	_queue.call_in(200, mbed::callback(this,
			&GattSubscriber::start_descriptor_discovery));
}

void GattSubscriber::descriptor_discovery_termination_cb(
		const CharacteristicDescriptorDiscovery::TerminationCallbackParams_t* params)
{
	if(!_ignore_termination_cb)
	{
		// This means descriptor discovery terminated without success... retry
		_queue.call(mbed::callback(this, &GattSubscriber::update_state), false);

	}
	else
	{
		_ignore_termination_cb = false;
	}
}

void GattSubscriber::on_written_cb(const GattWriteCallbackParams* params)
{
	if(params->connHandle == *_connection_handle)
	{
		if(params->handle ==
				_discovered_chars[_idx_current_char].descriptor)
		{
			debug("gatt subscriber: descriptor %i - %s\r\n", _idx_current_char, (params->status? "FAILED" : "SUCCESS"));

			// Flag it
			if(params->status == BLE_ERROR_NONE) {
				_char_flags[_idx_current_char] = true;
				_queue.call(mbed::callback(this, &GattSubscriber::subscribe));
			} else {
				// Descriptor write request rejected -- the application may need to upgrade link security
				// Retry
				_queue.call(mbed::callback(this, &GattSubscriber::update_state), false);
			}


		}
	}
}

void GattSubscriber::on_connect(
		const Gap::ConnectionCallbackParams_t* params)
{
	reset();
}

void GattSubscriber::timeout_cb(void)
{
	debug("gatt subscriber:  timeout cb!\r\n");
	// Timeout - report a failure
	_queue.call(mbed::callback(this, &GattSubscriber::update_state), false);
}

void GattSubscriber::reset_timeout(void)
{
	_timeout.detach();

	// Start the local timeout
	_timeout.attach_us(mbed::callback(this, &GattSubscriber::timeout_cb),
			_timeout_ms);
}
