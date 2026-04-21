/*
 * sd_functions.c
 *
 *  Created on: Jan 7, 2026
 *      Author: Vaishnav
 */
#include "fatfs.h"
#include "sd_diskio_spi.h"
#include "sd_spi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ff.h"
#include "ffconf.h"
#include "Get_Temp_Time.h"
char sd_path[4];
extern uint16_t adc_value[MAX_SAMPLES];
char buf[100];
FATFS fs;
extern UART_HandleTypeDef huart2;
extern uint16_t integral[MAX_SAMPLES];
extern uint16_t fractional[MAX_SAMPLES];
//int csv_buffer[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09};
//char csv_char_buffer[]= {'V','A','I','S','H','N','A','V'};
//int sd_format(void) {
//	// Pre-mount required for legacy FatFS
//	f_mount(&fs, sd_path, 0);
//
//	FRESULT res;
//	res = f_mkfs(sd_path, 1, 0);
//	if (res != FR_OK) {
//		printf("Format failed: f_mkfs returned %d\r\n", res);
//	}
//		return res;
//

/*************************************************** Write String to Uart Terminal ************************************************/

void send_uart (char *string)
{
	uint8_t len = strlen (string);
	HAL_UART_Transmit(&huart2, (uint8_t *) string, len, HAL_MAX_DELAY); // Transmitting in Blocking Mode
}

/***********************************************************************************************************************************/


/*************************************************** Find Total Space used & available ************************************************/

int sd_get_space_kb(void) {
	FATFS *pfs;
	DWORD fre_clust, tot_sect, fre_sect, total_kb, free_kb;
	FRESULT res = f_getfree(sd_path, &fre_clust, &pfs);
	if (res != FR_OK) return res;

	tot_sect = (pfs->n_fatent - 2) * pfs->csize;
	fre_sect = fre_clust * pfs->csize;
	total_kb = tot_sect / 2;
	free_kb = fre_sect / 2;
	sprintf(buf,"💾 Total: %lu KB, Free: %lu KB\r\n", total_kb, free_kb);
	send_uart(buf);
	return FR_OK;
}

/***********************************************************************************************************************************/


/***************************************************  SD card Mount Function ************************************************/

int sd_mount(void) {
	FRESULT res;
	extern uint8_t sd_is_sdhc(void);

	send_uart("Linking SD driver...\r\n");
	if (FATFS_LinkDriver(&SD_Driver, sd_path) != 0) {			//FatFs_linkDriver() connects FATFS middleware
																// to our SD driver (user_diskio.c)
		/* sd_path = 0: using SD card
		 * "  0:"	SD Card
			"1:"	USB Mass Storage
			"2:"	External Flash
			"3:"	EEPROM*/
		send_uart("FATFS_LinkDriver failed\n");
		return FR_DISK_ERR;										// Returns error if fails to link SD dirver
	}

	send_uart("Initializing disk...\r\n");
	DSTATUS stat = disk_initialize(0);							// SD hardware initiazation function Resets
																// SD card & Sends CMD0, CMD8, ACMD41

	if (stat != 0) {											// for debug
		sprintf(buf,"disk_initialize failed: 0x%02X\n", stat);
		send_uart(buf);
		send_uart("FR_NOT_READY\r\n");
		return FR_NOT_READY;
	}

	sprintf(buf, "Attempting mount at %s...\r\n", sd_path);
	send_uart(buf);
	res = f_mount(&fs, sd_path, 1);								// Mounting FAT File system which Reads:
																// Boot sector, FAT tables & File system parameters

		// For confirmation & SD card type checking
		// SD card <= 2GB = SDSC
		// SD card > 2GB = SDHC or SDXC
	if (res == FR_OK)
	{
		sprintf(buf,"SD card mounted successfully at %s\r\n", sd_path);
		send_uart(buf);
		sprintf(buf,"Card Type: %s\r\n", sd_is_sdhc() ? "SDHC/SDXC" : "SDSC");
		send_uart(buf);

		// Capacity and free space reporting
		sd_get_space_kb();
		return FR_OK;
	}

/***********************************************************************************************************************************/


/***************************************************  Software Reset Function ******************************************************/
/***************************************************  if no file system error occures **********************************************/

//	 Handle no filesystem by creating one
//	if (res == FR_NO_FILESYSTEM)
//	{
//		printf("No filesystem found on SD card. Attempting format...\r\n");
//		sd_format();
//
//		printf("Retrying mount after format...\r\n");
//		res = f_mount(&fs, sd_path, 1);
//		if (res == FR_OK) {
//			printf("SD card formatted and mounted successfully.\r\n");
//			printf("Card Type: %s\r\n", sd_is_sdhc() ? "SDHC/SDXC" : "SDSC");
//
//			// Report capacity after format
//			sd_get_space_kb();
//		}
//		else {
//			printf("Mount failed even after format: %d\r\n", res);
//		}
//		return res;
//	}

	// Any other mount error
	sprintf(buf,"Mount failed with code: %d\r\n", res);
	send_uart(buf);
	return res;
}


/***********************************************************************************************************************************/

/***************************************************  SD Card Unmount Function ******************************************************/

int sd_unmount(void) {
	FRESULT res = f_mount(NULL, sd_path, 1);
	sprintf(buf,"SD card unmounted: %s\r\n", (res == FR_OK) ? "OK" : "Failed");
	send_uart(buf);
	return res;
}

/***********************************************************************************************************************************/


/***************************************************  File Write Function for ******************************************************/
/***************************************************  " .TXT " file ****************************************************************/

int sd_write_file(const char *filename, const char *text) {
	FIL file;
	UINT bw;
	FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) return res;

	res = f_write(&file, text, strlen(text), &bw);
	f_close(&file);
//	sprintf(buf,"Write %u bytes to %s\r\n", bw, filename);
//	send_uart(buf);
	return (res == FR_OK && bw == strlen(text)) ? FR_OK : FR_DISK_ERR;
}

/***********************************************************************************************************************************/


/***************************************************  File Append Function  ********************************************************/

int sd_append_file(const char *filename, const char *text) {
	FIL file;
	UINT bw;
	FRESULT res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
	if (res != FR_OK) return res;

	res = f_lseek(&file, f_size(&file));
	if (res != FR_OK) {
		f_close(&file);
		return res;
	}

	res = f_write(&file, text, strlen(text), &bw);
	f_close(&file);
	sprintf(buf,"Appended %u bytes to %s\r\n", bw, filename);
	send_uart(buf);
	return (res == FR_OK && bw == strlen(text)) ? FR_OK : FR_DISK_ERR;
}
/***********************************************************************************************************************************/


/***************************************************  File Read function  *********************************************************/

int sd_read_file(const char *filename, char *buffer, UINT bufsize, UINT *bytes_read) {
	FIL file;
	*bytes_read = 0;

	FRESULT res = f_open(&file, filename, FA_READ);
	if (res != FR_OK) {
		sprintf(buf,"f_open failed with code: %d\r\n", res);
		send_uart(buf);
		return res;
	}

	res = f_read(&file, buffer, bufsize - 1, bytes_read);
	if (res != FR_OK) {
		sprintf(buf,"f_read failed with code: %d\r\n", res);
		send_uart(buf);
		f_close(&file);
		return res;
	}

	buffer[*bytes_read] = '\0';

	res = f_close(&file);
	if (res != FR_OK) {
		sprintf(buf,"f_close failed with code: %d\r\n", res);
		send_uart(buf);
		return res;
	}

	sprintf(buf,"Read %u bytes from %s\r\n", *bytes_read, filename);
	send_uart(buf);
	return FR_OK;
}

/***********************************************************************************************************************************/



/*************************************************** Helper Structure for CSV READ Function ****************************************/

typedef struct CsvRecord {
	int index;
	int value;
} CsvRecord;

/***************************************************  CSV File Read Function *******************************************************/

int sd_read_csv(const char *filename, CsvRecord *records, int max_records, int *record_count) {
	FIL file;
	char line[128];
	*record_count = 0;

	FRESULT res = f_open(&file, filename, FA_READ);
	if (res != FR_OK) {
		sprintf(buf,"Failed to open CSV: %s (%d)", filename, res);
		send_uart(buf);
		return res;
	}

	sprintf(buf,"📄 Reading CSV: %s\r\n", filename);
	send_uart(buf);
	while (f_gets(line, sizeof(line), &file) && *record_count < max_records) {
		char *token = strtok(line, ",");
		if (!token) continue;
		records[*record_count].index = atoi(token);

//		token = strtok(NULL, ",");
//		if (!token) continue;
//		strncpy(records[*record_count].field2, token, sizeof(records[*record_count].field2));

		token = strtok(NULL, ",");
		if (token)
			records[*record_count].value = atoi(token);
		else
			records[*record_count].value = 0;

		(*record_count)++;
	}

	f_close(&file);

	// Print parsed data
	for (int i = 0; i < *record_count; i++) {
		sprintf(buf,"[%d] Index=  %d | Value =  0x%02X ", i,
				records[i].index,
				//records[i].field2,
				records[i].value);
		send_uart(buf);
	}

	return FR_OK;
}

/***********************************************************************************************************************************/


/***************************************************  File Delete Function  ********************************************************/

int sd_delete_file(const char *filename) {
	FRESULT res = f_unlink(filename);
	sprintf(buf,"Delete %s: %s\r\n", filename, (res == FR_OK ? "OK" : "Failed"));
	send_uart(buf);
	return res;
}

/***********************************************************************************************************************************/


/***************************************************  File Rename Fucntion  ********************************************************/

int sd_rename_file(const char *oldname, const char *newname) {
	FRESULT res = f_rename(oldname, newname);
	sprintf(buf,"Rename %s to %s: %s\r\n", oldname, newname, (res == FR_OK ? "OK" : "Failed"));
	send_uart(buf);
	return res;
}

/***********************************************************************************************************************************/


/***************************************************  File write function for ******************************************************/

FRESULT sd_create_directory(const char *path) {
	FRESULT res = f_mkdir(path);
	sprintf(buf,"Create directory %s: %s\r\n", path, (res == FR_OK ? "OK" : "Failed"));
	send_uart(buf);
	return res;
}

/***********************************************************************************************************************************/


/***************************************************  File write function for ******************************************************/

//void sd_list_directory_recursive(const char *path, int depth) {
//	DIR dir;
//	FILINFO fno;
//	FRESULT res = f_opendir(&dir, path);
//	if (res != FR_OK) {
//		sprintf(buf,"%*s[ERR] Cannot open: %s\r\n", depth * 2, "", path);
//		send_uart(buf);
//		return;
//	}
//
//	while (1) {
//		res = f_readdir(&dir, &fno);
//		if (res != FR_OK || fno.fname[0] == 0) break;
//
//		const char *name = (*fno.fname) ? fno.fname : fno.fname;
//
//		if (fno.fattrib & AM_DIR) {
//			if (strcmp(name, ".") && strcmp(name, "..")) {
//				sprintf(buf,"%*s📁 %s\r\n", depth * 2, "", name);
//				send_uart(buf);
//				char newpath[128];
//				snprintf(newpath, sizeof(newpath), "%s / %s", path, name);
//				sd_list_directory_recursive(newpath, depth + 1);
//			}
//		} else {
//			sprintf(buf,"%*s📄 %s (%lu bytes)\r\n", depth * 2, "", name, (unsigned long)fno.fsize);
//			send_uart(buf);
//		}
//	}
//	f_closedir(&dir);
//}

/***********************************************************************************************************************************/


/***************************************************  File write function for ******************************************************/

//void sd_list_files(void) {
//	send_uart("📂 Files on SD Card:\r\n");
//	sd_list_directory_recursive(sd_path, 0);
//	send_uart("\r\n\r\n");
//}

/***********************************************************************************************************************************/


/***************************************************  File write function for ******************************************************/

int sd_write_csv_buffer(const char *filename1) {

    FIL file;						// FatFS File Handle
    UINT bw;						// Byets written counter
    FRESULT res;					// To Store Result Status
    char line_buf[64];				// Temporary line buffer stores single data

    res = f_open(&file, filename1, FA_OPEN_APPEND | FA_WRITE);	// Open file in Append Mode so it does not over
    															// writes the previous data in our file
    if (res != FR_OK) return res;								// Confirm open status
    send_uart("File Open Sucessful...!\r\n");
    for(int i = 0; i < MAX_SAMPLES; i++ )								// Loop for writing exactly 20 Data logs in file
    {
    	//Syntax of sprintf(buffer to store data," Actual data or variable consisting data", veriables to read from it)
        sprintf(line_buf, "%d.%02d\r\n", integral[i], fractional[i]);
        //send_uart(line_buf);
        res = f_write(&file, line_buf, strlen(line_buf), &bw);		// Writing data in file strlen = Bytes to Sd card
        															// bw = Count of actual bytes written in File
        if (res != FR_OK) break;									// For debug
        //HAL_Delay(500);											// Delay between each Sample or Logg
    }


    f_close(&file);
    send_uart("Data Logging Complete....!\r\n");					// Confirmation message

    return res;
}

/***********************************************************************************************************************************/

