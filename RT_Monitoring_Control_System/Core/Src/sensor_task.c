/**
  ******************************************************************************
  * @file           : sensor_task.c
  * @brief          : Simulated sensor data generation task
  *
  * This task simulates temperature and pressure sensor readings using
  * a pseudo-random walk algorithm. Data is packaged into CAN-style
  * messages and sent to the CommTask via a FreeRTOS queue.
  *
  * Temperature: Starts at 25°C, random walk with drift
  * Pressure:    Starts at 3.0 bar, random walk with drift
  ******************************************************************************
  */

#include "sensor_task.h"
#include "main.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  External references                                                       */
/* -------------------------------------------------------------------------- */
extern QueueHandle_t sensorToCommQueue;

/* -------------------------------------------------------------------------- */
/*  Private variables                                                         */
/* -------------------------------------------------------------------------- */
static float currentTemperature = 25.0f;
static float currentPressure    = 3.0f;
static uint32_t sensorSeed      = 12345U;

/* -------------------------------------------------------------------------- */
/*  Private functions                                                         */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Simple linear congruential pseudo-random number generator.
  *         Provides deterministic but varied sensor simulation.
  * @retval Random value 0 - 32767
  */
static uint16_t PseudoRandom(void)
{
    sensorSeed = (sensorSeed * 1103515245U + 12345U) & 0x7FFFFFFFU;
    return (uint16_t)((sensorSeed >> 16) & 0x7FFFU);
}

/**
  * @brief  Generate a random float in range [-range, +range].
  * @param  range: Maximum absolute deviation
  * @retval Random float in [-range, +range]
  */
static float RandomDeviation(float range)
{
    float normalized = ((float)PseudoRandom() / 32767.0f) * 2.0f - 1.0f;
    return normalized * range;
}

/**
  * @brief  Simulate temperature reading with random walk.
  *         Naturally drifts upward to test warning/shutdown thresholds.
  * @retval Simulated temperature in degrees Celsius
  */
static float SimulateTemperature(void)
{
    /* Random walk with slight upward drift to exercise state machine */
    float drift = 0.15f;   /* Slight upward trend */
    float noise = RandomDeviation(2.0f);

    currentTemperature += drift + noise;

    /* Clamp to realistic bounds */
    if (currentTemperature < -10.0f)  currentTemperature = -10.0f;
    if (currentTemperature > 120.0f)  currentTemperature = 120.0f;

    /* Occasionally add a spike to test warning/shutdown */
    if ((PseudoRandom() % 100) < 3)  /* 3% chance of spike */
    {
        currentTemperature += RandomDeviation(15.0f);
    }

    return currentTemperature;
}

/**
  * @brief  Simulate pressure reading with random walk.
  * @retval Simulated pressure in bar
  */
static float SimulatePressure(void)
{
    float drift = 0.05f;   /* Slight upward trend */
    float noise = RandomDeviation(0.5f);

    currentPressure += drift + noise;

    /* Clamp to realistic bounds */
    if (currentPressure < 0.0f)   currentPressure = 0.0f;
    if (currentPressure > 15.0f)  currentPressure = 15.0f;

    /* Occasionally add a spike */
    if ((PseudoRandom() % 100) < 3)
    {
        currentPressure += RandomDeviation(3.0f);
    }

    return currentPressure;
}

/**
  * @brief  Build a CAN-style message from a sensor reading.
  * @param  msg: Pointer to CAN message structure to fill
  * @param  sensorId: CAN message ID for this sensor
  * @param  value: Sensor reading value
  */
static void BuildSensorMessage(CANMessage_t *msg, uint32_t sensorId, float value)
{
    msg->id  = sensorId;
    msg->dlc = 4;   /* float = 4 bytes */
    memset(msg->data, 0, sizeof(msg->data));
    FLOAT_TO_CAN_DATA(msg->data, value);
    msg->timestamp = xTaskGetTickCount();
}

/* -------------------------------------------------------------------------- */
/*  Task function                                                             */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Sensor simulation task.
  *         Runs periodically, generates temp & pressure readings,
  *         packages them as CAN messages, sends to CommTask queue.
  */
void SensorTask(void *argument)
{
    (void)argument;

    CANMessage_t tempMsg;
    CANMessage_t pressMsg;
    TickType_t   lastWakeTime = xTaskGetTickCount();

    /* Seed the RNG with the current tick for variation across runs */
    sensorSeed += xTaskGetTickCount();

    for (;;)
    {
        /* Generate simulated sensor readings */
        float temperature = SimulateTemperature();
        float pressure    = SimulatePressure();

        /* Package into CAN-style messages */
        BuildSensorMessage(&tempMsg,  SENSOR_ID_TEMPERATURE, temperature);
        BuildSensorMessage(&pressMsg, SENSOR_ID_PRESSURE,    pressure);

        /* Send to CommTask queue (don't block if full — drop oldest-style) */
        xQueueSend(sensorToCommQueue, &tempMsg,  pdMS_TO_TICKS(10));
        xQueueSend(sensorToCommQueue, &pressMsg, pdMS_TO_TICKS(10));

        /* Wait for next period */
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
    }
}
