/**
  ******************************************************************************
  * @file           : processing_task.c
  * @brief          : Data processing and state machine task
  *
  * This task receives CAN-style messages from the CommTask, extracts
  * sensor data (temperature, pressure), evaluates safety thresholds,
  * manages state machine transitions (NORMAL → WARNING → SHUTDOWN),
  * controls LED indicators, and sends log entries to the LoggingTask.
  ******************************************************************************
  */

#include "processing_task.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/*  External references                                                       */
/* -------------------------------------------------------------------------- */
extern QueueHandle_t commToProcessQueue;
extern QueueHandle_t processToLogQueue;

/* -------------------------------------------------------------------------- */
/*  Private variables                                                         */
/* -------------------------------------------------------------------------- */
static volatile SystemState_t currentState  = STATE_NORMAL;
static volatile uint8_t       resetRequested = 0;

static float lastTemperature = 25.0f;
static float lastPressure    = 3.0f;

/* LED blink timing */
static TickType_t lastLedToggle = 0;

/* -------------------------------------------------------------------------- */
/*  Public functions                                                          */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Get the current system state (thread-safe read of volatile).
  */
SystemState_t GetSystemState(void)
{
    return currentState;
}

/**
  * @brief  Request a state reset from SHUTDOWN.
  *         Called from the user button ISR callback.
  */
void RequestStateReset(void)
{
    resetRequested = 1;
}

/* -------------------------------------------------------------------------- */
/*  Private functions                                                         */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Evaluate safety thresholds and determine the new state.
  * @param  temp: Current temperature reading
  * @param  press: Current pressure reading
  * @retval New system state
  */
static SystemState_t EvaluateSafety(float temp, float press)
{
    SystemState_t newState = currentState;

    switch (currentState)
    {
        case STATE_NORMAL:
            /* Check for WARNING conditions */
            if (temp > TEMP_SHUTDOWN_HIGH || press > PRESS_SHUTDOWN_HIGH)
            {
                newState = STATE_SHUTDOWN;
            }
            else if (temp > TEMP_WARNING_HIGH || press > PRESS_WARNING_HIGH)
            {
                newState = STATE_WARNING;
            }
            break;

        case STATE_WARNING:
            /* Check for SHUTDOWN escalation */
            if (temp > TEMP_SHUTDOWN_HIGH || press > PRESS_SHUTDOWN_HIGH)
            {
                newState = STATE_SHUTDOWN;
            }
            /* Check for return to NORMAL (with hysteresis) */
            else if (temp < TEMP_NORMAL_RETURN && press < PRESS_NORMAL_RETURN)
            {
                newState = STATE_NORMAL;
            }
            break;

        case STATE_SHUTDOWN:
            /* Only exit SHUTDOWN via manual reset (user button) */
            if (resetRequested)
            {
                resetRequested = 0;
                /* Check if conditions permit reset */
                if (temp < TEMP_NORMAL_RETURN && press < PRESS_NORMAL_RETURN)
                {
                    newState = STATE_NORMAL;
                }
                else if (temp < TEMP_SHUTDOWN_HIGH && press < PRESS_SHUTDOWN_HIGH)
                {
                    newState = STATE_WARNING;
                }
                /* If still critical, stay in SHUTDOWN */
            }
            break;

        default:
            newState = STATE_SHUTDOWN;
            break;
    }

    return newState;
}

/**
  * @brief  Update LED indicators based on current system state.
  *         GREEN (PA5):  NORMAL  — slow blink (1s)
  *         YELLOW (LED_YELLOW): WARNING — fast blink (250ms)
  *         RED (LED_RED):    SHUTDOWN — solid ON
  */
static void UpdateLEDs(void)
{
    TickType_t now = xTaskGetTickCount();

    switch (currentState)
    {
        case STATE_NORMAL:
            /* Green LED: slow blink */
            BSP_LED_Off(LED_YELLOW);
            BSP_LED_Off(LED_RED);
            if ((now - lastLedToggle) >= pdMS_TO_TICKS(LED_BLINK_NORMAL_MS))
            {
                BSP_LED_Toggle(LED_GREEN);
                lastLedToggle = now;
            }
            break;

        case STATE_WARNING:
            /* Yellow LED: fast blink, Green OFF */
            BSP_LED_Off(LED_GREEN);
            BSP_LED_Off(LED_RED);
            if ((now - lastLedToggle) >= pdMS_TO_TICKS(LED_BLINK_WARNING_MS))
            {
                BSP_LED_Toggle(LED_YELLOW);
                lastLedToggle = now;
            }
            break;

        case STATE_SHUTDOWN:
            /* Red LED: solid ON, others OFF */
            BSP_LED_Off(LED_GREEN);
            BSP_LED_Off(LED_YELLOW);
            BSP_LED_On(LED_RED);
            break;

        default:
            break;
    }
}

/**
  * @brief  Send a log entry to the LoggingTask.
  * @param  prevState: Previous system state (for transition detection)
  * @param  newState:  Current system state
  * @param  temp:      Current temperature
  * @param  press:     Current pressure
  */
static void SendLogEntry(SystemState_t prevState, SystemState_t newState,
                          float temp, float press)
{
    LogEntry_t entry;
    entry.state         = newState;
    entry.previousState = prevState;
    entry.temperature   = temp;
    entry.pressure      = press;
    entry.stateChanged  = (prevState != newState) ? 1 : 0;
    entry.timestamp     = xTaskGetTickCount();

    /* Non-blocking send — if log queue is full, drop this entry */
    xQueueSend(processToLogQueue, &entry, 0);
}

/* -------------------------------------------------------------------------- */
/*  Task function                                                             */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Processing task.
  *         Receives CAN messages, extracts sensor data, evaluates safety,
  *         manages state transitions, updates LEDs, and sends log entries.
  */
void ProcessingTask(void *argument)
{
    (void)argument;

    CANMessage_t  rxMsg;
    SystemState_t previousState;
    uint8_t       tempUpdated  = 0;
    uint8_t       pressUpdated = 0;

    for (;;)
    {
        /* Wait for a message from CommTask */
        if (xQueueReceive(commToProcessQueue, &rxMsg, pdMS_TO_TICKS(100)) == pdPASS)
        {
            /* Extract sensor data based on message ID */
            switch (rxMsg.id)
            {
                case SENSOR_ID_TEMPERATURE:
                    lastTemperature = CAN_DATA_TO_FLOAT(rxMsg.data);
                    tempUpdated = 1;
                    break;

                case SENSOR_ID_PRESSURE:
                    lastPressure = CAN_DATA_TO_FLOAT(rxMsg.data);
                    pressUpdated = 1;
                    break;

                default:
                    break;
            }

            /* When we have both readings, evaluate the system */
            if (tempUpdated && pressUpdated)
            {
                previousState = currentState;

                /* Evaluate safety and update state */
                currentState = EvaluateSafety(lastTemperature, lastPressure);

                /* Update LED indicators */
                UpdateLEDs();

                /* Send log entry */
                SendLogEntry(previousState, currentState,
                             lastTemperature, lastPressure);

                /* Reset update flags */
                tempUpdated  = 0;
                pressUpdated = 0;
            }
        }
        else
        {
            /* No message received — still update LEDs for blinking effect */
            UpdateLEDs();
        }
    }
}
