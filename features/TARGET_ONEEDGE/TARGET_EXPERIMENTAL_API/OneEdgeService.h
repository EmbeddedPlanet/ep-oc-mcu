/**
 * OneEdgeService.h
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2020 Embedded Planet, Inc.
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

#ifndef ONE_EDGE_SERVICE
#define ONE_EDGE_SERVICE

#include "mbed.h"
#include "ATHandler.h"
#include "CellularDevice.h"
#include "mbed_trace.h"

#define ME310_SOCKET_MAX 6
#define ME310_CONTEXT_MAX 6
#define ME310_CREATE_SOCKET_TIMEOUT 150000 //150 seconds
#define ME310_CLOSE_SOCKET_TIMEOUT 20000 // TCP socket max timeout is >10sec
#define ME310_MAX_RECV_SIZE 1000
#define ME310_MAX_SEND_SIZE 1023
#define ME310_SOCKET_BIND_FAIL 556
#define ME310_IPEASY_ACTIVATED_CONTEXT 1
#define ME310_IPEASY_DEACTIVATED_CONTEXT 0
#define ME310_SOCKET_TIMEOUT 1000
#define ME310_MAX_FULL_FILE_PATH_LENGTH 128
#define CTRL_Z  "\x1a"
#define ESC     "\x1b"
#define ONEEDGE_CLIENT_ENABLED 1
#define ONEEDGE_CLIENT_STATE_MAX_LENGTH 20
#define ONEEDGE_CLIENT_URL_MAX_LENGTH 50

class OneEdgeService {
public:

    /**
     * OneEdge LWM2M client ACK modality
     */
    enum AckModaliy {
        AckModalityAckNotRequired = 0,
        AckModalityAckRequired = 1
    };

    /**
     * OneEdge LWM2M client ring event
     */
    enum ClientRingEvent {
        RegisterEvent,      // The client needs to register
        UpdateEvent,        // Registration update to be sent
        NotificationEvent,  // A value under observation has changed and it should be notified to the server
        SMSWakeUpEvent,     // Wake up SMS received from the server
        DeregistrationEvent // Deregistration
    };

    /**
     * OneEdge LWM2M client info event
     */
    enum ClientInfoEvent {
        FotaRebootEvent,    // A reboot occurring during FW upgrade
        DeviceRebootEvent   // A reboot issued by LwM2M server. Case: EXEC 3/0/4 and EXEC 3/0/5.
    };

    /**
     * OneEdge LWM2M client server registration event
     */
    enum ClientServerRegistrationEvent {
        BootstrappingEvent,     // The client is starting the bootstrap to the specified server
        BootstrappedEvent,      // The client finished successfully the bootstrap to the specified server
        RegisteringEvent,       // The client is starting the DM connection to the specified server
        RegisteredEvent,        // The client finished successfully the DM connection to the specified server
        SuspendedEvent,         // The client suspended successfully the DM connection to the specified server, as requested via /1/x/4 execution resource
        ClientDisabledEvent,    // The client has been disabled by AT command or by internal failures (i.e.: handshake failure)
        ForceExitEvent          // The client failed the server connection and after the proper retries, it is stopped. This action is only manageable by a module reboot.
    };

    /**
     * OneEdge LWM2M client enabling status
     */
    enum ClientEnablingStatus {
        ClientDisabled = 0, // The client is disabled
        ClientEnabled = 1   // The client is enabled
    };

    /**
     * OneEdge LWM2M client internal status
     */
    enum ClientInternalStatus {
        Disabled,       // The client is disabled
        Waiting,        // Waiting for the user's ACK
        Active,         // After the ACK, the session is currently active
        Idle,           // There is not an active session currently
        Deregistering,  // The client is deregistering
        Unknown         // Current status unknown
    };

    /**
     * OneEdge LWM2M client status
     */
    struct ClientStatus {
        ClientEnablingStatus enabled_status;
        ClientInternalStatus internal_status;
    };

    /** 
     * Default constructor
     */
    OneEdgeService();

    /** 
     * Initializes the OneEdge service and communication with the Telit modem
     */
    void init();

    /** 
     * Enables the Telit OneEdge LWM2M client
     * 
     *  @param desired_state    The desired state for the client (enabled/dsiabled)
     *  @param context          The desired PDP context for the client to use
     *  @param mode             The desired ACK modality (ACK or no ACK)
     *  @return                 NSAPI_ERROR_OK on success, otherwise modem may be need power cycling
     */
    nsapi_error_t lwm2m_client_enable(ClientEnablingStatus desired_state = ClientEnabled, int context = 1, AckModaliy mode = AckModalityAckNotRequired);

    /** 
     * Retrieves the state of the Telit OneEdge LWM2M client
     * 
     *  @return The current state of the LWM2M client
     */
    ClientStatus lwm2m_client_get_status();

    /** 
     * Triggers the Telit OneEdge LWM2M client to send an ACK to the server
     * 
     *  @param action   Acknowledge: the <cid> context indicated in
     *                  #LWM2MENA command is active and the user allows the client
     *                  to send data through this.
     *  @return         NSAPI_ERROR_OK on success, otherwise modem may be need power cycling
     */
    nsapi_error_t lwm2m_client_send_ack(int action = 1);

    /** 
     * Sets the current value of the battery level resource via the Telit OneEdge LWM2M client
     * 
     *  @param battery_level    new battery level value
     *  @return                 NSAPI_ERROR_OK on success, otherwise modem may be need power cycling
     */
    nsapi_error_t lwm2m_client_set_battery_level(int battery_level);

    /** 
     * Enables the temperature LWM2M object for use with the Telit OneEdge LWM2M client
     * 
     *  @return         true on success or if the temperature object is already enabled
     */
    bool lwm2m_client_enable_temperature_object();

    /** 
     * Creates an instance of a temperature LWM2M object for use with the Telit OneEdge LWM2M client
     * 
     *  @param instance target temperature object instance
     *  @return         true on success or if the target temperature object instance already exists
     */
    bool lwm2m_client_create_temperature_object_instance(int instance = 0);

    /**
     * Sets the callback which is fired when a LWM2MRING event occurs
     * 
     * @param ring_callback The callback which is fired
     */
    void lwm2m_client_set_ring_callback(Callback<void(ClientRingEvent)> ring_callback) {
        _ring_callback = ring_callback;
    }

    /**
     * Sets the callback which is fired when a LWM2MINFO event occurs
     * 
     * @param info_callback The callback which is fired
     */
    void lwm2m_client_set_info_callback(Callback<void(ClientInfoEvent)> info_callback) {
        _info_callback = info_callback;
    }

    /**
     * Sets the callback which is fired when a LWM2M-TLT event occurs
     * 
     * @param server_registration_callback  The callback which is fired
     */
    void lwm2m_client_set_server_registration_callback(Callback<void(ClientServerRegistrationEvent, int, const char *)> server_registration_callback) {
        _server_registration_callback = server_registration_callback;
    }

    /**
     * Sets the callback which is fired when a LWM2MEND event occurs
     * 
     * @param end_callback  The callback which is fired
     */
    void lwm2m_client_set_end_callback(Callback<void(int)> end_callback) {
        _end_callback = end_callback;
    }

protected:

    /** 
     * Checks if target context of the Telit modem's IPEasy IP stack is activated
     * 
     *  @param context_id   context ID to check
     *  @return             true if the context is activated
     */
    bool is_ipeasy_context_activated(int context_id);

    /** 
     * Activates the target context of the Telit modem's IPEasy IP stack
     * 
     *  @param context_id   context ID to activate
     *  @return             NSAPI_ERROR_OK on success, otherwise modem may be need power cycling
     */
    nsapi_error_t activate_ipeasy_context(int context_id);

    /** 
     * Checks if a file exists on the Telit modem's internal storage in the 'XML' folder
     * 
     *  @param target_file  file to find
     *  @return             true if file exists, false if not
     */
    bool file_exists(char *target_file);

    /** 
     * Retrieves static pointer to the contents of the temperature object's (3303) XML description
     * 
     *  @return             pointer to file contents
     */
    const char *get_object_3303()
    {
        static char object_3303[] = {
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<LWM2M  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openmobilealliance.org/tech/profiles/LWM2M.xsd\">"
                "<Object ObjectType=\"MODefinition\">"
                    "<Name>Temperature</Name>"
                    "<Description1>Description: This IPSO object should be used with a temperature sensor to report a temperature measurement.  It also provides resources for minimum/maximum measured values and the minimum/maximum range that can be measured by the temperature sensor. An example measurement unit is degrees Celsius (ucum:Cel).</Description1>"
                    "<ObjectID>3303</ObjectID>"
                    "<ObjectURN>urn:oma:lwm2m:ext:3303</ObjectURN>"
                    "<MultipleInstances>Multiple</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Resources>"
                    "<Item ID=\"5700\">"
                        "<Name>Sensor Value</Name>"
                        "<Operations>R</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Mandatory</Mandatory>"
                        "<Type>Float</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>Defined by \"Units\" resource.</Units>"
                        "<Description>Last or Current Measured Value from the Sensor</Description>"
                    "</Item>"
                    "<Item ID=\"5601\">"
                        "<Name>Min Measured Value</Name>"
                        "<Operations>R</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Optional</Mandatory>"
                        "<Type>Float</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>Defined by \"Units\" resource.</Units>"
                        "<Description>The minimum value measured by the sensor since power ON or reset</Description>"
                    "</Item>"
                    "<Item ID=\"5602\">"
                        "<Name>Max Measured Value</Name>"
                        "<Operations>R</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Optional</Mandatory>"
                        "<Type>Float</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>Defined by \"Units\" resource.</Units>"
                        "<Description>The maximum value measured by the sensor since power ON or reset</Description>"
                    "</Item>"
                    "<Item ID=\"5603\">"
                        "<Name>Min Range Value</Name>"
                        "<Operations>R</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Optional</Mandatory>"
                        "<Type>Float</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>Defined by \"Units\" resource.</Units>"
                        "<Description>The minimum value that can be measured by the sensor</Description>"
                    "</Item>"
                    "<Item ID=\"5604\">"
                        "<Name>Max Range Value</Name>"
                        "<Operations>R</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Optional</Mandatory>"
                        "<Type>Float</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>Defined by \"Units\" resource.</Units>"
                        "<Description>The maximum value that can be measured by the sensor</Description>"
                    "</Item>"
                    "<Item ID=\"5701\">"
                        "<Name>Sensor Units</Name>"
                        "<Operations>R</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Optional</Mandatory>"
                        "<Type>String</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>"
                        "</Units>"
                        "<Description>Measurement Units Definition e.g. \"Cel\" for Temperature in Celsius.</Description>"
                    "</Item>"
                    "<Item ID=\"5605\">"
                        "<Name>Reset Min and Max Measured Values</Name>"
                        "<Operations>E</Operations>"
                        "<MultipleInstances>Single</MultipleInstances>"
                        "<Mandatory>Optional</Mandatory>"
                        "<Type>String</Type>"
                        "<RangeEnumeration>"
                        "</RangeEnumeration>"
                        "<Units>"
                        "</Units>"
                        "<Description>Reset the Min and Max Measured Values to Current Value</Description>"
                    "</Item>"
                    "</Resources>"
                    "<Description2>"
                    "</Description2>"
                "</Object>"
            "</LWM2M>\r\n"
        };
        return object_3303;
    };

    /**
     * LWM2M-TLT URC handler
     */
    void urc_lwm2m_tlt();

    /**
     * LWM2MRING URC handler
     */
    void urc_lwm2mring();

    /**
     * LWM2MEND URC handler
     */
    void urc_lwm2mend();

    /**
     * LWM2MINFO URC handler
     */
    void urc_lwm2minfo();
    
    CellularDevice *dev;
    ATHandler *at_handler;
    Callback<void(ClientRingEvent)> _ring_callback;
    Callback<void(ClientInfoEvent)> _info_callback;
    Callback<void(ClientServerRegistrationEvent, int, const char *)> _server_registration_callback;
    Callback<void(int)> _end_callback;
};

#endif // ONE_EDGE_SERVICE