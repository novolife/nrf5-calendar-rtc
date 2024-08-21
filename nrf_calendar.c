#include "sdk_common.h"
#if NRF_MODULE_ENABLED(CALENDAR)

#include "nrf_calendar.h"
#include "nrf.h"
#include "drv_rtc.h"
 
#define NRF_LOG_MODULE_NAME calendar
#if CALENDAR_CONFIG_LOG_ENABLED
    #define NRF_LOG_LEVEL       CALENDAR_CONFIG_LOG_LEVEL
    #define NRF_LOG_INFO_COLOR  CALENDAR_CONFIG_INFO_COLOR
    #define NRF_LOG_DEBUG_COLOR CALENDAR_CONFIG_DEBUG_COLOR
#else //CALENDAR_CONFIG_LOG_ENABLED
    #define NRF_LOG_LEVEL       0
#endif //CALENDAR_CONFIG_LOG_ENABLED
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

static drv_rtc_t nrf_cal_inst = DRV_RTC_INSTANCE(CALENDAR_CONFIG_RTC_INSTANCE); // Should be 1 or 2

#if  CALENDAR_TIME_AS_MILLISECONDS
    static uint64_t m_time, m_last_calibrate_time = 0;
#else
    static time_t m_time, m_last_calibrate_time = 0;
#endif

static struct tm m_tm_return_time; 
static float m_calibrate_factor = 0.0f;
static uint32_t m_rtc_increment = 60;
static void (*cal_event_callback)(void) = 0;



static void print_time(struct tm *time)
{
    char cal_string[80];
    strftime(cal_string, 80, "%F %T", time);
    NRF_LOG_INFO("utc time: %s\n", cal_string);
}


static struct tm *get_time_raw(void)
{
    time_t return_time;
#if  CALENDAR_TIME_AS_MILLISECONDS
    return_time = (uint32_t)(m_time / 1024ULL);
#else
    return_time = m_time;
#endif

    m_tm_return_time = *localtime(&return_time);
    return &m_tm_return_time;
}


static struct tm *get_time_calibrated(void)
{
    time_t ret;
#if  CALENDAR_TIME_AS_MILLISECONDS
    uint64_t uncalibrated_time, calibrated_time;
#else
    time_t uncalibrated_time, calibrated_time;
#endif

    if (m_calibrate_factor != 0.0f) {
        uncalibrated_time = m_time;

#if  CALENDAR_TIME_AS_MILLISECONDS
        calibrated_time = m_last_calibrate_time + (uint64_t)((float)(uncalibrated_time - m_last_calibrate_time) * m_calibrate_factor + 0.5f);
        ret = (uint32_t)(calibrated_time / 1024ULL);
#else
        calibrated_time = m_last_calibrate_time + (time_t)((float)(uncalibrated_time - m_last_calibrate_time) * m_calibrate_factor + 0.5f);
        ret = calibrated_time;
#endif

        m_tm_return_time = *localtime(&ret);
        return &m_tm_return_time;
    } else {
        return get_time_raw();
    }
}


static void set_interval(size_t interval)
{
#if CALENDAR_TIME_AS_MILLISECONDS
    drv_rtc_compare_set(&nrf_cal_inst, 
                        CALENDAR_CONFIG_RTC_INSTANCE, 
                        interval * 1024ULL, 
                        true);
#else
    drv_rtc_compare_set(&nrf_cal_inst, 
                        CALENDAR_CONFIG_RTC_INSTANCE, 
                        interval * (32768 / (CALENDAR_CONFIG_RTC_FREQUENCY + 1)), 
                        true);
#endif
}


static void calendar_handler(drv_rtc_t const *const p_instance)
{
    drv_rtc_compare_pending(p_instance, CALENDAR_CONFIG_RTC_INSTANCE);
    nrf_rtc_task_trigger(nrf_cal_inst.p_reg, NRF_RTC_TASK_CLEAR);
    m_time += m_rtc_increment;

#if  CALENDAR_TIME_AS_MILLISECONDS

    if (!(m_time % 1024ULL) && cal_event_callback)
#else
    if (cal_event_callback)
#endif
    {
        cal_event_callback();
    }
}



void nrf_cal_init(nrf_cal_def_t *config)
{
    drv_rtc_config_t m_config = {
        .prescaler          = CALENDAR_CONFIG_RTC_FREQUENCY,
        .interrupt_priority = CALENDAR_CONFIG_IRQ_PRIORITY
    };

    m_rtc_increment = config->int_interval;
    cal_event_callback = config->callback;

    APP_ERROR_CHECK(drv_rtc_init(&nrf_cal_inst, &m_config, calendar_handler));
    drv_rtc_overflow_enable(&nrf_cal_inst, true);
    set_interval(config->int_interval);
    drv_rtc_start(&nrf_cal_inst);
}


void nrf_cal_uninit(void)
{
    drv_rtc_uninit(&nrf_cal_inst);
}


void nrf_cal_set_callback(void (*callback)(void), uint32_t interval)
{
    // Set the calendar callback, and set the callback interval in seconds
    cal_event_callback = callback;
    m_rtc_increment = interval;
    set_interval(interval);
}


void nrf_cal_set_time(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second)
{
#if  CALENDAR_TIME_AS_MILLISECONDS
    uint64_t uncal_difftime, difftime, newtime;
#else
    time_t uncal_difftime, difftime, newtime;
#endif
    struct tm time_struct;
 
    time_struct.tm_year = year - 1900;
    time_struct.tm_mon = month;
    time_struct.tm_mday = day;
    time_struct.tm_hour = hour;
    time_struct.tm_min = minute;
    time_struct.tm_sec = second;   
 
    print_time(&time_struct);
 
#if  CALENDAR_TIME_AS_MILLISECONDS
    newtime = (uint64_t)mktime(&time_struct);
    newtime *= 1024ULL;
#else
    newtime = mktime(&time_struct);
#endif

    nrf_rtc_task_trigger(nrf_cal_inst.p_reg, NRF_RTC_TASK_CLEAR);

    // Calculate the calibration offset
    if (m_last_calibrate_time != 0) {
        difftime = newtime - m_last_calibrate_time;
        uncal_difftime = m_time - m_last_calibrate_time;
        m_calibrate_factor = (float)difftime / (float)uncal_difftime;
    }

    // Assign the new time to the local time variables
    m_time = m_last_calibrate_time = newtime;
}    


struct tm *nrf_cal_get_time(bool calibrated)
{
    return (calibrated ? get_time_calibrated() : get_time_raw());
}


char *nrf_cal_get_time_string(bool calibrated)
{
    static char cal_string[80];
    strftime(cal_string, 80, "%F %T", nrf_cal_get_time(calibrated));
    return cal_string;
}
 
#endif // NRF_MODULE_ENABLED(CALENDAR)
