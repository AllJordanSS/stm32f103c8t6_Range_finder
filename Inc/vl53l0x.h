#ifndef VL53L0X_H
#define VL53L0X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* VL53L0X I2C Device Address */
#define VL53L0X_DEFAULT_ADDRESS    0x29

/* Configurações de timing e precisão */
#define VL53L0X_HIGH_ACCURACY_TIMING_BUDGET   200000  // 200ms
#define VL53L0X_SIGNAL_RATE_LIMIT            0.25    // mcps
#define VL53L0X_SIGMA_LIMIT                  60      // mm
#define VL53L0X_VALID_READS_BEFORE_UPDATE    3       // Número de leituras válidas antes de atualizar
#define VL53L0X_MAX_MEASUREMENT_JUMP         100     // mm - Máxima variação permitida entre medidas

/* Estrutura para dados de medição */
typedef struct {
    uint16_t distance_mm;        // Distância em milímetros
    uint16_t signalRate;         // Taxa de sinal de retorno
    uint16_t ambientRate;        // Taxa de luz ambiente
    uint8_t rangeStatus;         // Status da medição
    uint8_t sigma;               // Estimativa de precisão
} VL53L0X_RangingData;

/* VL53L0X Register Addresses */
#define VL53L0X_REG_SYSRANGE_START                    0x00
#define VL53L0X_REG_RESULT_RANGE_STATUS              0x14
#define VL53L0X_REG_RESULT_RANGE_VAL                 0x1E
#define VL53L0X_REG_SYSTEM_THRESH_HIGH               0x0C
#define VL53L0X_REG_SYSTEM_THRESH_LOW                0x0E
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG           0x01
#define VL53L0X_REG_SYSTEM_RANGE_CONFIG              0x09
#define VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD   0x04
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO     0x0A
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH         0x84
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR           0x0B

/* Function Status Returns */
typedef enum {
    VL53L0X_OK = 0,
    VL53L0X_ERROR = 1
} VL53L0X_Status;

/* Function Prototypes */

/**
 * @brief Initialize the VL53L0X sensor
 * @param hi2c Pointer to I2C handle
 * @return VL53L0X_Status
 */
VL53L0X_Status VL53L0X_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Configure sensor for high accuracy mode
 * @param hi2c Pointer to I2C handle
 * @return VL53L0X_Status
 */
VL53L0X_Status VL53L0X_SetHighAccuracy(I2C_HandleTypeDef *hi2c);

/**
 * @brief Read distance measurement from sensor with filtering
 * @param hi2c Pointer to I2C handle
 * @param ranging_data Pointer to store ranging data
 * @return VL53L0X_Status
 */
VL53L0X_Status VL53L0X_ReadRangingData(I2C_HandleTypeDef *hi2c, VL53L0X_RangingData *ranging_data);

/**
 * @brief Read filtered distance measurement from sensor
 * @param hi2c Pointer to I2C handle
 * @param distance Pointer to store distance value (in mm), can be volatile
 * @return VL53L0X_Status
 */
VL53L0X_Status VL53L0X_ReadDistance(I2C_HandleTypeDef *hi2c, volatile uint16_t *distance);

#ifdef __cplusplus
}
#endif

#endif /* VL53L0X_H */