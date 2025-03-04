#if defined(USE_TFT)
// #include "TFT_eSPI.h"
// #include "Free_Fonts.h"
#include "SoCHelper.h"
#include "EEPROMHelper.h"
#include "TrafficHelper.h"
#include "TFTHelper.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "TouchDrvCST92xx.h"
// #include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
// #include <Adafruit_ST77xx.h> // Hardware-specific library for ST7789

#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>


#include "SkyView.h"
static int TFT_view_mode = 0;
unsigned long EPDTimeMarker = 0;
bool EPD_display_frontpage = false;

static int EPD_view_mode = 0;
static unsigned long EPD_anti_ghosting_timer = 0;

volatile int EPD_task_command = EPD_UPDATE_NONE;
 

#if defined(AMOLED)

SPIClass SPI_2(HSPI);

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_SDIO0 /* SDIO0 */, LCD_SDIO1 /* SDIO1 */,
    LCD_SDIO2 /* SDIO2 */, LCD_SDIO3 /* SDIO3 */);


  #if defined DO0143FAT01
  Arduino_GFX *gfx = new Arduino_SH8601(bus, LCD_RST /* RST */,
                                        0 /* rotation */, false /* IPS */, LCD_WIDTH, LCD_HEIGHT);
  #elif defined H0175Y003AM
  Arduino_GFX *gfx = new Arduino_CO5300(bus, LCD_RST /* RST */,
                                        0 /* rotation */, false /* IPS */, LCD_WIDTH, LCD_HEIGHT,
                                        6 /* col offset 1 */, 0 /* row offset 1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);
  

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
    std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

std::unique_ptr<Arduino_IIC> SY6970(new Arduino_SY6970(IIC_Bus, SY6970_DEVICE_ADDRESS,
                                                       DRIVEBUS_DEFAULT_VALUE, DRIVEBUS_DEFAULT_VALUE));

std::unique_ptr<Arduino_IIC> PCF8563(new Arduino_PCF8563(IIC_Bus, PCF8563_DEVICE_ADDRESS,
                                                         DRIVEBUS_DEFAULT_VALUE, DRIVEBUS_DEFAULT_VALUE));

TouchDrvCST92xx CST9217;
#else
  #error "Unknown macro definition. Please select the correct macro definition."
  #endif
  
#endif
// TFT_eSPI tft = TFT_eSPI();
// Adafruit_ST7789 tft = Adafruit_ST7789(SOC_GPIO_PIN_SS_TFT, SOC_GPIO_PIN_DC_TFT, SOC_GPIO_PIN_MOSI_TFT, SOC_GPIO_PIN_SCK_TFT, -1);

unsigned long drawTime = 0;

void TFT_setup(void) {
  pinMode(LCD_EN, OUTPUT);
  digitalWrite(LCD_EN, LOW);
  gfx->begin(); // Start the display
  gfx->setRotation(1);
  // add some other code
  delay(100);
  TFT_view_mode = settings->vmode;
  gfx->fillScreen(BLACK);
  digitalWrite(LCD_EN, HIGH);
  gfx->setFont(&FreeMonoBold24pt7b);
  // gfx->setTextSize(5);
  gfx->setCursor(50, 180);
  gfx->setTextColor(RGB565_WHITE);
  gfx->Display_Brightness(0);
  gfx->println("SkyView");
  for (int i = 0; i <= 255; i++)
  {
      gfx->Display_Brightness(i);
      delay(3);
  }
  delay(1000);
  gfx->setFont(&FreeMonoBold12pt7b);
  // gfx->setTextSize(3);
  gfx->setCursor(70, 220);
  gfx->print("powered by SoftRF");

  TFT_radar_setup();
}
void TFT_loop(void) {
  // UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  // Serial.print("TFT Task Stack High Water Mark: ");
  // Serial.println(stackHighWaterMark);
  TFT_radar_loop();
  yield();  // Ensure the watchdog gets reset
  delay(200);
}

void TFT_Mode()
{
  if (hw_info.display == DISPLAY_TFT) {

    if (EPD_view_mode == VIEW_MODE_RADAR) {
      EPD_view_mode = VIEW_MODE_TEXT;
      EPD_display_frontpage = false;
    }  else if (EPD_view_mode == VIEW_MODE_TEXT) {
      EPD_view_mode = VIEW_MODE_RADAR;
      EPD_display_frontpage = false;
    }
  }
}

void TFT_Up()
{
  if (hw_info.display == DISPLAY_TFT) {
    switch (EPD_view_mode)
    {
    case VIEW_MODE_RADAR:
      TFT_radar_unzoom();
      break;
    case VIEW_MODE_TEXT:
      TFT_text_prev();
      break;
    default:
      break;
    }
  }
}

void TFT_Down()
{
  if (hw_info.display == DISPLAY_TFT) {
    switch (EPD_view_mode)
    {
    case VIEW_MODE_RADAR:
      TFT_radar_zoom();
      break;
    case VIEW_MODE_TEXT:
      TFT_text_next();
      break;
    default:
      break;
    }
  }
}
#endif /* USE_TFT */