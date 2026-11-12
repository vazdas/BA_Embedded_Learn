#include "system_events.h"

const char* GetEventString(SystemEvent_t sys_event) {
    switch(sys_event) {
        case SYSTEM_BOOT:               return "System Loading. Please Wait";
        case STATUS_OK:                 return "System STatus: OK";
        case ERR_STATUS_FALSE:          return "System Status: FALSE";
        case ERR_I2C_TIMEOUT:           return "Err: I2C Timeout";
        case ERR_SENSOR_DISCONNECT:     return "Err: Sensor Don't Respond";
        case ERR_SENSOR_FAILURE:        return "Err: Sensor Failure";
        case ERR_LOW_BATTERY:           return "Err: Low Battery";
        case ERR_FAILED_TO_READ_MEMORY: return "Err: Failed to Read Memory";
        case ERR_MOTOR_JAMMED:          return "Err: Motor Jamed";
        default:                        return "Err: Unknown";
    }
}
