/**
  ******************************************************************************
  * @file           : comm_task.c
  * @brief          : CAN-style communication bus task
  *
  * This task acts as the communication bus layer. It receives CAN-style
  * messages from the SensorTask, validates them, and forwards them to
  * the ProcessingTask.
  *
  * Currently uses FreeRTOS queues to simulate the CAN bus.
  * The structure is designed so that when FDCAN hardware is enabled
  * (via CubeMX), the actual FDCAN TX/RX can replace the queue-based
  * loopback with minimal code changes.
  ******************************************************************************
  */

#include "comm_task.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/*  External references                                                       */
/* -------------------------------------------------------------------------- */
extern QueueHandle_t sensorToCommQueue;
extern QueueHandle_t commToProcessQueue;

/* -------------------------------------------------------------------------- */
/*  Private variables                                                         */
/* -------------------------------------------------------------------------- */
static uint32_t msgReceivedCount = 0;
static uint32_t msgForwardedCount = 0;
static uint32_t msgDroppedCount = 0;

/*
 * ============================================================================
 * FDCAN HARDWARE INTEGRATION (for future use)
 * ============================================================================
 *
 * When FDCAN1 is enabled in CubeMX with Internal Loopback mode:
 *
 * 1. Uncomment HAL_FDCAN_MODULE_ENABLED in stm32h7xx_hal_conf.h
 * 2. Add the following to this file:
 *
 *    extern FDCAN_HandleTypeDef hfdcan1;
 *
 *    static void FDCAN_TransmitMessage(CANMessage_t *msg)
 *    {
 *        FDCAN_TxHeaderTypeDef txHeader;
 *        txHeader.Identifier = msg->id;
 *        txHeader.IdType = FDCAN_STANDARD_ID;
 *        txHeader.TxFrameType = FDCAN_DATA_FRAME;
 *        txHeader.DataLength = FDCAN_DLC_BYTES_8;
 *        txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
 *        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
 *        txHeader.FDFormat = FDCAN_CLASSIC_CAN;
 *        txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
 *        txHeader.MessageMarker = 0;
 *
 *        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, msg->data);
 *    }
 *
 * ============================================================================
 */

/* -------------------------------------------------------------------------- */
/*  Private functions                                                         */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Validate a CAN-style message.
  * @param  msg: Pointer to CAN message to validate
  * @retval 1 if valid, 0 if invalid
  */
static uint8_t ValidateMessage(const CANMessage_t *msg)
{
    /* Check for valid sensor IDs */
    if (msg->id != SENSOR_ID_TEMPERATURE &&
        msg->id != SENSOR_ID_PRESSURE &&
        msg->id != SENSOR_ID_HEARTBEAT)
    {
        return 0;
    }

    /* Check DLC is within bounds */
    if (msg->dlc > 8)
    {
        return 0;
    }

    return 1;
}

/**
  * @brief  Simulate CAN bus loopback transmission.
  *         In the future, this will use actual FDCAN hardware.
  *         For now, it validates and forwards via FreeRTOS queue.
  * @param  msg: Pointer to CAN message to transmit
  * @retval 1 if successfully forwarded, 0 if failed
  */
static uint8_t SimulateCANLoopback(const CANMessage_t *msg)
{
    /*
     * FDCAN Loopback simulation:
     * In a real FDCAN loopback, the message is sent on the TX side
     * and received on the RX side internally. Here we simulate this
     * by directly forwarding the validated message to the process queue.
     */

    if (xQueueSend(commToProcessQueue, msg, pdMS_TO_TICKS(10)) == pdPASS)
    {
        msgForwardedCount++;
        return 1;
    }
    else
    {
        msgDroppedCount++;
        return 0;
    }
}

/* -------------------------------------------------------------------------- */
/*  Task function                                                             */
/* -------------------------------------------------------------------------- */

/**
  * @brief  Communication task.
  *         Receives CAN messages from SensorTask queue, validates them,
  *         and forwards them through the simulated CAN bus to ProcessingTask.
  */
void CommTask(void *argument)
{
    (void)argument;

    CANMessage_t rxMsg;

    for (;;)
    {
        /* Wait for a message from the SensorTask */
        if (xQueueReceive(sensorToCommQueue, &rxMsg, portMAX_DELAY) == pdPASS)
        {
            msgReceivedCount++;

            /* Validate the message */
            if (ValidateMessage(&rxMsg))
            {
                /* Simulate CAN bus loopback (forward to processing) */
                SimulateCANLoopback(&rxMsg);
            }
            else
            {
                msgDroppedCount++;
            }
        }
    }
}
