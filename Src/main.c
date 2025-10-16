/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "vl53l0x.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
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

/* USER CODE BEGIN PV */
/* Variaveis globais */
static uint16_t current_distance_mm = 0;
static bool sensor_initialized_ok = false;
static uint8_t rx_buffer[16];
static bool command_received = false;
static uint32_t uwTick_last_measurement = 0;
static uint32_t init_start_time = 0;
static bool init_blink_period = true;

/* Flag para controle da recepção UART */
volatile uint8_t uart_rx_complete = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void I2C_Scan_Bus(void);
static void Start_Uart_Reception(void);
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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  
  /* Garante que o LED começa apagado */
  HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_SET); // LED é ativo baixo
  
  /* Tenta inicializar o sensor VL53L0X */
  VL53L0X_Status init_status = VL53L0X_Init(&hi2c1);
  sensor_initialized_ok = (init_status == VL53L0X_OK);
  
  /* Log do status de inicialização */
  char init_msg[64];
  sprintf(init_msg, "Status inicializacao: %s (code: %d)\r\n", 
          sensor_initialized_ok ? "OK" : "ERRO", init_status);
  HAL_UART_Transmit(&huart1, (uint8_t*)init_msg, strlen(init_msg), 100);
  
  /* Configura modo de alta precisão se inicialização OK */
  if(sensor_initialized_ok) {
      VL53L0X_Status accuracy_status = VL53L0X_SetHighAccuracy(&hi2c1);
      if(accuracy_status != VL53L0X_OK) {
          sensor_initialized_ok = false;
          sprintf(init_msg, "Erro ao configurar alta precisao (code: %d)\r\n", accuracy_status);
          HAL_UART_Transmit(&huart1, (uint8_t*)init_msg, strlen(init_msg), 100);
      }
  }
  
  /* Inicia contagem do período de 10s se sensor foi inicializado */
  if(sensor_initialized_ok) {
    init_start_time = HAL_GetTick();
    init_blink_period = true;
  }

  /* Envia mensagens de boas-vindas */
  HAL_UART_Transmit(&huart1, (uint8_t*)"bem vindo ao console de informações\r\n", 37, 100);
  char msg[64];
  sprintf(msg, "Sensor range finder %s\r\n", sensor_initialized_ok ? "iniciado" : "nao iniciado");
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
  HAL_UART_Transmit(&huart1, (uint8_t*)"distancia medida a 5hz se disponivel.\r\n", 39, 100);
  HAL_UART_Transmit(&huart1, (uint8_t*)"> ", 2, 100);

  /* Inicia a recepção UART em modo IT */
  Start_Uart_Reception();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Medição de distância a 5Hz (200ms) */
    if(HAL_GetTick() - uwTick_last_measurement >= 200)
    {
      uwTick_last_measurement = HAL_GetTick();
      
      if(sensor_initialized_ok)
      {
        /* Lê a distância do sensor */
        VL53L0X_RangingData ranging_data = {0};
        VL53L0X_Status read_status = VL53L0X_ReadRangingData(&hi2c1, &ranging_data);
        
        if(read_status == VL53L0X_OK)
        {
          /* Envia os dados detalhados pela UART */
          char msg[64];
          sprintf(msg, "Dist: %u mm, Status: %u, Signal: %u\r\n", 
                  ranging_data.distance_mm,
                  ranging_data.rangeStatus,
                  ranging_data.signalRate);
          HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
          
          current_distance_mm = ranging_data.distance_mm;
          
          /* Controle do LED baseado na distância e tempo */
          if(init_blink_period)
          {
            /* Durante os primeiros 10 segundos, pisca o LED */
            if(HAL_GetTick() - init_start_time <= 10000)
            {
              HAL_GPIO_TogglePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin);
            }
            else
            {
              init_blink_period = false;
              HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_SET); // Apaga LED
            }
          }
          else
          {
            /* Após 10s, LED acende apenas se objeto próximo */
            if(current_distance_mm < 100)
            {
              HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_RESET); // Acende LED
            }
            else
            {
              HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_SET); // Apaga LED
            }
          }
        }
        else
        {
          /* Em caso de erro de leitura, apaga o LED */
          HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_SET);
        }
      }
    }

    /* Processamento de comando recebido */
    if(command_received)
    {
      if(strcmp((char*)rx_buffer, "i2c_bar") == 0)
      {
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\nIniciando I2C Bus Scan (i2cdetect-like)...\r\n", 46, 100);
        I2C_Scan_Bus();
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n> ", 4, 100);
      }
      
      /* Limpa buffer e flag */
      memset(rx_buffer, 0, sizeof(rx_buffer));
      command_received = false;
      
      /* Reinicia recepção UART */
      Start_Uart_Reception();
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief I2C Bus Scanner function
  * @note Scans all valid I2C addresses (0x01-0x7F) and prints results
  * @retval None
  */
static void I2C_Scan_Bus(void)
{
    char msg[64];
    uint8_t i, j;
    
    HAL_UART_Transmit(&huart1, (uint8_t*)"     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n", 49, 100);
    
    for(i = 0; i < 8; i++)
    {
        sprintf(msg, "%02X: ", i * 16);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, 4, 100);
        
        for(j = 0; j < 16; j++)
        {
            uint8_t address = (i * 16) + j;
            if(address >= 0x01 && address <= 0x77)  // Valid I2C addresses
            {
                // Try to communicate with the device
                if(HAL_I2C_IsDeviceReady(&hi2c1, address << 1, 2, 5) == HAL_OK)
                {
                    sprintf(msg, "%02X ", address);
                }
                else
                {
                    sprintf(msg, "-- ");
                }
            }
            else
            {
                sprintf(msg, "   ");
            }
            HAL_UART_Transmit(&huart1, (uint8_t*)msg, 3, 100);
        }
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
    }
}

/**
  * @brief Start UART receive in interrupt mode
  * @retval None
  */
static void Start_Uart_Reception(void)
{
    memset(rx_buffer, 0, sizeof(rx_buffer));
    HAL_UART_Receive_IT(&huart1, rx_buffer, 1);
}

/**
  * @brief UART Reception Complete Callback
  * @param huart UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static uint8_t rx_index = 0;
    
    if(huart->Instance == USART1)
    {
        if(rx_buffer[rx_index] == '\r' || rx_buffer[rx_index] == '\n')
        {
            rx_buffer[rx_index] = 0; // Null terminate
            rx_index = 0;
            command_received = true;
        }
        else
        {
            rx_index++;
            if(rx_index >= sizeof(rx_buffer) - 1)
            {
                rx_index = 0; // Buffer overflow protection
            }
            // Reinicia recepção para próximo caractere
            HAL_UART_Receive_IT(&huart1, &rx_buffer[rx_index], 1);
        }
    }
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
#ifdef USE_FULL_ASSERT
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
