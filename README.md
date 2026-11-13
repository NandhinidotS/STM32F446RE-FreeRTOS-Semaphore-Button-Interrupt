# STM32 NUCLEO-F446RE FreeRTOS Binary Semaphore Button Interrupt using CMSIS-RTOS V2, STM32CubeMX and STM32CubeIDE

![MCU](https://img.shields.io/badge/MCU-STM32F446RE-blue) ![Board](https://img.shields.io/badge/Board-NUCLEO--F446RE-03234B) ![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE%202.2.0-red) ![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20(CMSIS--RTOS%20V2)-green) ![Language](https://img.shields.io/badge/Language-Embedded%20C-lightgrey)

## Project Overview

This project shows the standard **interrupt-to-task signaling pattern** of an RTOS, using **FreeRTOS** on the **STM32 NUCLEO-F446RE** board. An external **push button** is connected to **D2 (PA10)** and works as an **interrupt**. When the button is pressed, the interrupt routine does only one small job: it gives a **binary semaphore**. A task named `ButtonTask` is sleeping and waiting for that semaphore. When the semaphore arrives, the task wakes up, **toggles an LED** and **prints the press number** over UART. A second task, `HeartbeatTask`, blinks another LED every 500 ms the whole time, which shows that the system is never blocked.

The code is written in **Embedded C** with the **STM32 HAL library**, uses FreeRTOS through the **CMSIS-RTOS V2** interface, and was created with **STM32CubeMX** and built, flashed and run with **STM32CubeIDE 2.2.0**.

This project covers four topics: external interrupts (EXTI), binary semaphores, running two FreeRTOS tasks at the same time, and moving the HAL timebase from SysTick to TIM6.

## Key Features

- FreeRTOS with the CMSIS-RTOS V2 interface
- Two tasks running at the same time (`ButtonTask` and `HeartbeatTask`)
- Push button interrupt on PA10 (EXTI10), falling edge, internal pull-up
- Binary semaphore given from the interrupt and taken by a task
- `ButtonTask` sleeps in the Blocked state, using no CPU, until the button is pressed
- 200 ms software debounce, then extra signals are cleared
- Press count and time printed over UART (115200 baud) through the ST-LINK virtual COM port
- Heartbeat LED that keeps blinking while the button is handled
- HAL timebase on TIM6, so SysTick is free for the RTOS
- Programming and debugging through the on-board ST-LINK

## Working Principle

1. The kernel starts two tasks. `HeartbeatTask` begins blinking its LED every 500 ms. `ButtonTask` asks for the semaphore, finds it empty and goes to sleep.
2. Pressing the button pulls **PA10** LOW. This is a falling edge, so the **EXTI interrupt** runs.
3. The interrupt routine calls `osSemaphoreRelease()` to give the semaphore, and returns at once.
4. FreeRTOS sees that `ButtonTask` was waiting, so it wakes the task. `ButtonTask` has a higher priority than `HeartbeatTask`, so it runs immediately.
5. `ButtonTask` adds 1 to the press count, toggles the LED on PA8 and prints `[time ms] Button press #N` over UART.
6. `ButtonTask` waits 200 ms (debounce), clears any extra signals caused by switch bounce, and goes back to sleep.
7. During all of this, `HeartbeatTask` keeps toggling the LED on PA9 every 500 ms.

| Task            | Priority    | Stack  | Job                                            |
| --------------- | ----------- | ------ | ---------------------------------------------- |
| `ButtonTask`    | AboveNormal | 2048 B | Waits on the semaphore and handles every press |
| `HeartbeatTask` | Normal      | 512 B  | Toggles the heartbeat LED every 500 ms         |

| RTOS Concept              | Where it appears in the code                                           |
| ------------------------- | ---------------------------------------------------------------------- |
| Tasks                     | `StartButtonTask` and `StartHeartbeatTask`, created with `osThreadNew` |
| Scheduler                 | Started with `osKernelStart()`                                         |
| Binary semaphore          | `osSemaphoreNew(1, 0, NULL)` (maximum count 1, initial count 0)        |
| Interrupt-to-task signal  | `osSemaphoreRelease()` inside `HAL_GPIO_EXTI_Callback()`               |
| Blocking                  | `osSemaphoreAcquire(..., osWaitForever)` and `osDelay()`               |
| Priorities and preemption | `ButtonTask` runs as soon as the semaphore is given                    |

## Components Required

| Component                 | Part              | Value   | Quantity |
| ------------------------- | ----------------- | ------- | -------- |
| Microcontroller board     | NUCLEO-F446RE     | -       | 1        |
| Push button               | Tactile switch    | -       | 1        |
| Toggle LED                | LED               | -       | 1        |
| Heartbeat LED             | LED               | -       | 1        |
| LED resistors             | Resistor          | 220 ohm | 2        |
| Breadboard                | -                 | -       | 1        |
| Jumper wires              | -                 | -       | As needed |
| USB cable (data cable)    | USB Type-A to Mini-B | -    | 1        |

### Hardware Setup

![Semaphore](Semaphore.jpeg)

![Blue_led](Blue_led.jpeg)

### Pin Connections

| Part            | Pin                          | Arduino Pin | MCU Pin |
| --------------- | ---------------------------- | ----------- | ------- |
| Toggle LED      | Anode (+) through 220 ohm    | D7          | PA8     |
| Heartbeat LED   | Anode (+) through 220 ohm    | D8          | PA9     |
| Push button     | Leg 1                        | D2          | PA10    |
| UART TX         | USART2 (ST-LINK COM port)    | -           | PA2     |
| UART RX         | USART2 (ST-LINK COM port)    | -           | PA3     |

- **LEDs:** the long leg (anode, +) goes through a 220 ohm resistor to the pin, and the short leg (cathode, -) goes to GND.
- **Push button:** leg 1 goes to D2 and leg 2 goes to GND. Use two legs on opposite corners of the button. No external resistor is needed, because the internal pull-up of PA10 is enabled in software.
- **UART:** no wiring is needed. The data goes through the USB cable to the ST-LINK virtual COM port.

## Source Code

The complete program is in the file `main.c`. It replaces the `Core/Src/main.c` file generated by CubeMX, and it contains the semaphore, the two tasks, the interrupt callback and the peripheral setup.

To change the blink speed or the debounce time, edit these lines in the tasks:

```
osDelay(500);    /* heartbeat LED blink period in ms */
osDelay(200);    /* button debounce time in ms */
```

## Setup Process

### Step 1: Create the Project in STM32CubeIDE

Click **File → New → STM32 Project**, open the **Board Selector** tab, select **NUCLEO-F446RE**, and click **Next**. Enter the project name `RTOS_Semaphore` and finish the wizard. When asked, choose **Yes** to initialize all peripherals with their default mode. The `.ioc` file opens in CubeMX.

### Step 2: Set PA10 as the Button Interrupt Pin

In the **Pinout view**, click **PA10** and choose **GPIO_EXTI10**.

![MX_PinOut_PA10](MX_PinOut_PA10.png)

### Step 3: Set the GPIO Mode

Open **System Core → GPIO**, select **PA10** and set:

| Setting                | Value                                                       |
| ---------------------- | ----------------------------------------------------------- |
| GPIO mode              | External Interrupt Mode with Falling edge trigger detection |
| GPIO Pull-up/Pull-down | Pull-up                                                     |

![MX_GPIO](MX_GPIO.png)

### Step 4: Enable the Interrupt in NVIC

Open **System Core → NVIC**, tick **EXTI line[15:10] interrupts** and set its preemption priority to **5**.

![MX_NVIC](MX_NVIC.png)

### Step 5: Change the Timebase Source to TIM6

Open **System Core → SYS** and set **Timebase Source** to **TIM6**. FreeRTOS uses SysTick for its own tick, so the HAL tick must move to another timer.

![MX_SYS](MX_SYS.png)

### Step 6: Configure USART2

Open **Connectivity → USART2** and set the mode to **Asynchronous**. Keep the baud rate at **115200** with 8 data bits, no parity and 1 stop bit.

![MX_USART2](MX_USART2.png)

### Step 7: Enable FreeRTOS

Open **Middleware and Software Packs → FREERTOS** and set the **Interface** to **CMSIS_V2**.

![MX_FreeRTOS](MX_FreeRTOS.png)

### Step 8: Project Manager and Generate Code

A parent folder is created first, and CubeMX creates the project folder inside it:

```
C:\stm32_ws        ← parent folder (created manually)
└── RTOS_Semaphore ← created automatically by CubeMX
```

| Setting          | Value                                           |
| ---------------- | ----------------------------------------------- |
| Project Name     | RTOS_Semaphore                                  |
| Project Location | C:\stm32_ws (parent folder, chosen with Browse) |
| Toolchain / IDE  | STM32CubeIDE                                    |
| Firmware Package | STM32Cube FW_F4 V1.28.3                         |

Press **Ctrl+S** and click **Generate Code**.

![MX_ProjectManager](MX_ProjectManager.png)

### Step 9: Replace main.c

In STM32CubeIDE, open `Core/Src/main.c`, delete everything in it, paste the complete program from the file `main.c` in this repository, and save it.

### Step 10: Build and Run

Click the **hammer icon** to build the project. The Console should show the build finishing without errors. Connect the board with a USB cable, click **Run**, and accept the default debug configuration. The Console ends with `Download verified successfully`.

### Step 11: Open the Terminal and Press the Button

Open a terminal in STM32CubeIDE on the ST-LINK COM port at **115200 baud**. Press the push button and watch the messages.

## Output

The heartbeat LED blinks every 500 ms from the start. Each button press toggles the second LED and prints one line.

| Condition               | Toggle LED | Heartbeat LED | UART Message                 |
| ----------------------- | ---------- | ------------- | ---------------------------- |
| After reset             | OFF        | Blinking      | None                         |
| Button pressed          | Toggles    | Blinking      | `[time ms] Button press #N`  |

```
[62691 ms] Button press #8
[64110 ms] Button press #9
[66713 ms] Button press #10
[67102 ms] Button press #11
[67443 ms] Button press #12
[67794 ms] Button press #13
[68610 ms] Button press #14
```

![IDE_Terminal](IDE_Terminal.png)

**Demo video:** [Watch the demo](https://drive.google.com/file/d/1tkdf7aMdBZXMfJ3AmlYh4H-RCceozLfv/view?usp=drivesdk)

## Important Notes

- **Timebase:** with FreeRTOS, the HAL timebase must not use SysTick. Keep **Timebase Source = TIM6**, otherwise the RTOS and the HAL clash.
- **Interrupt priority:** FreeRTOS only allows interrupts at priority **5 or numerically higher** to call its functions. The EXTI interrupt is set to 5, so it can safely call `osSemaphoreRelease()`.
- **Interrupt routine:** it only gives the semaphore and returns. The slow work (LED, UART, delay) is done in the task.
- **Jumpers:** both **CN2** jumper caps on the board must be fitted. If they are missing, the debugger shows `Error in initializing ST-LINK device. Reason: No device found on target.`
- **Project location:** in the Project Manager, set the location to the **parent folder** (for example `C:\stm32_ws`), not to a project folder. Otherwise CubeMX adds a second folder inside it and fails with "The system cannot find the path specified".
- **Regenerating code:** `main.c` is replaced completely in this project. If the code is regenerated from CubeMX, `main.c` is overwritten, so keep a copy of it.

## Future Improvements

- Counting semaphore to queue several quick presses
- Message queue between the button task and a logging task
- Mutex-protected UART shared by several tasks
- Software timer for long-press detection
