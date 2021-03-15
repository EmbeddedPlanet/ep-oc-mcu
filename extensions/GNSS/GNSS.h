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

#ifndef EP_OC_MCU_EXTENSIONS_GNSS_H_
#define EP_OC_MCU_EXTENSIONS_GNSS_H_

#include "mbed.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nmea.h"

namespace ep
{

/**
 * A class to expose generic GNSS functionality.
 */
class GNSS {
public:

    /**
     * Supported features by the GNSS device
     * 
     * NOTE! These are used as index to feature table, so the only allowed modification to this is appending
     *       to the end (just before PROPERTY_MAX). Do not modify any of the existing fields.
     */
    enum GNSSProperty {
        PROPERTY_HORIZONTAL_DILUTION_OF_PRECISION,  // Enable the Horizontal Dilution of Precision (hdop) property
        PROPERTY_COURSE_OVER_GROUND,                // Enable the Course Over Ground (cog) property
        PROPERTY_SPEED_OVER_GROUND,                 // Enable the Speed Over Ground property
        PROPERTY_NUMBER_OF_SATELLITES,              // Enable the number of satellites property
        PROPERTY_MAX
    };

    /**
     * GNSS specific errors
     */
    enum GNSSError {
        GNSS_ERROR_OK = 0,
        GNSS_ERROR_NOT_ENABLED,
        GNSS_ERROR_NOT_SUPPORTED,
        GNSS_ERROR_UNKNOWN_ERROR
    };

    /**
     * GNSS fix type
     */
    enum FixType {
        FIX_TYPE_INVALID,
        FIX_TYPE_2D,
        FIX_TYPE_3D,
        FIX_TYPE_UNKNOWN
    };

    /**
     * GNSS position information
     */
    struct PositionInfo {
        time_t UtcTimestamp;                    // Date and time in Unix timestamp format
        nmea_position Latitude;                 // NMEA latitude position
        nmea_position Longitude;                // NMEA longitude position
        float HorizontalDilutionOfPrecision;    // Horizontal Dilution of Precision (hdop)
        float Altitude;                         // Altitude - mean-sea-level (geoid) in meters
        FixType Fix;                            // Type of fix
        float CourseOverGround;                 // Course over ground
        float SpeedOverGround;                  // Speed over ground (km/hr)
        uint8_t NumberOfSatellites;             // Total number of satellites in use
    };

    /**
     * Default constructor
     */
    GNSS();

    /**
     * Default destructor
     */
    virtual ~GNSS();

    /**
     * Enable the GNSS device
     */
    virtual GNSSError enable();

    /**
     * Disable the GNSS device
     */
    virtual GNSSError disable();

    /**
     * Retrieve the current position
     * 
     * @return The current position info
     */
    virtual PositionInfo get_current_position();

private:
    const intptr_t *_property_array;
};

}

#endif /* EP_OC_MCU_EXTENSIONS_GNSS_H_ */