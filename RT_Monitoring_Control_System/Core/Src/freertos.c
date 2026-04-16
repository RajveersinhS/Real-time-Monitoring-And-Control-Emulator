/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : FreeRTOS task creation, queue setup, and RTOS
  *                      initialization for the RT Monitoring & Control System.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "system_types.h"
#include "sensor_task.h"
#include "comm_task.h"
#include "processing_task.h"
#include "logging_task.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* Task handles */
TaskHandle_t sensorTaskHandle;
TaskHandle_t commTaskHandle;
TaskHandle_t processingTaskHandle;
TaskHandle_t loggingTaskHandle;

/* Inter-task communication queues */
QueueHandle_t sensorToCommQueue;     /* SensorTask → CommTask */
QueueHandle_t commToProcessQueue;    /* CommTask → ProcessingTask */
QueueHandle_t processToLogQueue;     /* ProcessingTask → LoggingTask */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization.
  *         Creates all inter-task queues and application tasks.
  */
void MX_FREERTOS_Init(void)
{
    /* ====================================================================== */
    /*  Create inter-task communication queues                                */
    /* ====================================================================== */

    sensorToCommQueue  = xQueueCreate(SENSOR_QUEUE_SIZE,  sizeof(CANMessage_t));
    commToProcessQueue = xQueueCreate(PROCESS_QUEUE_SIZE, sizeof(CANMessage_t));
    processToLogQueue  = xQueueCreate(LOG_QUEUE_SIZE,     sizeof(LogEntry_t));

    /* Verify all queues were created successfully */
    configASSERT(sensorToCommQueue  != NULL);
    configASSERT(commToProcessQueue != NULL);
    configASSERT(processToLogQueue  != NULL);

    /* ====================================================================== */
    /*  Create application tasks                                              */
    /*                                                                        */
    /*  Priority hierarchy:                                                   */
    /*    3 (Highest) - CommTask, ProcessingTask (time-critical data path)    */
    /*    2           - SensorTask (periodic data generation)                 */
    /*    1 (Lowest)  - LoggingTask (non-critical output)                    */
    /* ====================================================================== */

    xTaskCreate(SensorTask,
                "SensorTask",
                256,            /* Stack size in words */
                NULL,
                2,              /* Priority: Normal */
                &sensorTaskHandle);

    xTaskCreate(CommTask,
                "CommTask",
                256,            /* Stack size in words */
                NULL,
                3,              /* Priority: Above Normal */
                &commTaskHandle);

    xTaskCreate(ProcessingTask,
                "ProcessTask",
                512,            /* Stack size in words (needs space for float ops) */
                NULL,
                3,              /* Priority: Above Normal */
                &processingTaskHandle);

    xTaskCreate(LoggingTask,
                "LogTask",
                512,            /* Stack size in words (needs space for snprintf) */
                NULL,
                1,              /* Priority: Below Normal (non-critical) */
                &loggingTaskHandle);

    /* Verify all tasks were created successfully */
    configASSERT(sensorTaskHandle     != NULL);
    configASSERT(commTaskHandle       != NULL);
    configASSERT(processingTaskHandle != NULL);
    configASSERT(loggingTaskHandle    != NULL);
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
