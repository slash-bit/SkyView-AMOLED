/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-06-05 13:01:59
 * @LastEditTime: 2025-01-23 15:07:46
 */
#pragma once

// #define DO0143FAT01 //DO0143FMST02//1.43 inches (SH8601 FT3168)
// #define H0175Y003AM //1.75 inches (CO5300 CST9217)
// #define DO0143FMST10 //1.43 inches (CO5300 FT3168)

#if defined(DO0143FAT01)
#define LCD_SDIO0 11
#define LCD_SDIO1 12
#define LCD_SDIO2 13
#define LCD_SDIO3 14
#define LCD_SCLK 10
#define LCD_CS 9
#define LCD_RST 21
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

#define LCD_EN 42

// IIC
#define IIC_SDA 47
#define IIC_SCL 48

// TOUCH
#define TP_INT 8

// Battery Voltage ADC
#define BATTERY_VOLTAGE_ADC_DATA 4

// SD
#define SD_CS 38
#define SD_MOSI 39
#define SD_MISO 40
#define SD_SCLK 41

// PCF8563
#define PCF8563_INT 15
#endif // DO0143FAT01

#if defined(H0175Y003AM)
//AMOLED
#define LCD_SDIO0 11
#define LCD_SDIO1 13
#define LCD_SDIO2 14
#define LCD_SDIO3 15
#define LCD_SCLK 12
#define LCD_CS 10
#define LCD_RST 17
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define LCD_EN 16

// IIC
#define IIC_SDA 7
#define IIC_SCL 6

// TOUCH
#define TP_INT 9

// Battery Voltage ADC
#define BATTERY_VOLTAGE_ADC_DATA 4

// SD
#define SD_CS 38
#define SD_MOSI 39
#define SD_MISO 40
#define SD_SCLK 41

// PCF8563
#define PCF8563_INT 9

#define SLEEP_WAKE_UP_INT GPIO_NUM_5
#define BUTTON_PIN 5



#endif // H0175Y003AM
#if defined(SQUARE_AMOLED)

#define LCD_WIDTH             450 //physical display horizontal resolution
#define LCD_HEIGHT            600 //physical display vertical resolution

// TOUCH
#define I2C_ADDR_FT3168 0x38
#define TOUCH_INT -1
#define TOUCH_RST -1
#define IIC_SDA 47
#define IIC_SCL 48
// Battery Voltage ADC
#define BATTERY_VOLTAGE_ADC_DATA 4

#define SLEEP_WAKE_UP_INT GPIO_NUM_5
#define BUTTON_PIN 5

#endif