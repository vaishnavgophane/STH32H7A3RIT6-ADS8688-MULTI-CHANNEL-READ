/*
 * ADS8688.c
 *
 *  Created on: Mar 14, 2026
 *      Author: Administrator
 */
#include "main.h"
#include "ADS8688.h"
extern  uint8_t status;
extern uint16_t command; // NOP to stay on current channel
extern uint16_t adc_result;

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern uint32_t avg;

extern uint16_t all_channels_raw[channels_enabled];
extern float all_channels_voltage[channels_enabled];

extern uint16_t spi_tx_buf[2]; // Command ( NO_OP)
extern uint16_t spi_rx_buf[2];           // Received data



void ADS8688_Write_Command(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint16_t cmd) {

    uint16_t tx[2] = {cmd, 0x0000};

    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, (uint8_t*)tx, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET);
}
void ADS8688_Write_Register(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t data) {

    uint16_t tx[2];

    // Command format: [Address(7 bits) | Write_Bit(1) | Data(8 bits)]

    tx[0] = (reg << 9) | (1 << 8) | data; // 15-9[Addr] - 8[1=Write/0=Read] - 7-0[Data]
    tx[1] = 0x0000;						  // 32- bit frame complete

    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, (uint8_t*)tx, 2, 15);
    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET);
}

void ADS8688_Init_Sequence(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin) {

    // 1. Reset the device
    ADS8688_Write_Command(hspi,CS_Port,CS_Pin, ADS8688_CMD_RST);	// Software Reset
    HAL_Delay(100); // Wait for internal reference to stabilize

    ADS8688_Write_Command(hspi, CS_Port, CS_Pin, ADS8688_CMD_NO_OP);	// NO_OP for Idel state
    ADS8688_Write_Command(hspi, CS_Port, CS_Pin, ADS8688_CMD_NO_OP);

    ADS8688_Write_Command(hspi, CS_Port, CS_Pin, ADS8688_CMD_STDBY);	// Entering into standby mode to set feature select and range

    ADS8688_Write_Command(hspi, CS_Port, CS_Pin, ADS8688_CMD_NO_OP);	// let the range and internal referance stabalize
    ADS8688_Write_Command(hspi, CS_Port, CS_Pin, ADS8688_CMD_NO_OP);

    ADS8688_Write_Register(hspi,CS_Port,CS_Pin,ADS8688_REG_FEATURE, ADS8688_INT_REF);		// Feature Select

    ADS8688_Config_AutoScan(hspi, CS_Port, CS_Pin, Selected_6_Channels);
    //ADS8688_Config_Manual(hspi, CS_Port, CS_Pin, ADS8688_REG_CH0_RANGE, Unipolar_RANGE_5V, ADS8688_CMD_MAN_CH0);
}
void ADS8688_Config_Manual(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, int8_t CHN_RNG, int8_t Range, int16_t Channel)
{
    ADS8688_Write_Register(hspi,CS_Port,CS_Pin, CHN_RNG, Range);	// Range Select
    HAL_Delay(5);

    // 4. Manually Select Channel 0 for the upcoming conversions
    ADS8688_Write_Command(hspi,CS_Port,CS_Pin, Channel);						// Manuel Channel 0 Selected
    ADS8688_Read_Ch(hspi,CS_Port,CS_Pin); // dummy read
    ADS8688_Read_Ch(hspi,CS_Port,CS_Pin); // dummy read

}
uint16_t ADS8688_Read_Ch(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin)
{
	uint16_t tx[2] = {0x0000};
	uint16_t rx[2] = {0};

    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_RESET);
//    HAL_SPI_TransmitReceive_DMA(&hspi1, (uint8_t*)spi_tx_buf, (uint8_t*)spi_rx_buf, 2);
    HAL_SPI_TransmitReceive(hspi, (uint8_t*)tx, (uint8_t*)rx, 2, 10);
    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET);

    return rx[1];
}

void ADS8688_Config_AutoScan(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, int8_t channel_mask) {
    // 1. Enable selected channels in the Auto Scan register
    // Example: channel_mask = 0xFF enables all 8 channels
    ADS8688_Write_Register(hspi, CS_Port,CS_Pin, ADS8688_REG_AUTO_SEQ_EN, channel_mask);

    // 2. Powering Down remaining channels in the Auto Scan register
    ADS8688_Write_Register(hspi, CS_Port, CS_Pin, ADS8688_CHN_POWER_DOWN, 0xC0); 	// CHN 7 & 6 Off Other 6 ON

    // 3. Set ranges for all channels (Optional: Repeat for all 8)
    for(uint8_t i = 0; i < channels_enabled; i++) {
        ADS8688_Write_Register(hspi, CS_Port,CS_Pin, (0x05 + i), Unipolar_RANGE_5V);
    }

    // 4. Command the ADC to enter Auto Scan Mode
    ADS8688_Write_Command(hspi, CS_Port,CS_Pin, ADS8688_CMD_AUTO_RST);

    // 5. Dummy Read for prevoius mode data removal
    ADS8688_Read_All_Auto(hspi, CS_Port, CS_Pin);
    ADS8688_Read_All_Auto(hspi, CS_Port, CS_Pin);
    ADS8688_Read_All_Auto(hspi, CS_Port, CS_Pin);
}

void ADS8688_Read_All_Auto(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin) {
    uint16_t tx[2] = {0x0000, 0x0000}; // NO_OP
    uint16_t rx[2] = {0};

    for (int i = 0; i < channels_enabled; i++) {
        HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(hspi, (uint8_t*)tx, (uint8_t*)rx, 2, 1);
        HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET);

        all_channels_raw[i] = rx[1];
        all_channels_voltage[i] = (all_channels_raw[i] / 65535.0f) * 5.106f;
    }
}
float Calculate_Channel_Average(uint16_t* data_array, uint16_t size) {
    uint64_t sum = 0;
    for (uint16_t i = 0; i < size; i++) {
        sum += data_array[i];
    }
    // Convert average raw value to voltage
    float avg_raw = (float)sum / (float)size;
    return (avg_raw / 65535.0f) * 5.106f;
}
