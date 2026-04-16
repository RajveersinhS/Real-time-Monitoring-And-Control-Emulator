/**
  ******************************************************************************
  * @file           : logging_task.h
  * @brief          : UART logging task header
  ******************************************************************************
  */

#ifndef LOGGING_TASK_H
#define LOGGING_TASK_H

#include "system_types.h"
#include "FreeRTOS.h"
#include "queue.h"

/**
  * @brief  Logging task function.
  *         Receives log entries from ProcessingTask and outputs
  *         formatted data via UART2 to a PC terminal.
  * @param  argument: Not used (queues accessed via extern)
  */
void LoggingTask(void *argument);

#endif /* LOGGING_TASK_H */
