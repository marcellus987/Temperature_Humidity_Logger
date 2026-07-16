# Temperature_Humidity_Logger
Repository for my temperature and humidity logger project.

**<u>Note</u>:** You must clone the drivers needed for this project under "drivers" repository. This is a must until I figure out
how to group them. Or, just download each individual drivers necessary for the project. Thank you!

**<u>FYI</u>:** This project is currently ***In Progress***, I made significant changes locally before I updating this remote repository (bad habit, I know).

## <u>Pins used (NUCLEO-STM32F411RE)</u>:
### LCD1602 (Parallel-mode):
- PC0 through PC7: DATA
- PC8: RS, for selecting which register to write.
- PC9: E, for enable signal for write/read.
- GND: R/W, grounded to pull low since we are just writing to LCD.

### DHT22 Temperature-Humidity sensor module (single wire):
- PA0: DATA

### DS3231 (RTC module):
- PB6: SDA
- PB7: SCL

### SD-card module (SPI-based):
- PA1: CS
- PA5: SCLK
- PA6: MISO
- PA7: MOSI

# Flow:
- **TBU**: I will update this with an image of a diagram, typing it is not easy.

<hr>

# Demo:

### Image(s):

**<u>Fully functional demo image:<u>** 
![A picture of fully functional (no errors encountered) temperature-humidity logger.](/demo/images/funtional_demo.jpg)

**<u>SD card error demo image:<u>** 
![A picture of SD card error encountered by the temperature-humidity logger.](/demo/images/sd_error_demo.jpg)

**<u>Components:<u>**
- NUCLEO-STM32F411RE, SD card module, DS3231 RTC module, DHT22 sensor, LCD1602, and potentiometer for LCD.
![Components used in this project.](/demo/images/components.jpg)


**<u>Sample Log:<u>**
- .csv file is located in the demo directory of this repository.
![Sample log graph](/demo/sample_log/sample_log_graph.png)


### Video(s):
- **TBU**: I will post a simple demonstration video here.

