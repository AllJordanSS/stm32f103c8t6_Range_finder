#include "vl53l0x.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

/* Private variables */
static uint16_t last_valid_distance = 0;
static uint8_t valid_reading_count = 0;
static VL53L0X_RangingData last_ranging_data = {0};

/* Private function prototypes */
static VL53L0X_Status VL53L0X_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value);
static VL53L0X_Status VL53L0X_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *value);
static VL53L0X_Status VL53L0X_ReadMulti(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint8_t count);
static bool VL53L0X_IsValidReading(VL53L0X_RangingData *ranging_data);

VL53L0X_Status VL53L0X_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t temp;
    
    /* Wait for boot */
    HAL_Delay(100);
    
    /* Check sensor ID */
    if(VL53L0X_ReadReg(hi2c, 0xC0, &temp) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    /* Initialize sensor with default settings */
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xFF) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    /* Set GPIO config to interrupt on new sample ready */
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    /* Set interrupt polarity to active high */
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, 0x21) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    return VL53L0X_OK;
}

VL53L0X_Status VL53L0X_SetHighAccuracy(I2C_HandleTypeDef *hi2c)
{
    /* Configure timing budget for high accuracy */
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xFF) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    /* Set timing budget (~200ms) */
    uint16_t budget = VL53L0X_HIGH_ACCURACY_TIMING_BUDGET / 1000;
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD, budget) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    return VL53L0X_OK;
}

VL53L0X_Status VL53L0X_ReadRangingData(I2C_HandleTypeDef *hi2c, VL53L0X_RangingData *ranging_data)
{
    uint8_t temp;
    uint8_t data[2];
    
    /* Start single range measurement */
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_SYSRANGE_START, 0x01) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    /* Wait for range measurement completion */
    do {
        if(VL53L0X_ReadReg(hi2c, VL53L0X_REG_RESULT_RANGE_STATUS, &temp) != VL53L0X_OK) {
            return VL53L0X_ERROR;
        }
    } while((temp & 0x01) == 0);
    
    /* Read range status and value */
    ranging_data->rangeStatus = temp >> 4;
    
    /* Read range value */
    if(VL53L0X_ReadMulti(hi2c, VL53L0X_REG_RESULT_RANGE_VAL, data, 2) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    /* Convert to mm */
    ranging_data->distance_mm = ((uint16_t)data[0] << 8) | data[1];
    
    /* Read signal and ambient rate (opcional, mas útil para filtragem) */
    if(VL53L0X_ReadMulti(hi2c, VL53L0X_REG_RESULT_RANGE_STATUS + 6, data, 2) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    ranging_data->signalRate = ((uint16_t)data[0] << 8) | data[1];
    
    /* Clear interrupt */
    if(VL53L0X_WriteReg(hi2c, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01) != VL53L0X_OK) {
        return VL53L0X_ERROR;
    }
    
    return VL53L0X_OK;
}

static bool VL53L0X_IsValidReading(VL53L0X_RangingData *ranging_data)
{
    /* Verifica status da medição */
    if(ranging_data->rangeStatus != 0) {
        return false;
    }
    
    /* Verifica se a taxa de sinal está acima do limite mínimo */
    if(ranging_data->signalRate < (VL53L0X_SIGNAL_RATE_LIMIT * 65536.0f)) {
        return false;
    }
    
    /* Verifica se a variação da medida não é muito brusca */
    if(last_valid_distance > 0) {
        int16_t diff = abs((int16_t)ranging_data->distance_mm - (int16_t)last_valid_distance);
        if(diff > VL53L0X_MAX_MEASUREMENT_JUMP) {
            return false;
        }
    }
    
    return true;
}

VL53L0X_Status VL53L0X_ReadDistance(I2C_HandleTypeDef *hi2c, volatile uint16_t *distance)
{
    VL53L0X_RangingData ranging_data;
    
    /* Lê os dados do sensor */
    if(VL53L0X_ReadRangingData(hi2c, &ranging_data) != VL53L0X_OK) {
        valid_reading_count = 0;
        return VL53L0X_ERROR;
    }
    
    /* Verifica se a leitura é válida */
    if(VL53L0X_IsValidReading(&ranging_data)) {
        valid_reading_count++;
        
        /* Atualiza a distância apenas após várias leituras válidas consecutivas */
        if(valid_reading_count >= VL53L0X_VALID_READS_BEFORE_UPDATE) {
            *distance = ranging_data.distance_mm;
            last_valid_distance = ranging_data.distance_mm;
            memcpy(&last_ranging_data, &ranging_data, sizeof(VL53L0X_RangingData));
        }
    } else {
        valid_reading_count = 0;
    }
    
    return VL53L0X_OK;
}

/* Private Functions */

static VL53L0X_Status VL53L0X_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    
    if(HAL_I2C_Master_Transmit(hi2c, VL53L0X_DEFAULT_ADDRESS << 1, data, 2, 100) != HAL_OK) {
        return VL53L0X_ERROR;
    }
    
    return VL53L0X_OK;
}

static VL53L0X_Status VL53L0X_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *value)
{
    if(HAL_I2C_Master_Transmit(hi2c, VL53L0X_DEFAULT_ADDRESS << 1, &reg, 1, 100) != HAL_OK) {
        return VL53L0X_ERROR;
    }
    
    if(HAL_I2C_Master_Receive(hi2c, VL53L0X_DEFAULT_ADDRESS << 1, value, 1, 100) != HAL_OK) {
        return VL53L0X_ERROR;
    }
    
    return VL53L0X_OK;
}

static VL53L0X_Status VL53L0X_ReadMulti(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data, uint8_t count)
{
    if(HAL_I2C_Master_Transmit(hi2c, VL53L0X_DEFAULT_ADDRESS << 1, &reg, 1, 100) != HAL_OK) {
        return VL53L0X_ERROR;
    }
    
    if(HAL_I2C_Master_Receive(hi2c, VL53L0X_DEFAULT_ADDRESS << 1, data, count, 100) != HAL_OK) {
        return VL53L0X_ERROR;
    }
    
    return VL53L0X_OK;
}