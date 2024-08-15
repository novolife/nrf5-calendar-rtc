/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef __NRF_CALENDAR_H__
#define __NRF_CALENDAR_H__

#include <stdint.h>
#include <stdbool.h>
#include "time.h"

typedef struct {
    uint8_t int_interval;       // in seconds
    void    (*callback)(void);  // callback handler
} nrf_cal_def_t;

// Initializes the calendar library. Run this before calling any other functions. 
void nrf_cal_init(nrf_cal_def_t *config);
void nrf_cal_uninit(void);

// Enables a callback feature in the calendar library that can call a function automatically at the specified interval (seconds).
void nrf_cal_set_callback(void (*callback)(void), uint32_t interval);

// Sets the date and time stored in the calendar library. 
// When this function is called a second time calibration data will be automatically generated based on the error in time since the
// last call to the set time function. To ensure good calibration this function should not be called too often 
// (depending on the accuracy of the 32 kHz clock it should be sufficient to call it between once a week and once a month). 
void nrf_cal_set_time(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second);


// Returns time as a tm struct. If no calibration data is available it will return the uncalibrated time.
// For more information about the tm struct and the time.h library in general please refer to:
// http://www.tutorialspoint.com/c_standard_library/time_h.htm
struct tm *nrf_cal_get_time(bool calibrated);

// Returns a string for printing the date and time. Turn the calibration on/off by setting the calibrate parameter. 
char *nrf_cal_get_time_string(bool calibrated);

#endif
