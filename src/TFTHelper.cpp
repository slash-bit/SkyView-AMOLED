#if defined(USE_TFT)
// #include "TFT_eSPI.h"
// #include "Free_Fonts.h"
#include "SoCHelper.h"
#include "EEPROMHelper.h"
#include "TrafficHelper.h"
#include "TFTHelper.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_ST77xx.h> // Hardware-specific library for ST7789

#include "SkyView.h"
static int TFT_view_mode = 0;



// TFT_eSPI tft = TFT_eSPI();
Adafruit_ST7789 tft = Adafruit_ST7789(SOC_GPIO_PIN_SS_TFT, SOC_GPIO_PIN_DC_TFT, SOC_GPIO_PIN_MOSI_TFT, SOC_GPIO_PIN_SCK_TFT, -1);

unsigned long drawTime = 0;

void TFT_setup(void) {
  tft.init(240, 320); // Initialize with width and height
  tft.setRotation(1);


    TFT_view_mode = settings->vmode;


  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_BLUE);
  tft.fillRect(0, 0, 320, 30, ST77XX_BLUE);
//   tft.setTextDatum(TC_DATUM);
  tft.println("SkyView"); // Font 4 for fast drawing with background
  delay(4000);
}
#endif /* USE_TFT */