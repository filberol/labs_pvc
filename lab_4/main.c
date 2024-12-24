/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "matrix_keyboard.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define CODE_LENGTH 9
#define TIMEOUT 10000
#define MAX_ATTEMPTS 3
#define GREEN_LED_Pin GPIO_PIN_13
#define YELLOW_LED_Pin GPIO_PIN_14
#define RED_LED_Pin GPIO_PIN_15
#define BUTTON_Pin GPIO_PIN_15
#define UART_BUFFER_SIZE 64
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char LOCK_CODE[CODE_LENGTH] = "123456789";
char input_buffer[CODE_LENGTH];
volatile uint8_t current_pos = 0;
volatile uint8_t attempt_count = 0;
volatile uint8_t mode = 0; // 0 = Unlock Mode, 1 = Change Code Mode, 2 = Await Confirmation
volatile uint8_t uart_mode = 0; // 0 = Polling, 1 = Interrupt
volatile uint8_t show_led = 1;
volatile uint8_t show_sound = 1;
uint8_t recieved_data;
char buffer = '@';
char new_code[CODE_LENGTH];
volatile uint8_t new_code_pos = 0;

char uart_buffer[UART_BUFFER_SIZE];
volatile uint8_t write_index = 0;
volatile uint8_t read_index = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UART_Send_Message(const char *msg);
void UART_Send_Char(char ch);
void Change_Code_Logic(char input);
void Confirm_New_Code(char input);
void Process_Input(char input);
void System_Reset(void);

char parse_matrix_button(uint16_t* keyboard);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  MX_TIM6_Init();
  MX_TIM4_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  keyboard_init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim6);
  UART_Send_Message("System Ready\r\n");

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);

  char input;
  uint16_t keyboard_state;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  keyboard_get_state(&keyboard_state);
	  char c = parse_matrix_button(&keyboard_state);
	  if (c != 0) {
		  Process_Input(c);
	  }

	  if (HAL_UART_Receive(&huart6, (uint8_t *)&input, 1, 100) == HAL_OK) {
		  Process_Input(input);
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
char parse_matrix_button(uint16_t* keyboard) {
	if (keyboard_is_set(keyboard, BUTTON_1)) return '1';
	if (keyboard_is_set(keyboard, BUTTON_2)) return '4';
	if (keyboard_is_set(keyboard, BUTTON_3)) return '7';
	if (keyboard_is_set(keyboard, BUTTON_4)) return 'L';
	if (keyboard_is_set(keyboard, BUTTON_5)) return '2';
	if (keyboard_is_set(keyboard, BUTTON_6)) return '5';
	if (keyboard_is_set(keyboard, BUTTON_7)) return '8';
	if (keyboard_is_set(keyboard, BUTTON_8)) return '0';
	if (keyboard_is_set(keyboard, BUTTON_9)) return '3';
	if (keyboard_is_set(keyboard, BUTTON_10)) return '6';
	if (keyboard_is_set(keyboard, BUTTON_11)) return '9';
	if (keyboard_is_set(keyboard, BUTTON_12)) return 'S';
	return 0;
}

void UART_Send_Message(const char *msg) {
    HAL_UART_Transmit(&huart6, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

void UART_Send_Char(char ch) {
	if (ch == '\r') {UART_Send_Message("\r\n");}
    HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {


	if (htim->Instance == TIM6) {
		int code = LOCK_CODE[current_pos] - '0';

		if (show_led) {
			int pulse = 0;
			switch (code) {
				case 1: pulse = 25; 	htim4.Instance->CCR2 = pulse; htim4.Instance->CCR3 = 0; htim4.Instance->CCR4 = 0; break;
				case 2: pulse = 50; 	htim4.Instance->CCR2 = pulse; htim4.Instance->CCR3 = 0; htim4.Instance->CCR4 = 0; break;
				case 3: pulse = 100; 	htim4.Instance->CCR2 = pulse; htim4.Instance->CCR3 = 0; htim4.Instance->CCR4 = 0; break;
				case 4: pulse = 25; 	htim4.Instance->CCR3 = pulse; htim4.Instance->CCR2 = 0; htim4.Instance->CCR4 = 0; break;
				case 5: pulse = 50; 	htim4.Instance->CCR3 = pulse; htim4.Instance->CCR2 = 0; htim4.Instance->CCR4 = 0; break;
				case 6: pulse = 100; 	htim4.Instance->CCR3 = pulse; htim4.Instance->CCR2 = 0; htim4.Instance->CCR4 = 0; break;
				case 7: pulse = 25; 	htim4.Instance->CCR4 = pulse; htim4.Instance->CCR2 = 0; htim4.Instance->CCR3 = 0; break;
				case 8: pulse = 50; 	htim4.Instance->CCR4 = pulse; htim4.Instance->CCR2 = 0; htim4.Instance->CCR3 = 0; break;
				case 9: pulse = 100; 	htim4.Instance->CCR4 = pulse; htim4.Instance->CCR2 = 0; htim4.Instance->CCR3 = 0; break;
			}
		} else {
			htim4.Instance->CCR2 = 0;
			htim4.Instance->CCR3 = 0;
			htim4.Instance->CCR4 = 0;
		}

		if (show_sound) {
			int code = LOCK_CODE[current_pos] - '0';
			switch (code) {
				case 1: htim1.Instance->PSC = 59; break;
				case 2: htim1.Instance->PSC = 54; break;
				case 3: htim1.Instance->PSC = 49; break;
				case 4: htim1.Instance->PSC = 44; break;
				case 5: htim1.Instance->PSC = 39; break;
				case 6: htim1.Instance->PSC = 34; break;
				case 7: htim1.Instance->PSC = 29; break;
				case 8: htim1.Instance->PSC = 24; break;
				case 9: htim1.Instance->PSC = 19; break;
			}

			HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
		} else {
			HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
		}
	}
}

void Process_Input(char input) {
	UART_Send_Char(input);
	if (input == 'L') {
		show_led = !show_led;
		return;
	}
	if (input == 'S') {
		show_sound = !show_sound;
		return;
	}
	if (input == '_') {
		mode = 1;
	}
	if (mode == 0) {
	        if (input == LOCK_CODE[current_pos]) {
	        	UART_Send_Message("..Correct..");
	            current_pos++;
	            if (current_pos == CODE_LENGTH) {
	            	UART_Send_Message("\r\nUnlocked!\r\n");
	                System_Reset();
	            }
	        } else {
	        	UART_Send_Message("..Incorrect..");
	            attempt_count++;
	            if (attempt_count >= MAX_ATTEMPTS) {
	            	UART_Send_Message("\r\nTry again((\r\n");
	                System_Reset();
	            }
	        }
	    } else if (mode == 1) {
	        Change_Code_Logic(input);
	    } else if (mode == 2) {
	        Confirm_New_Code(input);
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

void Change_Code_Logic(char input) {
	if (input != '_') {
		new_code[new_code_pos] = input;
		new_code_pos++;
	}
    if (new_code_pos >= CODE_LENGTH) {
        UART_Send_Message("\r\nSet as new code? (y/n)\r\n");
        mode = 2;
    }
}

void System_Reset(void) {
    current_pos = 0;
    new_code_pos = 0;
    attempt_count = 0;
    mode = 0;
    memset(input_buffer, 0, CODE_LENGTH);
}

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
