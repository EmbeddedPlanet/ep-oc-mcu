/**
 * ep-oc-mcu
 * Embedded Planet Open Core for Microcontrollers
 * 
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2021 Embedded Planet, Inc.
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

#ifndef EP_OC_MCU_EXTENSIONS_TELIT_ME310_GNSS_H_
#define EP_OC_MCU_EXTENSIONS_TELIT_ME310_GNSS_H_

// Only compile this if the config is enabled
#if TELIT_ME310_GNSS_ENABLED

#include "GNSS.h"
#include "ATHandler.h"
#include "CellularDevice.h"
#include "mbed_trace.h"
#include "TinyGPSplus.h"

namespace ep
{

static const int GPSCFG_SET_WWAN_GNSS_PRIORITY  = 0;
static const int GPSCFG_SET_TBF                 = 1;
static const int GPSCFG_SET_CONSTELLATION       = 2;
static const char *GPGGA_SENTENCE_URC_PREFIX    = "$GPGGA,";
static const char *GNRMC_SENTENCE_URC_PREFIX    = "$GNRMC,";

/**
 * Logical abstraction of the Telit ME310 GNSS controller
 */
class TELIT_ME310_GNSS : public GNSS  {
public:
    /**
     * GNSS priority
     */
    enum GNSSPriority {
        PRIORITY_GNSS = 0,
        PRIORITY_WWAN = 1
    };

    /**
     * ME310 eDRX modes for use with the AT#CEDRXS command
     */
    enum ME310eDRXMode {
        // Disable the use of eDRX
        EDRX_MODE_DISABLE           = 0,
        // Enable the use of eDRX
        EDRX_MODE_ENABLE            = 1,
        // Enable the use of eDRX and enable the unsolicited result code
        EDRX_MODE_ENABLE_URC        = 2,
        // Disable the use of eDRX and discard all parameters for eDRX,
        // or, if available, reset to the manufacturer specific default values
        EDRX_MODE_DISABLE_DISCARD   = 3
    };

    /**
     * ME310 access technology types for use with the
     * AT#CEDRXS command
     */
    enum ME310eDRXAcT {
        // Access technology is not using eDRX. This parameter value
        // is only used in the unsolicited result code, it cannot be
        // used in the set command.
        EDRX_ACT_NOT_EDRX   = 0,
        // GSM (A/Gb mode)
        EDRX_ACT_GSM        = 2,
        // E-UTRAN (CAT M1 mode)
        EDRX_ACT_CAT_M1     = 4,
        // E-UTRAN (NB1 mode)
        EDRX_ACT_NB1        = 5
    };

    /**
     * ME310 GNSS NMEA stream mode
     */
    enum ME310NMEAStreamMode {
        // Disable GNSS data stream
        NMEA_STREAM_DISABLE                             = 0,
        // Enable the first GNSS data stream format
        // Example:
        // $GPSNMUN: <NMEA SENTENCE 1><CR><LF>
        // ...
        // $GPSNMUN: <NMEA SENTENCE N><CR><LF>
        // ...
        NMEA_STREAM_ENABLE_FIRST_FORMAT                 = 1,
        // Enable the second GNSS data stream format
        // Example:
        // <NMEA SENTENCE 1><CR><LF>
        // ...
        // <NMEA SENTENCE N><CR><LF>
        // ...
        NMEA_STREAM_ENABLE_SECOND_FORMAT                = 2,
        // Enable the second GNSS data stream format and reserve the AT interface
        // port only for the GNSS data stream
        NMEA_STREAM_ENABLE_SECOND_FORMAT_RESERVE_PORT   = 3,
    };

    /**
     * Default constructor
     */
    TELIT_ME310_GNSS();

    /**
     * Initialize the connection with the GNSS controller. This
     * function should be called before enable/disable.
     */
    void init();

    /**
     * Enable the Telit ME310 GNSS controller
     * 
     * @return success
     */
    GNSSError enable();

    /**
     * Disable the Telit ME310 GNSS controller
     * 
     * @return success
     */
    GNSSError disable();

    /**
     * Retrieve the current position
     * 
     * @return The current position info
     */
    PositionInfo get_current_position();

    /**
     * Set GNSS controller priority (GNSS or WWAN)
     * 
     * @param desired_priority  Priority setting desired
     * @return success
     */
    GNSSError set_gnss_priority(GNSSPriority desired_priority);

    /**
     * Use the ME310's AT#CEDRXS command to set eDRX parameters. See the ME310
     * AT commands reference guide for more info.
     * 
     * @param mode                  Desired eDRX mode
     * @param access_technology     Desired access technology
     * @param req_edrx              half a byte in 4 bit format for eDRX value (string)
     * @param req_pag_time_window   half a byte in 4 bit format for paging window value (string)
     * @return success
     */
    GNSSError set_edrx_parameters(ME310eDRXMode mode, ME310eDRXAcT access_technology, char *req_edrx, char *req_pag_time_window);

    /**
     * Use the ME310's AT$GPSNMUN command to configure the GNSS NMEA stream.
     * 
     * Note: The GLL NMEA sentence is not supported
     * 
     * @param mode  Desired GNSS data stream mode
     * @param gga   Enable/disable GGA NMEA sentence
     * @param gll   Enable/disable GLL NMEA sentence
     * @param gsa   Enable/disable GSA NMEA sentence
     * @param gsv   Enable/disable GSV NMEA sentence
     * @param rmc   Enable/disable RMC NMEA sentence
     * @param vtg   Enable/disable VTG NMEA sentence
     * @return success
     */
    GNSSError configure_gnss_data_stream(ME310NMEAStreamMode mode,
        bool gga = false,
        bool gll = false,
        bool gsa = false,
        bool gsv = false,
        bool rmc = false,
        bool vtg = false);

    /**
     * Use the ME310's AT$GPSNMUNEX command to configure the extended
     * features of the GNSS NMEA stream.
     * 
     * Note: The GPGRS and GNGNS NMEA sentences are not supported
     * 
     * @param gngns Enable/disable GNGNS NMEA sentence
     * @param gngsa Enable/disable GNGSA NMEA sentence
     * @param glgsv Enable/disable GLGSV NMEA sentence
     * @param gpgrs Enable/disable GPGRS NMEA sentence
     * @param gagsv Enable/disable GAGSV NMEA sentence
     * @param gagsa Enable/disable GAGSA NMEA sentence
     * @param gavtg Enable/disable GAVTG NMEA sentence
     * @param gpgga Enable/disable GPGGA NMEA sentence
     * @param pqgsa Enable/disable PQGSA NMEA sentence
     * @param pqgsv Enable/disable PQGSV NMEA sentence
     * @param gnvtg Enable/disable GNVTG NMEA sentence
     * @param gnrmc Enable/disable GNRMC NMEA sentence
     * @param gngga Enable/disable GNGGA NMEA sentence
     * @return success
     */
    GNSSError configure_gnss_data_stream_extended(bool gngns = false,
                                                  bool gngsa = false,
                                                  bool glgsv = false,
                                                  bool gpgrs = false,
                                                  bool gagsv = false,
                                                  bool gagsa = false,
                                                  bool gavtg = false,
                                                  bool gpgga = false,
                                                  bool pqgsa = false,
                                                  bool pqgsv = false,
                                                  bool gnvtg = false,
                                                  bool gnrmc = false,
                                                  bool gngga = false);

    /**
     * GPGGA NMEA sentence URC callback
     */
    void urc_gpgga();

    /**
     * GNRMC NMEA sentence URC callback
     */
    void urc_gnrmc();

    /**
     * Convert year, month, day, hour, min, sec to unix timestamp
     * 
     * @param year  Year
     * @param mon   Month
     * @param mday  Day of the month
     * @param hour  Hours
     * @param min   Minutes
     * @param sec   Seconds
     * @return Unix timestamp
     */
    time_t as_unix_time(int year, int mon, int mday, int hour, int min, int sec);

private:
    CellularDevice *dev;
    ATHandler *at_handler;
    TinyGPSPlus values;
};

}

#endif /* TELIT_ME310_GNSS_ENABLED */

#endif /* EP_OC_MCU_EXTENSIONS_TELIT_ME310_GNSS_H_ */