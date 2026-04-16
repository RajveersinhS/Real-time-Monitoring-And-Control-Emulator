/**
  ******************************************************************************
  * @file           : sensor_task.h
  * @brief          : Sensor simulation task header
  ******************************************************************************
  */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "system_types.h"
#include "FreeRTOS.h"
#include "queue.h"

/**
  * @brief  Sensor simulation task function.
  *         Generates simulated temperature and pressure readings,
  *         packages them into CAN-style messages, and sends to CommTask.
  * @param  argument: Pointer to the output queue (QueueHandle_t)
  */
void SensorTask(void *argument);

#endif /* SENSOR_TASK_H */
