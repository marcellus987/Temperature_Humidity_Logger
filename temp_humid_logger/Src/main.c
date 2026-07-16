
#include <stdint.h>

#include "humidity_sensor_driver.h"
#include "lcd1602_driver.h"
#include "uart_driver.h"
#include "sd_card_driver.h"
#include "rtc_module_driver.h"
#include "rtc_driver.h"
#include "ff.h"
#include "power_driver.h"

//#define DEBUG_MODE__

#define SLEEP_DURATION 300

void processData(const raw_dht22_data_t* const rawSensorData, dht22_data_t* sensorData);
int celsiusToFahrenheit(uint16_t rawTempReading);
FRESULT makeLog(FIL* file, UINT* bw, const dht22_data_t* const sensorData);
void checkFRESULT(FRESULT res);

int main(void) {

	clear_pwr_flags();

#ifdef DEBUG_MODE__
	uart_init(1);
	setBaudrate(115200);
#endif

	FATFS fs;   /* File system. */
	FIL file;   /* File handler. */
	FRESULT res; /* Result from file function calls. */
	UINT br; /* Bytes read from file. */
	UINT bw; /* Bytes written to file. */
	uint32_t fileSize; /* Though f_size() returns uint64_t, it may be unrealistic
						  for a CSV file to be that large. So, only the lower 32-bit
						  will be used for now. */

	char header[] = "Date: [mm-dd-yyyy], Time (24-hour format): [hh-mm-ss], Humidity: (%), Temperature: (Celsius)\r\n";
	uint32_t headerLen = strlen(header);

	char testRead[headerLen + 1]; /* Contains what is read from file. +1 for null char to be added.
								   * Though, it may not be necessary since memcmp() would use strlen(header)
								   * which returns size not including the null char.
								   * */

	/* Mount drive. */
	res = f_mount(&fs, "",0);
	checkFRESULT(res);

	/* Open log file.*/
	res = f_open(&file, "sensor_log.csv", FA_WRITE | FA_READ | FA_OPEN_APPEND);
	checkFRESULT(res);

	/* Get lower 32-bit of the returned value of f_size(). */
	fileSize = f_size(&file) & 0xFFFFFFFFU;

	/* Set cursor to beginning. Then perform a read. */
	f_lseek(&file, 0);
	res = f_read(&file, testRead, headerLen, &br);
	checkFRESULT(res);

	/* Add null char to end of the string read from file. */
	testRead[headerLen] = '\0';

	/* Move back the cursor to EOF. To prepare for possible file write operations. */
	f_lseek(&file, fileSize);

	/* Check if header read from file matches the required header.
	 * Since strlen(header) is used  in memcmp(), it would compare
	 * bytes without the null character because strlen() returns
	 * size not including the null character.
	 *
	 * memcmp returns 0 if each byte from testRead matches those in header.
	 * If not match, there are few cases. But for now, just append the header.
	 * */
	if(memcmp((char*)testRead, header, headerLen) != 0) { /* No match. */
#ifdef DEBUG_MODE__
		printf("No header found!\r\n");
#endif
		res = f_write(&file, header, headerLen, &bw);
		checkFRESULT(res);
		f_sync(&file);
	}

	/***** Data logging phase. *****/
	raw_dht22_data_t rawSensorData;
	dht22_data_t sensorData;
	uint8_t readSucceed;

	char displayBuff[100];

	lcd_init();
	dht22_sensor_init();
	rtc_module_init();

	readSucceed = dht22_sensor_read(&rawSensorData);

	/* Store to buffer for LCD module.*/
	if(readSucceed) {
			processData(&rawSensorData, &sensorData);

			sprintf(displayBuff, "Humidity: %d.%d%% Temp: %d.%d ", sensorData.humidityInt, sensorData.humidityDec, sensorData.tempInt, sensorData.tempDec);
	#ifdef DEBUG_MODE__
			printf("%s Celsius\r\n", displayBuff);
	#endif
	}
	else {
			sprintf(displayBuff, "Error: failed to read sensor.");
	#ifdef DEBUG_MODE__
			printf(displayBuff);
	#endif
		}

	/* Print data to LCD module. */
	for(int i = 0; i < strlen(displayBuff); ++i) {
		if(displayBuff[i] == 'T' || i == 15) {
			send_command(0xC0);
		}
		send_data(displayBuff[i]);
	}

	if(readSucceed) {
		/* Celsius symbol. */
		send_data(0xDFU);
		send_data('C');
	}

	makeLog(&file, &bw, &sensorData);

	res = f_close(&file);
	checkFRESULT(res);
	res = f_mount(NULL, "", 0);
	checkFRESULT(res);

	rtc_init();
	setWakeupTimer(SLEEP_DURATION);
	power_standby(0);
} /* End of main(). */

/* If standby mode is not used. */
void RTC_WKUP_IRQHandler(void) {
	EXTI->PR |= EXTI_PR_PR22;
	clear_pwr_flags();
	clear_rtc_wutf();

	/* Might not be necessary if timer needs to be kept alive after waking up.
	 * Otherwise, stop the timer.
	 * */
	disable_wakeup_timer();
} /* End of RTC_WKUP_IRQHandler(). */

/* If at any moment user-led on board lit, there is a problem with file operation. */
void checkFRESULT(FRESULT res) {
	char errorMsg[] = "SD card Error!";
	if(res != FR_OK) {
		lcd_cls();
//		debug_led_on();
//		printf("res != FR_OK...\r\n");
		/* Print data to LCD module. */
		for(int i = 0; i < strlen(errorMsg); ++i) {
			if(i == 15) {
				send_command(0xC0);
			}
			send_data(errorMsg[i]);
		}
		delay_ms(2000);
	}
//	else {
////		debug_led_off();
////		printf("res == FR_OK...\r\n");
//	}
}


FRESULT makeLog(FIL* file, UINT* bw, const dht22_data_t* const sensorData) {
	char buff[200];
	getTimeStamp(buff);
	uint8_t lastPos = strlen(buff);

	sprintf((buff + lastPos), ",%d.%d, %d.%d\r\n", sensorData->humidityInt, sensorData->humidityDec, sensorData->tempInt, sensorData->tempDec);

	return(f_write(file, buff, ((UINT)strlen((char*)buff)), bw));
}


void processData(const raw_dht22_data_t* const rawSensorData, dht22_data_t* sensorData) {
	uint16_t humRawBits = rawSensorData->raw_humidityInt << 8 | rawSensorData->raw_humidityDec;
	uint16_t tempRawBits = rawSensorData->raw_tempInt << 8 | rawSensorData->raw_tempDec;

	/***** Humidity data. *****/
	int wholeHumReading = (int)humRawBits;
	sensorData->humidityInt = wholeHumReading / 10;
	sensorData->humidityDec = wholeHumReading % 10;

	/***** Temperature data. *****/
	int wholeTempReading = ((int)(tempRawBits & 0x7FFF));

	/* Negate if MSB is set. */
	if(tempRawBits & 0x8000U) {
		wholeTempReading *= -1;
	}

	/* Extract Bits excluding MSB which is sign-bit. */
	sensorData->tempInt = wholeTempReading / 10;
	sensorData->tempDec = wholeTempReading % 10;
}

/* Converts the raw temperature reading to Fahrenheit.
 * Param: raw bits temperature reading.
 * Return value: int value where decimal is moved to right one place.
 *               hence, it must be divided by 10 afterwards to get
 *               correct Integer and Decimal value. */
int celsiusToFahrenheit(uint16_t rawTempReading) {
	/* Enable full access to FPU Coprocessor. */
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));

	/* Convert to int first. */
	int wholeTempReading = ((int)(rawTempReading & 0x7FFF));

	/* Negate if MSB is set. */
	if(rawTempReading & 0x8000U) {
		wholeTempReading *= -1;
	}

	/* Convert to float and adjust decimal point position. */
	float tempInCelsius = (float)wholeTempReading / 10.0;

	/* Formula: (Celsius * 9/5) + 32 = Fahrenheit. */
	float tempInFahrenheit = (tempInCelsius * 9.0/5.0) + 32.0f;

	/* Convert back to int. */
	int rawFahr = tempInFahrenheit * 10;

	/* Disable full access to FPU Coprocessor to save power. */
	SCB->CPACR &= ~((3UL << 10*2) | (3UL << 11*2));

	return rawFahr;
} /* End of celsiusToFahrenheit(). */
