# Real-Time Monitoring and Control Embedded System

![STM32](https://img.shields.io/badge/Microcontroller-STM32H7A3ZI--Q-blue.svg)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green.svg)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-lightgrey.svg)
![Status](https://img.shields.io/badge/Status-Completed-success.svg)

This project is a real-time, mission-critical monitoring and control system built for the **STM32 NUCLEO-H7A3ZI-Q** board. It implements a multi-threaded architecture using **FreeRTOS** to concurrently generate, route, process, and log sensor data, simulating real-world industrial fault-detection and safety environments.

## 🚀 Key Features

- **Multi-Tasking RTOS Architecture:** Implements 4 concurrent FreeRTOS tasks (Sensor, Comm, Processing, Logging) ensuring deterministic real-time operations.
- **Inter-Task Communication:** Utilizes Thread-safe FreeRTOS Queues for data piping to prevent race conditions.
- **State Machine Safety Logic:** Includes Hysteresis-backed transitions handling multi-tiered threshold limits (`NORMAL` → `WARNING` → `SHUTDOWN`).
- **CAN-Ready Message Bus:** Formats internal communications into standard CAN frame structures (`CANMessage_t` with ID and DLC), built ready for FDCAN loopback/hardware integration.
- **Hardware Integration:** Employs onboard BSP LEDs for visual status indication and EXTI User Button interrupts for manual system resets.
- **Formatted Telemetry output:** Advanced UART logging to a PC terminal featuring ASCII tables and dynamic system state change alerts—achieved without massive standard-library float dependency.

---

## 🛠️ Hardware Requirements

*   **Board:** STMicroelectronics NUCLEO-H7A3ZI-Q
*   **CPU:** Cortex-M7
*   **Peripherals Used:** 
    *   USART3 (Configured to 115200 Baud, 8N1 for USB ST-LINK VCP)
    *   GPIO (LEDs: Green/Yellow/Red, Push Button: Blue USER Button)

---

## 🧠 System Architecture

Instead of a monolithic `while(1)` super-loop, the system utilizes a modern pipeline design separated into modular responsibilities:

1. **Sensor Task (Priority: 2)**
   - Wakes every 500ms.
   - Generates simulated Temperature and Pressure readings using a custom persistent pseudo-random walk algorithm.
   - Packages data into CAN frames and queues them to the Comm Task.
2. **Comm Task (Priority: 3)**
   - Acts as the simulated CAN Bus Node. Validates `rxMsg.id` and message lengths.
   - Currently loops verified frames into the processing pipeline, designed explicitly as a drop-in replacement location for future `HAL_FDCAN` TX/RX hardware interrupts.
3. **Processing Task (Priority: 3 - Brains)**
   - Evaluates sensor telemetry against hard-coded safety thresholds.
   - Actuates physical LEDs natively through the BSP.
   - Locks the system into a physical `SHUTDOWN` block upon critical limits being breached, requiring human intervention. 
4. **Logging Task (Priority: 1)**
   - Runs purely in the background formatting raw binary structs into human-readable tables.
   - Safely casts floats down via math logic to circumvent `newlib-nano` memory-heavy float formatting limitations, sending serial via USART3 payload arrays.

---

## 💻 Output Example & State Hysteresis

The system handles warnings gracefully. To retreat from a warning, conditions must drop below the lower hysteresis bound avoiding "flickering" around the threshold.

```text
==================================================
  RT Monitoring & Control System v1.0
  STM32 NUCLEO-H7A3ZI-Q | FreeRTOS
==================================================
  Thresholds:
    Temp  WARNING: >70C  SHUTDOWN: >90C
    Press WARNING: >8.0bar  SHUTDOWN: >10.0bar
--------------------------------------------------
  [#]  Time(ms)  State     Temp(C)  Press(bar)
--------------------------------------------------
    1      500  NORMAL     25   6    3   12
...
  *** STATE TRANSITION ***
  NORMAL  --> WARNING  @ 15000ms
  [!] Sensor values approaching critical limits!

 ! 30    15000  WARNING    71.  2    8.  34

  *** STATE TRANSITION ***
  WARNING --> SHUTDOWN @ 25000ms
  [X] CRITICAL! System entering SHUTDOWN mode!
  [X] Press USER button to reset when safe.
```

---

## ⚙️ How to Run This Project

1. **Clone the Repository:** 
   Clone or download this repository to your local machine.
2. **Import into STM32CubeIDE:**
   - Open STM32CubeIDE.
   - Go to `File` > `Import` > `General` > `Existing Projects into Workspace`.
   - Select the root folder of this project (`RT_Monitoring_Control_System`).
3. **Build the Project:**
   - Press **Ctrl+B** (or `Project > Build All`).
4. **Flash the Microcontroller:**
   - Connect your Nucleo board via USB.
   - Click the green `Debug` or `Run` button (F11).
5. **View the Telemetry:**
   - Open **PuTTY** or **Tera Term**.
   - Input your COM Port (Find *STLink Virtual COM Port* in Device Manager).
   - Set Baud Rate to `115200`.

---

## 🔭 Future Enhancements
*   **Hardware FDCAN Installation:** Replace `SimulateCANLoopback()` queues with actual CAN transceiver lines and onboard FDCAN1 registers.
*   **Real I2C/SPI Sensors:** Substitute the random-walk simulation algorithms inside `sensor_task.c` with I2C reads from BME280/MPU6050 external modules.
*   **Dashboard Visualization:** Develop a Python/PyQt5 script to read the USART telemetry and graph live visualizations globally. 

---
*Created by Suratiya Rajveersinh*
