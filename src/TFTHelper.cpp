#if defined(USE_TFT)
// #include "TFT_eSPI.h"
// #include "Free_Fonts.h"
#include "SoCHelper.h"
#include "EEPROMHelper.h"
#include "TrafficHelper.h"
#include "TFTHelper.h"
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_ST77xx.h> // Hardware-specific library for ST7789
#include <Fonts/FreeSansBold9pt7b.h> // Include the missing font
#include <Fonts/FreeSans24pt7b.h> // Include the missing font

#include "SkyView.h"
static int TFT_view_mode = 0;
unsigned long EPDTimeMarker = 0;
bool EPD_display_frontpage = false;

static int EPD_view_mode = 0;
static unsigned long EPD_anti_ghosting_timer = 0;

volatile int EPD_task_command = EPD_UPDATE_NONE;

// canvas for fast re-draw
GFXcanvas16 canvas_radar(240, 240); 

// TFT_eSPI tft = TFT_eSPI();
Adafruit_ST7789 tft = Adafruit_ST7789(SOC_GPIO_PIN_SS_TFT, SOC_GPIO_PIN_DC_TFT, SOC_GPIO_PIN_MOSI_TFT, SOC_GPIO_PIN_SCK_TFT, -1);

unsigned long drawTime = 0;

void TFT_setup(void) {
  tft.init(240, 320); // Initialize with width and height
  tft.setRotation(0);
  TFT_view_mode = settings->vmode;
  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeSans24pt7b);
  tft.setCursor(20, 80);
  tft.println("SkyView");
  delay(1000);
  tft.setFont(&FreeSansBold9pt7b);
  tft.setCursor(25, 110);
  tft.print("powered by SoftRF");
  delay(3000);
  tft.setCursor(0, 0);
  tft.setFont();

  tft.setFont();
  delay(3000);
  TFT_radar_setup();
}
void TFT_loop(void) {
  UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  Serial.print("TFT Task Stack High Water Mark: ");
  Serial.println(stackHighWaterMark);
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