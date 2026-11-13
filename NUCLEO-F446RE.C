/* RTOS_Semaphore - NUCLEO-F446RE
 * External push button on D2 (PA10): other leg to GND, internal pull-up.
 * Falling edge -> EXTI10 interrupt -> ISR gives binary semaphore.
 * ButtonTask   : waits on semaphore, toggles green LED (PA8 / D7), prints press count on USART2
 * HeartbeatTask: toggles red LED (PA9 / D8) every 500 ms (shows system stays responsive)
 * HAL tick: TIM6, RTOS tick: SysTick, clock: HSI 16 MHz
 */

#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart2;

static osSemaphoreId_t btnSem;
static uint32_t pressCount = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

void StartButtonTask(void *argument);
void StartHeartbeatTask(void *argument);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  osKernelInitialize();

  /* binary semaphore: max count 1, initial count 0 (nothing signalled yet) */
  btnSem = osSemaphoreNew(1, 0, NULL);

  const osThreadAttr_t buttonAttr = {
    .name = "ButtonTask",
    .stack_size = 2048,
    .priority = (osPriority_t) osPriorityAboveNormal,
  };
  const osThreadAttr_t heartbeatAttr = {
    .name = "HeartbeatTask",
    .stack_size = 512,
    .priority = (osPriority_t) osPriorityNormal,
  };

  osThreadNew(StartButtonTask,    NULL, &buttonAttr);
  osThreadNew(StartHeartbeatTask, NULL, &heartbeatAttr);

  osKernelStart();

  while (1)
  {
  }
}

/* ---------------- Interrupt callback (runs in ISR context) ---------------- */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_10)
  {
    osSemaphoreRelease(btnSem);   /* "a press happened" - keep ISR tiny */
  }
}

/* ---------------- Tasks ---------------- */

void StartButtonTask(void *argument)
{
  char msg[64];
  int len;

  for (;;)
  {
    /* sleep here (Blocked, 0% CPU) until the ISR gives the semaphore */
    if (osSemaphoreAcquire(btnSem, osWaitForever) == osOK)
    {
      pressCount++;
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);

      len = snprintf(msg, sizeof(msg),
                     "[%lu ms] Button press #%lu\r\n",
                     (unsigned long)osKernelGetTickCount(),
                     (unsigned long)pressCount);
      HAL_UART_Transmit(&huart2, (uint8_t *)msg, (uint16_t)len, 100);

      /* simple debounce: ignore bounces for 200 ms, then clear stale signals */
      osDelay(200);
      while (osSemaphoreAcquire(btnSem, 0) == osOK)
      {
      }
    }
  }
}

void StartHeartbeatTask(void *argument)
{
  for (;;)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9);
    osDelay(500);
  }
}

/* ---------------- Peripheral init ---------------- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                     RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance          = USART2;
  huart2.Init.BaudRate     = 115200;
  huart2.Init.WordLength   = UART_WORDLENGTH_8B;
  huart2.Init.StopBits     = UART_STOPBITS_1;
  huart2.Init.Parity       = UART_PARITY_NONE;
  huart2.Init.Mode         = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);

  /* LEDs: PA8 (green, D7), PA9 (red, D8) */
  GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Button: PA10 (D2), falling-edge interrupt, internal pull-up */
  GPIO_InitStruct.Pin  = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* priority must be 5 or numerically higher to call FreeRTOS "FromISR" functions */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* ---------------- HAL tick (TIM6) ---------------- */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
