/**
 * ep-oc-mcu
 * Embedded Planet Open Core for Microcontrollers
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2019-2020 Embedded Planet, Inc.
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

#include "ResistorDivider.h"

#include "platform/mbed_assert.h"

using namespace ep;

ResistorDivider::ResistorDivider(mbed::AnalogIn* adc_in, float r_pd, float r_pu,
        float vin_volts) : adc_in(adc_in), r_pu(r_pu), r_pd(r_pd), vin_volts(vin_volts) {

    /** Exactly 2 of the given parameters must be > 0.0f (ie: they are known/fixed parameters) */
    MBED_ASSERT(((r_pd <= 0.0f) + (r_pu <= 0.0f) + (vin_volts <= 0.0f)) == 1);

}

float ResistorDivider::get_R_pu_ohms(void) {
    // Return known value, if available
    if(r_pu >= 0.0f) {
        return r_pu;
    } else {
        // Calculate pu
#if MBED_MAJOR_VERSION == 5
        return (r_pd * ((vin_volts / (adc_in->read() * MBED_CONF_TARGET_DEFAULT_ADC_VREF))));
#else
        return (r_pd * ((vin_volts / adc_in->read_voltage()) - 1.0f));
#endif
    }
}

float ResistorDivider::get_R_pd_ohms(void) {
    // Return known value, if available
    if(r_pd >= 0.0f) {
        return r_pd;
    } else {
        // Calculate pd
#if MBED_MAJOR_VERSION == 5
        return (r_pu * (1.0f / ((vin_volts / (adc_in->read() * MBED_CONF_TARGET_DEFAULT_ADC_VREF)) - 1.0f )));
#else
        return (r_pu * (1.0f / ((vin_volts / adc_in->read_voltage()) - 1.0f)));
#endif
    }
}

float ResistorDivider::get_Vin_volts(void) {
    // Return known value, if available
    if(vin_volts >= 0.0f) {
        return vin_volts;
    } else {
        // Calculate vin_volts
#if MBED_MAJOR_VERSION == 5
        return (((r_pu + r_pd) / r_pd) * (adc_in->read() * MBED_CONF_TARGET_DEFAULT_ADC_VREF));
#else
        return (((r_pu + r_pd) / r_pd) * adc_in->read_voltage());
#endif
    }
}
