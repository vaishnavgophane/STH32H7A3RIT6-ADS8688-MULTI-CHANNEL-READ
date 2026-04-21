/*
 * ADS8688.h
 *
 *  Created on: Mar 14, 2026
 *      Author: Administrator
 */

#ifndef INC_ADS8688_H_
#define INC_ADS8688_H_

#include "main.h"
#include "stm32h7xx_hal.h"

// ----------------------- ADS Commands and Registers ------------------------ //
// Commands

#define CONT 0x0000				// continue operation in previous mode
#define STBY 0x82				// device is placed into standby mode
#define PWDN 0x83				// device is powered down
#define RST 0x8500				// program register is reset to default
#define AUTO_RST 0xA0			// Auto mode enabled following a reset
#define MAN_0 0xC000			// Channel 0 manual input selected
#define MAN_1 0xC4				// Channel 1 manual input selected
#define MAN_2 0xC8				// Channel 2 manual input selected
#define MAN_3 0xCc				// Channel 3 manual input selected
#define MAN_4 0xD0				// Channel 4 manual input selected
#define MAN_5 0xD4				// Channel 5 manual input selected
#define MAN_6 0xD8				// Channel 6 manual input selected
#define MAN_7 0xDc				// Channel 7 manual input selected
#define ADS8688_CMD_REG_WRITE   0xD8  // Write to register (1101 1000)

// Register addresses
#define AUTO_SEQ_EN 0x01		// Auto Scan Sequencing Control
#define CHN_PWRDN 0x02			// Channel Power Down
#define FEATURE_SELECT 0x03		// Feature Select
#define CHN_0_RANGE 0x05		// Channel 0 Input Range
#define CHN_1_RANGE 0x06		// Channel 1 Input Range
#define CHN_2_RANGE 0x07		// Channel 2 Input Range
#define CHN_3_RANGE 0x08		// Channel 3 Input Range
#define CHN_4_RANGE 0x09		// Channel 4 Input Range
#define CHN_5_RANGE 0x0A		// Channel 5 Input Range
#define CHN_6_RANGE 0x0B		// Channel 6 Input Range
#define CHN_7_RANGE 0x0C		// Channel 7 Input Range


#define CMD_RD_BCK 0x3f			// COMMAND READ BACK (Read-Only)

#define CHNS_NUM_READ 1			// the number of channel you want to get the raw data
								// (you also have to adjust the AUTO_SEQ_EN register value
								//to match with the number of channel you like to read)

/* Commands */

#define CS_LOW() HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET)

/* ADS8688 Commands */
#define ADS8688_REG_AUTO_SEQ_EN 0x01	//AUTO scan sequencing enable
#define ADS8688_CMD_RST        0x8500  	// Reset
#define ADS8688_CMD_STDBY      0x8200  	// Stand by
#define ADS8688_CMD_PWR_DOWN   0x8300	// Power Down
#define ADS8688_CMD_AUTO_RST   0xA000	// Auto Channel Sequence Command

#define ADS8688_CMD_MAN_CH0    0xC000  	// Manual Select CH0
#define ADS8688_CMD_MAN_CH1    0xC400  	// Manual Select CH1
#define ADS8688_CMD_MAN_CH2    0xC800  	// Manual Select CH2
#define ADS8688_CMD_MAN_CH3    0xCC00  	// Manual Select CH3
#define ADS8688_CMD_NO_OP      0x0000  	// No Operation

/* ADS8688 Program Registers */

#define ADS8688_REG_FEATURE    0x03    // Feature Select Register

#define ADS8688_CHN_POWER_DOWN 0x02    // Channel Power Down Register

#define ADS8688_REG_CH0_RANGE  0x05    // Channel 0 Range Register
#define ADS8688_REG_CH1_RANGE  0x06    // Channel 1 Range Register
#define ADS8688_REG_CH2_RANGE  0x07    // Channel 2 Range Register
#define ADS8688_REG_CH3_RANGE  0x08    // Channel 3 Range Register
#define ADS8688_REG_CH4_RANGE  0x09    // Channel 2 Range Register
#define ADS8688_REG_CH5_RANGE  0x0A    // Channel 3 Range Register


/* Register Values */
#define ADS8688_INT_REF        0x00    // Internal Ref (REFSEL Bit = 0)
#define Unipolar_RANGE_5V       0x06    // unipolar 0 to 5.12V
#define Bipolar_RANGE_5V_PN       0x01    // Bipolar 0 to +/- 5.12V
/* Range Settings for Register 0x05 */
#define RANGE_BIPOL_10V         0x00    // +/- 10.24V
#define RANGE_BIPOL_5V          0x01    // +/- 5.12V
#define RANGE_UNIPOL_10V        0x05    // 0 to 10.24V
#define RANGE_UNIPOL_5V         0x06    // 0 to 5.12V

/* Number of Channels Selected */
#define Selected_1_Channel 0x01
#define Selected_2_Channels 0x03
#define Selected_3_Channels 0x07
#define Selected_4_Channels 0x0F
#define Selected_6_Channels 0x3F
/* Number of Channels Powered Down   */


typedef struct {

	/* SPI */
	SPI_HandleTypeDef *spiHandle;
	GPIO_TypeDef 	  *csPort;
	uint16_t 		   csPin;

} ADS8688;

uint16_t ADS8688_Read_Ch(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin);
void ADS8688_Write_Command(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint16_t cmd);
void ADS8688_Write_Register(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t data);
void ADS8688_Init_Sequence(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin);
void ADS8688_Config_AutoScan(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, int8_t channel_mask);
void ADS8688_Config_Manual(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, int8_t CHN_RNG, int8_t Range, int16_t Channel);
void ADS8688_Read_All_Auto(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_Port, uint16_t CS_Pin);
float Calculate_Channel_Average(uint16_t* data_array, uint16_t size);
#endif /* INC_ADS8688_H_ */
