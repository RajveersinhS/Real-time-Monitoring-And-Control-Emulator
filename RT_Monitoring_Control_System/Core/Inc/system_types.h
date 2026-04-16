/**
  ******************************************************************************
  * @file           : system_types.h
  * @brief          : Shared types, constants, and safety thresholds for the
  *                   RT Monitoring & Control System.
  ******************************************************************************
  */

#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include <stdint.h>

/* ========================== System States ================================= */
typedef enum {
    STATE_NORMAL   = 0,
    STATE_WARNING  = 1,
    STATE_SHUTDOWN = 2
} SystemState_t;

/* ========================== CAN Message IDs =============================== */
#define SENSOR_ID_TEMPERATURE   0x100U
#define SENSOR_ID_PRESSURE      0x200U
#define SENSOR_ID_HEARTBEAT     0x700U

/* ========================== Safety Thresholds ============================= */
/* Temperature thresholds (degrees Celsius) */
#define TEMP_WARNING_HIGH       70.0f
#define TEMP_SHUTDOWN_HIGH      90.0f
#define TEMP_NORMAL_RETURN      65.0f   /* Hysteresis: must drop below this to return to NORMAL */

/* Pressure thresholds (bar) */
#define PRESS_WARNING_HIGH      8.0f
#define PRESS_SHUTDOWN_HIGH     10.0f
#define PRESS_NORMAL_RETURN     7.0f    /* Hysteresis: must drop below this to return to NORMAL */

/* ========================== Task Timing =================================== */
#define SENSOR_TASK_PERIOD_MS   500U
#define LED_BLINK_NORMAL_MS     1000U
#define LED_BLINK_WARNING_MS    250U
#define LOGGING_PERIOD_MS       500U

/* ========================== Queue Sizes =================================== */
#define SENSOR_QUEUE_SIZE       10U
#define PROCESS_QUEUE_SIZE      10U
#define LOG_QUEUE_SIZE          10U

/* ========================== CAN-Style Message Structure =================== */
/**
  * @brief  CAN-style message frame, compatible with FDCAN standard.
  *         Uses standard 11-bit ID format.
  */
typedef struct {
    uint32_t id;            /**< Message ID (CAN-style, 11-bit standard) */
    uint8_t  dlc;           /**< Data Length Code (0-8 bytes) */
    uint8_t  data[8];       /**< Data payload */
    uint32_t timestamp;     /**< System tick when message was created */
} CANMessage_t;

/* ========================== Processed Data ================================ */
/**
  * @brief  Aggregated sensor data after processing.
  */
typedef struct {
    float         temperature;
    float         pressure;
    SystemState_t state;
    SystemState_t previousState;
    uint32_t      timestamp;
} ProcessedData_t;

/* ========================== Log Entry ===================================== */
/**
  * @brief  Log entry sent to the LoggingTask for UART output.
  */
typedef struct {
    SystemState_t state;
    SystemState_t previousState;
    float         temperature;
    float         pressure;
    uint8_t       stateChanged;     /**< 1 if state transition occurred */
    uint32_t      timestamp;
} LogEntry_t;

/* ========================== Helper Macros ================================= */
/**
  * @brief  Encode a float into 4 bytes (little-endian) for CAN data field.
  */
#define FLOAT_TO_CAN_DATA(data_ptr, value) \
    do { \
        union { float f; uint8_t b[4]; } _u; \
        _u.f = (value); \
        (data_ptr)[0] = _u.b[0]; \
        (data_ptr)[1] = _u.b[1]; \
        (data_ptr)[2] = _u.b[2]; \
        (data_ptr)[3] = _u.b[3]; \
    } while(0)

/**
  * @brief  Decode 4 bytes (little-endian) from CAN data field to float.
  */
#define CAN_DATA_TO_FLOAT(data_ptr) \
    ({ union { float f; uint8_t b[4]; } _u; \
       _u.b[0] = (data_ptr)[0]; \
       _u.b[1] = (data_ptr)[1]; \
       _u.b[2] = (data_ptr)[2]; \
       _u.b[3] = (data_ptr)[3]; \
       _u.f; })

#endif /* SYSTEM_TYPES_H */
