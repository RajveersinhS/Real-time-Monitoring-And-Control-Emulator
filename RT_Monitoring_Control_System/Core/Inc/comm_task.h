/**
  ******************************************************************************
  * @file           : comm_task.h
  * @brief          : Communication task header (CAN-style message bus)
  ******************************************************************************
  */

#ifndef COMM_TASK_H
#define COMM_TASK_H

#include "system_types.h"
#include "FreeRTOS.h"
#include "queue.h"

/**
  * @brief  Communication task function.
  *         Receives CAN-style messages from SensorTask,
  *         processes them through the communication bus (FDCAN loopback ready),
  *         and forwards to ProcessingTask.
  * @param  argument: Not used (queues accessed via extern)
  */
void CommTask(void *argument);

#endif /* COMM_TASK_H */
