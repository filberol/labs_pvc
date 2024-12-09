#include "main.h"
#include <string.h>

UART_HandleTypeDef huart6;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_NVIC_Init(void);

#define CODE_LENGTH 8
#define TIMEOUT 10000
#define MAX_ATTEMPTS 3
#define GREEN_LED_Pin GPIO_PIN_13
#define YELLOW_LED_Pin GPIO_PIN_14
#define RED_LED_Pin GPIO_PIN_15
#define BUTTON_Pin GPIO_PIN_15
#define UART_BUFFER_SIZE 64

char LOCK_CODE[CODE_LENGTH] = "stmstmst";
char input_buffer[CODE_LENGTH];
volatile uint8_t current_pos = 0;
volatile uint8_t attempt_count = 0;
volatile uint8_t mode = 0; // 0 = Unlock Mode, 1 = Change Code Mode, 2 = Await Confirmation
volatile uint8_t uart_mode = 0; // 0 = Polling, 1 = Interrupt
uint8_t recieved_data;
char buffer = '@';
char new_code[CODE_LENGTH];
volatile uint8_t new_code_pos = 0;

char uart_buffer[UART_BUFFER_SIZE];
volatile uint8_t write_index = 0;
volatile uint8_t read_index = 0;

void UART_Init(void);
void LED_Init(void);
void System_Reset(void);
void UART_Send_Message(const char *msg);
void UART_Send_Char(char ch);
uint8_t UART_Receive_Buffer(char *data);
void UART_Buffer_Write(uint8_t data);
void Process_Input(char input);
void Change_Code_Logic(char input);
void Confirm_New_Code(char input);
void Toggle_UART_Mode(void);
void blinkLed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t times, uint16_t delay);
void unlock();

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  MX_NVIC_Init();

  UART_Send_Message("System Ready\r\n");


  if (HAL_GPIO_ReadPin(GPIOC, BUTTON_Pin) == GPIO_PIN_RESET) {
	  HAL_Delay(50);
	  if (HAL_GPIO_ReadPin(GPIOC, BUTTON_Pin) == GPIO_PIN_RESET) {
		  Toggle_UART_Mode();
		  while (HAL_GPIO_ReadPin(GPIOC, BUTTON_Pin) == GPIO_PIN_RESET);
	  }
  } else {
	  UART_Send_Message("Polling mode enabled\r\n");
  }

  if (uart_mode == 1) {
	  HAL_UART_Receive_IT(&huart6, &recieved_data, 1);
  }
  while (1) {
	  char input;
	  if (uart_mode == 0) {
		  if (HAL_UART_Receive(&huart6, (uint8_t *)&input, 1, 100) == HAL_OK) {
			  Process_Input(input);
		  }
	  } else {
		  if (buffer != '@') {
			Process_Input(buffer);
			buffer = '@';
		  }
	  }
  }
}

void UART_Send_Message(const char *msg) {
    HAL_UART_Transmit(&huart6, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

void UART_Send_Char(char ch) {
	if (ch == '\r') {UART_Send_Message("\r\n");}
    HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
}

void Process_Input(char input) {
	UART_Send_Char(input);
	if (input == '_') {
		mode = 1;
	}
	if (mode == 0) {
	        if (input == LOCK_CODE[current_pos]) {
	        	blinkLed(GPIOD, GPIO_PIN_14, 1, 100);
	            current_pos++;
	            if (current_pos == CODE_LENGTH) {
	            	UART_Send_Message("\r\nUnlocked!\r\n");
	            	unlock();
	                System_Reset();
	            }
	        } else {
	        	UART_Send_Message("\r\n");
	        	blinkLed(GPIOD, GPIO_PIN_15, 1, 200);
	            attempt_count++;
	            if (attempt_count >= MAX_ATTEMPTS) {
	            	blinkLed(GPIOD, GPIO_PIN_15, 5, 500);
	                System_Reset();
	            }
	        }
	    } else if (mode == 1) {
	        Change_Code_Logic(input);
	    } else if (mode == 2) {
	        Confirm_New_Code(input);
	    }
}

void Change_Code_Logic(char input) {
	blinkLed(GPIOD, GPIO_PIN_14, 1, 100);
	if (input != '_') {
		new_code[new_code_pos] = input;
		new_code_pos++;
	}
    if (new_code_pos >= CODE_LENGTH) {
        UART_Send_Message("\r\nSet as new code? (y/n)\r\n");
        mode = 2;
    }
}

void Confirm_New_Code(char input) {
    if (input == 'y' || input == 'Y') {
        strncpy(LOCK_CODE, new_code, CODE_LENGTH);
        UART_Send_Message("\r\nNew code set successfully!\r\n");
        System_Reset();
    } else if (input == 'n' || input == 'N') {
        UART_Send_Message("New code discarded.\r\n");
        System_Reset();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	buffer = recieved_data;
	HAL_UART_Receive_IT(&huart6, &recieved_data, 1);
}

void unlock() {
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
  HAL_Delay(5000);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
}

void blinkLed(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t times, uint16_t delay) {
  for (uint8_t i = 0; i < times; i++) {
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
    HAL_Delay(delay);
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
    HAL_Delay(delay);
  }
}

void Toggle_UART_Mode(void) {
    uart_mode = !uart_mode;
    if (uart_mode) {
        UART_Send_Message("Interrupt mode enabled\r\n");
    } else {
        UART_Send_Message("Polling mode enabled\r\n");
    }
}

void System_Reset(void) {
    current_pos = 0;
    new_code_pos = 0;
    attempt_count = 0;
    mode = 0;
    memset(input_buffer, 0, CODE_LENGTH);
    blinkLed(GPIOD, GPIO_PIN_14 | GPIO_PIN_13, 3, 300);
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* USART6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART6_IRQn);
}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
