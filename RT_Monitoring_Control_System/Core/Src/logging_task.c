/**
  ******************************************************************************
  * @file           : logging_task.c
  * @brief          : UART logging task for real-time system monitoring
  *
  * This task receives log entries from the ProcessingTask and formats
  * them as human-readable output sent via UART2 to a PC terminal.
  *
  * Output format includes:
  *   - System state with transitions highlighted
  *   - Temperature and pressure readings
  *   - Timestamps (in milliseconds since boot)
  *   - ASCII art status bars for visual monitoring
  ******************************************************************************
  */

#include "logging_task.h"
#include "main.h"
#include "stm32h7xx_nucleo.h"
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/*  External references                                                       */
/* -------------------------------------------------------------------------- */
extern QueueHandle_t processToLogQueue;
extern UART_HandleTypeDef hcom_uart[];  /* BSP COM handle array — hcom_uart[COM1] = USART3 = ST-LINK VCP */

/* -------------------------------------------------------------------------- */
/*  Private variables                                                         */
/* -------------------------------------------------------------------------- */
static char uartBuffer[256];
static uint32_t logCount = 0;

/* -------------------------------------------------------------------------- */
/*  Private functions                                                         */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Send a string via UART2 (blocking).
  * @param  str: Null-terminated string to send
  */
static void UART_Print(const char *str)
{
    HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
  * @brief  Get a string representation of the system state.
  * @param  state: System state enum
  * @retval Pointer to state name string
  */
static const char* StateToString(SystemState_t state)
{
    switch (state)
    {
        case STATE_NORMAL:   return "NORMAL  ";
        case STATE_WARNING:  return "WARNING ";
        case STATE_SHUTDOWN: return "SHUTDOWN";
        default:             return "UNKNOWN ";
    }
}

/**
  * @brief  Print a formatted separator line.
  */
static void PrintSeparator(void)
{
    UART_Print("--------------------------------------------------\r\n");
}

/**
  * @brief  Print the system startup banner.
  */
static void PrintBanner(void)
{
    UART_Print("\r\n");
    UART_Print("==================================================\r\n");
    UART_Print("  RT Monitoring & Control System v1.0\r\n");
    UART_Print("  STM32 NUCLEO-H7A3ZI-Q | FreeRTOS\r\n");
    UART_Print("==================================================\r\n");
    UART_Print("  Thresholds:\r\n");

    snprintf(uartBuffer, sizeof(uartBuffer),
             "    Temp  WARNING: >%dC  SHUTDOWN: >%dC\r\n",
             (int)TEMP_WARNING_HIGH, (int)TEMP_SHUTDOWN_HIGH);
    UART_Print(uartBuffer);

    snprintf(uartBuffer, sizeof(uartBuffer),
             "    Press WARNING: >%d.%dbar  SHUTDOWN: >%d.%dbar\r\n",
             (int)PRESS_WARNING_HIGH, (int)(PRESS_WARNING_HIGH * 10.0f) % 10,
             (int)PRESS_SHUTDOWN_HIGH, (int)(PRESS_SHUTDOWN_HIGH * 10.0f) % 10);
    UART_Print(uartBuffer);

    PrintSeparator();
    UART_Print("  [#]  Time(ms)  State     Temp(C)  Press(bar)\r\n");
    PrintSeparator();
}

/**
  * @brief  Print a state transition alert.
  * @param  entry: Log entry containing transition info
  */
static void PrintStateTransition(const LogEntry_t *entry)
{
    UART_Print("\r\n");
    UART_Print("  *** STATE TRANSITION ***\r\n");
    snprintf(uartBuffer, sizeof(uartBuffer),
             "  %s --> %s  @ %lums\r\n",
             StateToString(entry->previousState),
             StateToString(entry->state),
             (unsigned long)entry->timestamp);
    UART_Print(uartBuffer);

    switch (entry->state)
    {
        case STATE_WARNING:
            UART_Print("  [!] Sensor values approaching critical limits!\r\n");
            break;
        case STATE_SHUTDOWN:
            UART_Print("  [X] CRITICAL! System entering SHUTDOWN mode!\r\n");
            UART_Print("  [X] Press USER button to reset when safe.\r\n");
            break;
        case STATE_NORMAL:
            UART_Print("  [OK] System returned to normal operation.\r\n");
            break;
        default:
            break;
    }
    UART_Print("\r\n");
}

/**
  * @brief  Print a regular log entry line.
  * @param  entry: Log entry to format and print
  */
static void PrintLogEntry(const LogEntry_t *entry)
{
    char stateIndicator;
    switch (entry->state)
    {
        case STATE_NORMAL:   stateIndicator = ' '; break;
        case STATE_WARNING:  stateIndicator = '!'; break;
        case STATE_SHUTDOWN: stateIndicator = 'X'; break;
        default:             stateIndicator = '?'; break;
    }

    int temp_int  = (int)entry->temperature;
    int temp_frac = (int)(entry->temperature * 10.0f) % 10;
    if (temp_frac < 0) temp_frac = -temp_frac;

    int press_int  = (int)entry->pressure;
    int press_frac = (int)(entry->pressure * 100.0f) % 100;
    if (press_frac < 0) press_frac = -press_frac;

    snprintf(uartBuffer, sizeof(uartBuffer),
             " %c%4lu  %7lu  %s  %3d.%1d    %2d.%02d\r\n",
             stateIndicator,
             (unsigned long)logCount,
             (unsigned long)entry->timestamp,
             StateToString(entry->state),
             temp_int, temp_frac,
             press_int, press_frac);
    UART_Print(uartBuffer);
}

/* -------------------------------------------------------------------------- */
/*  Task function                                                             */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Logging task.
  *         Receives log entries from ProcessingTask and outputs
  *         formatted monitoring data via UART2.
  */
void LoggingTask(void *argument)
{
    (void)argument;

    LogEntry_t entry;

    /* Print startup banner */
    PrintBanner();

    for (;;)
    {
        /* Wait for a log entry from ProcessingTask */
        if (xQueueReceive(processToLogQueue, &entry, portMAX_DELAY) == pdPASS)
        {
            logCount++;

            /* If state changed, print transition alert */
            if (entry.stateChanged)
            {
                PrintStateTransition(&entry);
            }

            /* Print the regular log line */
            PrintLogEntry(&entry);
        }
    }
}
