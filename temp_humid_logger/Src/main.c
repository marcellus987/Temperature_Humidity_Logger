
#include <stdint.h>

#include "humidity_sensor_driver.h"
#include "lcd1602_driver.h"
#include "uart_driver.h"
#include "sd_card_driver.h"
#include "rtc_module_driver.h"

#define DEBUG_MODE__

void processData(const raw_dht22_data_t* const rawSensorData, dht22_data_t* sensorData);
int celsiusToFahrenheit(uint16_t rawTempReading);

int main(void)
{
	raw_dht22_data_t rawSensorData;
	dht22_data_t sensorData;
	uint8_t readSucceed;
//	char header[] = "Time (#),Humidity (%),Temperature (Celsius)\r\n";
	char displayBuff[100];

	uart_init(1);
	setBaudrate(115200);

	lcd_init();
	dht22_sensor_init();

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
		if(displayBuff[i] == 'T'){
			send_command(0xC0);
		}
		send_data(displayBuff[i]);
	}

	if(readSucceed) {
		/* Celsius symbol. */
		send_data(0xDFU);
		send_data('C');
	}

	printf("Testing c to f()...\r\n");
	uint16_t rawTemp = rawSensorData.raw_tempInt << 8 | rawSensorData.raw_tempDec;
	int rawFahr = celsiusToFahrenheit(rawTemp);
	printf("rawFahr: %d\r\n", rawFahr);

} /* End of main(). */


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
