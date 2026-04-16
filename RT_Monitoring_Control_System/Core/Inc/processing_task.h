/**
  ******************************************************************************
  * @file           : processing_task.h
  * @brief          : Data processing and state machine task header
  ******************************************************************************
  */

#ifndef PROCESSING_TASK_H
#define PROCESSING_TASK_H

#include "system_types.h"
#include "FreeRTOS.h"
#include "queue.h"

/**
  * @brief  Processing task function.
  *         Receives CAN messages from CommTask, extracts sensor data,
  *         evaluates safety thresholds, manages state machine transitions,
  *         and sends log entries to LoggingTask.
  * @param  argument: Not used (queues accessed via extern)
  */
void ProcessingTask(void *argument);

/**
  * @brief  Get the current system state.
  * @retval SystemState_t current state
  */
SystemState_t GetSystemState(void);

/**
  * @brief  Request a state reset (called from button ISR).
  *         Transitions from SHUTDOWN back to NORMAL.
  */
void RequestStateReset(void);

#endif /* PROCESSING_TASK_H */
