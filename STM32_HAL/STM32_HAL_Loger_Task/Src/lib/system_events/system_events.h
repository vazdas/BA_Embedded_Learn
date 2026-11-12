#ifndef SYSTEM_EVENTS_H
#define SYSTEM_EVENTS_H

typedef enum {
    SYSTEM_BOOT = 0,
    STATUS_OK,
    ERR_STATUS_FALSE,
    ERR_I2C_TIMEOUT,
    ERR_SENSOR_DISCONNECT,
    ERR_SENSOR_FAILURE,
    ERR_LOW_BATTERY,
    ERR_FAILED_TO_READ_MEMORY,
    ERR_MOTOR_JAMMED,
} SystemEvent_t;

const char* GetEventString(SystemEvent_t sys_event);

#endif // SYSTEM_EVENTS_H
