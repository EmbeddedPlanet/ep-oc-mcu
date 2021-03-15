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

// Only compile this if the config is enabled
#if TELIT_ME310_GNSS_ENABLED

#include "TELIT_ME310_GNSS.h"
#include "libnmea/src/parsers/parse.h"
#include "TinyGPSplus.h"

#define TRACE_GROUP   "GNSS"

using namespace ep;
using namespace std::chrono_literals;

TELIT_ME310_GNSS::TELIT_ME310_GNSS()
{
    dev = CellularDevice::get_target_default_instance();
    at_handler = dev->get_at_handler();

    // Set up URC callbacks
    at_handler->set_urc_handler(GPGGA_SENTENCE_URC_PREFIX, mbed::Callback<void()>(this, &TELIT_ME310_GNSS::urc_gpgga));
    at_handler->set_urc_handler(GNRMC_SENTENCE_URC_PREFIX, mbed::Callback<void()>(this, &TELIT_ME310_GNSS::urc_gnrmc));
}

void TELIT_ME310_GNSS::urc_gpgga()
{
    char sentence[100];
    strcpy(sentence, GPGGA_SENTENCE_URC_PREFIX);

    at_handler->lock();

    // Temporarily set delimiter to '\n' to grab the whole sentence
    at_handler->set_delimiter('\n');

    // Offset by the length of the sentence prefix
    at_handler->read_string(sentence + strlen(GPGGA_SENTENCE_URC_PREFIX), sizeof(sentence) - strlen(GPGGA_SENTENCE_URC_PREFIX));

    // Reset the delimiter
    at_handler->set_default_delimiter();
    at_handler->unlock();

    // Loop through the sentence and pass it to the TinyGPSPlus library
    char *iterator;
    for (iterator = sentence; *iterator != '\0'; iterator++) {
        values.encode(*iterator);
    }
}

void TELIT_ME310_GNSS::urc_gnrmc()
{
    char sentence[100];
    strcpy(sentence, GNRMC_SENTENCE_URC_PREFIX);

    at_handler->lock();

    // Temporarily set delimiter to '\n' to grab the whole sentence
    at_handler->set_delimiter('\n');

    // Offset by the length of the sentence prefix
    at_handler->read_string(sentence + strlen(GNRMC_SENTENCE_URC_PREFIX), sizeof(sentence) - strlen(GNRMC_SENTENCE_URC_PREFIX));

    // Reset the delimiter
    at_handler->set_default_delimiter();
    at_handler->unlock();

    // Loop through the sentence and pass it to the TinyGPSPlus library
    char *iterator;
    for (iterator = sentence; *iterator != '\0'; iterator++) {
        values.encode(*iterator);
    }
}

void TELIT_ME310_GNSS::init()
{
    // Check if the device is ready
    if (dev->is_ready() != NSAPI_ERROR_OK) {
        dev->soft_power_on();
        ThisThread::sleep_for(10s);
        dev->init();
    }
}

GNSS::GNSSError TELIT_ME310_GNSS::enable()
{
    at_handler->lock();

    at_handler->at_cmd_discard("$GPSP", "=", "%d", 1);
    if (at_handler->unlock_return_error() == NSAPI_ERROR_OK) {
        return GNSS_ERROR_OK;
    }

    return GNSS_ERROR_UNKNOWN_ERROR;
}

GNSS::GNSSError TELIT_ME310_GNSS::disable()
{
    at_handler->lock();

    at_handler->at_cmd_discard("$GPSP", "=", "%d", 0);
    if (at_handler->unlock_return_error() == NSAPI_ERROR_OK) {
        return GNSS_ERROR_OK;
    }

    return GNSS_ERROR_UNKNOWN_ERROR;
}

GNSS::PositionInfo TELIT_ME310_GNSS::get_current_position()
{
    PositionInfo position_info;

    // Fix is valid, so fill in the data values
    // Latitude
    nmea_position latitude_pos;
    latitude_pos.degrees = abs(values.location.rawLat().deg);
    latitude_pos.minutes = (values.location.rawLat().billionths / 1000000000.0) * 60; // Convert to minutes
    latitude_pos.cardinal = values.location.rawLat().negative ? NMEA_CARDINAL_DIR_SOUTH : NMEA_CARDINAL_DIR_NORTH;
    position_info.Latitude = latitude_pos;

    // Longitude
    nmea_position longitude_pos;
    longitude_pos.degrees = abs(values.location.rawLng().deg);
    longitude_pos.minutes = (values.location.rawLng().billionths / 1000000000.0) * 60; // Convert to minutes
    longitude_pos.cardinal = values.location.rawLng().negative ? NMEA_CARDINAL_DIR_WEST : NMEA_CARDINAL_DIR_EAST;
    position_info.Longitude = longitude_pos;

    // Horizontal dilution of precision
    position_info.HorizontalDilutionOfPrecision = values.hdop.value() / 100.0;

    // Altitude
    position_info.Altitude = values.altitude.meters();

    // Fix type (fix is 3D if altitude value is valid)
    if(values.location.isValid()) {
        position_info.Fix = values.altitude.isValid() ? FIX_TYPE_3D : FIX_TYPE_2D;
    } else {
        /* In this case, only some values will be valid (eg: Satellites in View) */
        position_info.Fix = FIX_TYPE_INVALID;
    }

    // Course over ground
    position_info.CourseOverGround = values.course.deg();

    // Speed over ground
    position_info.SpeedOverGround = values.speed.kmph();

    // Number of satellites
    if(values.satellites.isValid()) {
        position_info.NumberOfSatellites = values.satellites.value();
    } else {
        position_info.NumberOfSatellites = 0;
    }

    // Timestamp
    position_info.UtcTimestamp = as_unix_time(
                                    values.date.year(),
                                    values.date.month(),
                                    values.date.day(),
                                    values.time.hour(),
                                    values.time.minute(),
                                    values.time.second());

    return position_info;
}

GNSS::GNSSError TELIT_ME310_GNSS::set_gnss_priority(GNSSPriority desired_priority)
{
    at_handler->lock();

    at_handler->at_cmd_discard("$GPSCFG", "=", "%d%d", GPSCFG_SET_WWAN_GNSS_PRIORITY, desired_priority);
    if (at_handler->unlock_return_error() == NSAPI_ERROR_OK) {
        return GNSS_ERROR_OK;
    }

    return GNSS_ERROR_UNKNOWN_ERROR;
}

GNSS::GNSSError TELIT_ME310_GNSS::set_edrx_parameters(ME310eDRXMode mode, ME310eDRXAcT access_technology, char *req_edrx, char *req_pag_time_window)
{
    at_handler->lock();

    at_handler->at_cmd_discard("#CEDRXS", "=", "%d%d%s%s",
        mode, access_technology, req_edrx, req_pag_time_window);
    if (at_handler->unlock_return_error() == NSAPI_ERROR_OK) {
        return GNSS_ERROR_OK;
    }

    return GNSS_ERROR_UNKNOWN_ERROR;
}

GNSS::GNSSError TELIT_ME310_GNSS::configure_gnss_data_stream(ME310NMEAStreamMode mode,
    bool gga, bool gll, bool gsa, bool gsv, bool rmc, bool vtg)
{
    at_handler->lock();

    at_handler->at_cmd_discard("$GPSNMUN", "=", "%d%d%d%d%d%d%d", mode, gga, gll, gsa, gsv, rmc, vtg);
    if (at_handler->unlock_return_error() == NSAPI_ERROR_OK) {
        return GNSS_ERROR_OK;
    }

    return GNSS_ERROR_UNKNOWN_ERROR;
}

GNSS::GNSSError TELIT_ME310_GNSS::configure_gnss_data_stream_extended(bool gngns, bool gngsa, bool glgsv, bool gpgrs,
    bool gagsv, bool gagsa, bool gavtg, bool gpgga, bool pqgsa, bool pqgsv, bool gnvtg, bool gnrmc, bool gngga)
{
    at_handler->lock();

    at_handler->at_cmd_discard("$GPSNMUNEX", "=", "%d%d%d%d%d%d%d%d%d%d%d%d%d",
        gngns,
        gngsa,
        glgsv,
        gpgrs,
        gagsv,
        gagsa,
        gavtg,
        gpgga,
        pqgsa,
        pqgsv,
        gnvtg,
        gnrmc,
        gngga);
    if (at_handler->unlock_return_error() == NSAPI_ERROR_OK) {
        return GNSS_ERROR_OK;
    }

    return GNSS_ERROR_UNKNOWN_ERROR;
}

time_t TELIT_ME310_GNSS::as_unix_time(int year, int mon, int mday, int hour, int min, int sec)
{
    struct tm   t;
    t.tm_year = year - 1900;
    t.tm_mon =  mon - 1;        // convert to 0 based month
    t.tm_mday = mday;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    t.tm_isdst = -1;            // Is Daylight saving time on? 1 = yes, 0 = no, -1 = unknown

    return mktime(&t);          // returns seconds elapsed since January 1, 1970 (begin of the Epoch)
}


#endif /* TELIT_ME310_GNSS_ENABLED */
