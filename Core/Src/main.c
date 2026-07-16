/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
MMC_HandleTypeDef hmmc1;
UART_HandleTypeDef huart3;

FATFS fs;

/* --- CLI (Command Line Interface) Variables --- */
char rx_buffer[1024];      /* Buffer to store incoming characters */
char file_data[1024];      /* Text box */
uint16_t rx_index = 0;     /* Current index in the buffer */
uint8_t rx_data;           /* Single character received via UART */
uint8_t command_ready = 0; /* Flag indicating Enter key was pressed */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SDMMC1_MMC_Init(void);
static void MX_USART3_UART_Init(void);

/* USER CODE BEGIN 0 */

/* Retarget printf to UART3 */
#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}

/**
  * @brief  Strict Case-Sensitive check for filenames
  * @param  target_filename: Filename input by the user
  * @param  real_filename: Buffer to store the actual filename found on eMMC
  * @retval 1 if exact match is found, 0 otherwise
  */
uint8_t OS_CheckCaseSensitive(const char* target_filename, char* real_filename)
{
	DIR dir;
	FILINFO fno;
	uint8_t match = 0;

	if (f_opendir(&dir, "/") == FR_OK)
	{
		while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0)
		{
			/* Case-insensitive match to find the actual file on disk */
			if (strcasecmp(target_filename, fno.fname) == 0)
			{
				strcpy(real_filename, fno.fname);

				/* Strict case-sensitive comparison */
				if (strcmp(target_filename, fno.fname) == 0)
				{
					match = 1;
				}
				break;
			}
		}
		f_closedir(&dir);
	}
	return match;
}

/**
  * @brief  List all files in the root directory
  */
void OS_ListFiles(void)
{
	DIR dir;
	FILINFO fno;
	printf("\r\n--- eMMC File List ---\r\n");

	if (f_opendir(&dir, "/") == FR_OK)
	{
		while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0)
		{
			printf("- %s \t\t(%lu bytes)\r\n", fno.fname, fno.fsize);
		}

		f_closedir(&dir);
	}
	else
	{
		printf("Error: Cannot open root directory.\r\n");
	}

	printf("----------------------\r\n");
}

/**
  * @brief  Read and print the content of a file
  * @param  filename: Name of the file to read
  */
void OS_Readfile(const char* filename)
{
	FIL tempFil;
	FRESULT res;
	uint8_t buffer[128];
	UINT bytes_read;
	char real_name[64] = {0};

	if (OS_CheckCaseSensitive(filename, real_name) == 1)
	{
		printf("\r\n[Reading: %s]\r\n", filename);
		res = f_open(&tempFil, filename, FA_READ);

		if (res == FR_OK)
		{
			do
			{
				f_read(&tempFil, buffer, sizeof(buffer)-1, &bytes_read);
				buffer[bytes_read] = '\0'; /* Ensure null-termination */
				printf("%s", buffer);
			}
			while (bytes_read == sizeof(buffer)-1);

			f_close(&tempFil);
			printf("\r\n[End of File]\r\n");
		}
		else
		{
			printf("Error: Cannot open file for reading.\r\n");
		}
	}
	else
	{
		if (strlen(real_name) > 0)
		{
			printf("Error: Case mismatch!\r\n");
			printf("You typed '%s' but the exact filename is '%s'\r\n", filename, real_name);
		}
		else
		{
			printf("Error: Cannot find %s.\r\n", filename);
		}
	}
}

/**
  * @brief  Create a new file or overwrite an existing one
  * @param  filename: Name of the file to write
  * @param  data: Content to be written
  */
void OS_WriteFile(const char* filename, const char* data)
{
	FIL tempFil;
	UINT bw;

	if (f_open(&tempFil, filename, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
	{
		f_write(&tempFil, data, strlen(data), &bw);
		f_close(&tempFil);
		printf("Success: Saved to %s\r\n", filename);
	}
	else
	{
		printf("Error: Cannot write to file.\r\n");
	}
}

/**
  * @brief  Delete a file from the eMMC
  * @param  filename: Name of the file to delete
  */
void OS_DeleteFile(const char* filename)
{
	char real_name[64] = {0};

	if (OS_CheckCaseSensitive(filename, real_name) == 1)
	{
		if (f_unlink(filename) == FR_OK)
		{
			printf("Success: %s deleted.\r\n", filename);
		}
		else
		{
			printf("Error: Cannot delete %s.\r\n", filename);
		}
	}
	else
	{
		if (strlen(real_name) > 0)
		{
			printf("Error: Case mismatch!\r\n");
			printf("You typed '%s' but the exact filename is '%s'\r\n", filename, real_name);
		}
		else
		{
			printf("Error: Cannot find %s.\r\n", filename);
		}
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SDMMC1_MMC_Init();
  MX_FATFS_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */

  /* Disable buffering for stdout to ensure immediate printing to Terminal */
  setvbuf(stdout, NULL, _IONBF, 0);

  HAL_Delay(500);

  /* Mount FatFs, format if it doesn't exist */
  if (f_mount(&fs, "", 1) != FR_OK)
  {
	  uint8_t workBuffer[512];
	  f_mkfs("", FM_FAT32, 0, workBuffer, sizeof(workBuffer));
	  f_mount(&fs, "", 1);
  }

  /* Print initial prompt for the Terminal */
  printf("\r\n=======================================\r\n");
  printf("       STM32 eMMC File System CLI      \r\n");
  printf("=======================================\r\n");
  printf("Available Commands:\r\n");
  printf("  1 [filename] [text] : Create/Write a file (e.g., 1 log.txt Hello!)\r\n");
  printf("  2                   : List all files in eMMC\r\n");
  printf("  3 [filename]        : Read a file (e.g., 3 log.txt)\r\n");
  printf("  4 [filename]        : Delete a file (e.g., 4 log.txt)\r\n");
  printf("=======================================\r\n");
  printf("STM32> ");
  /* USER CODE END 2 */

  /* Initialize leds and user button */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* Check if data is received via UART (Non-blocking mode) */
      if (HAL_UART_Receive(&huart3, &rx_data, 1, 0) == HAL_OK)
      {
          /* If Enter is pressed (Carriage Return or Line Feed) */
          if (rx_data == '\r' || rx_data == '\n')
          {
              if (rx_index > 0)
              {
                  rx_buffer[rx_index] = '\0'; /* Null terminate the string */
                  command_ready = 1;          /* Set flag to process command */
                  rx_index = 0;               /* Reset buffer index for next command */
                  printf("\r\n");             /* Move to next line on console */
              }
          }
          /* If Backspace is pressed */
          else if (rx_data == '\b' || rx_data == 127)
          {
              if (rx_index > 0)
              {
                  rx_index--;
                  printf("\b \b"); /* Clear character on console display */
              }
          }
          /* Normal character input */
          else if (rx_index < sizeof(rx_buffer) - 1)
          {
              rx_buffer[rx_index++] = rx_data; /* Store character in buffer */
              printf("%c", rx_data);           /* Echo character back to console */
          }
      }

      /* Process the command when Enter is pressed */
      if (command_ready == 1)
      {
          command_ready = 0; /* Clear flag */

          int cmd = 0;
          char filename[32] = {0};

          memset(file_data, 0, sizeof(file_data));

          /* Parse the input string */
          int parsed = sscanf(rx_buffer, "%d %31s %1023[^\r\n]", &cmd, filename, file_data);

          /* Execute command based on user input */
          switch (cmd)
          {
			case 1: /* Create and write file */
			  if (parsed >= 3 && strlen(filename) > 0 && strlen(file_data) > 0)
			  {
				  size_t len = strlen(file_data);
				  if (len < sizeof(file_data) - 2)
				  {
					  file_data[len] = '\n';
					  file_data[len+1] = '\0';
				  }

				  OS_WriteFile(filename, file_data);
			  }
			  else if (parsed == 2 && strlen(filename) > 0)
			  {
				  printf("Warning: No text provided. Writing default text.\r\n");
				  OS_WriteFile(filename, "Default text generated by STM32.\n");
			  }
			  else
			  {
				  printf("Error (Cmd 1): Invalid format.\r\n");
				  printf("-> Usage: 1 [filename] [text to write]\r\n");
				  printf("-> Example: 1 mydata.txt Hello STM32 World!\r\n");
			  }
			  break;

			case 2: /* List all files */
				OS_ListFiles();
				break;

			case 3: /* Read file */
				if (strlen(filename) > 0)
				{
					OS_Readfile(filename);
				}
				else
				{
					printf("Error (Cmd 3): Missing filename.\r\n");
					printf("-> Usage: 3 [filename]\r\n");
					printf("-> Example: 3 mydata.txt\r\n");
				}
				break;

			case 4: /* Delete file */
				if (strlen(filename) > 0)
				{
					OS_DeleteFile(filename);
				}
				else
				{
					printf("Error (Cmd 4): Missing filename.\r\n");
					printf("-> Usage: 4 [filename]\r\n");
					printf("-> Example: 4 mydata.txt\r\n");
				}
				break;

			default:
				printf("Error: Unknown command '%d'.\r\n", cmd);
				printf("Please use 1, 2, 3, or 4 (Type '2' to see available files).\r\n");
				break;
		  }

		/* Print prompt for the next command */
		printf("\r\nSTM32> ");
      }
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_MMC_Init(void)
{
  hmmc1.Instance = SDMMC1;
  hmmc1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hmmc1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hmmc1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hmmc1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hmmc1.Init.ClockDiv = 48-1;
  if (HAL_MMC_Init(&hmmc1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/**
  * @brief MPU Configuration
  * @retval None
  */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
