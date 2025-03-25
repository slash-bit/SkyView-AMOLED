#if defined(USE_TFT)
#include "TFT_eSPI.h"
// #include "Free_Fonts.h"
#include "SoCHelper.h"
#include "EEPROMHelper.h"
#include "TrafficHelper.h"
#include "TFTHelper.h"
// #include <Adafruit_GFX.h>    // Core graphics library
// #include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
// #include "TouchDrvCST92xx.h"
#include <../pins_config.h>
// #include <driver/display/CO5300.h>

// #include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
// #include <Adafruit_ST77xx.h> // Hardware-specific library for ST7789

// #include <Fonts/GFXFF/FreeSansBold12pt7b.h>
// #include <Fonts/GFXFF/FreeMonoBold24pt7b.h>


#include "SkyView.h"
static int TFT_view_mode = 0;
unsigned long EPDTimeMarker = 0;
bool EPD_display_frontpage = false;




#if defined(AMOLED)




// SPIClass SPI_2(HSPI);

// Arduino_DataBus *bus = new Arduino_ESP32QSPI(
//     LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_SDIO0 /* SDIO0 */, LCD_SDIO1 /* SDIO1 */,
//     LCD_SDIO2 /* SDIO2 */, LCD_SDIO3 /* SDIO3 */);

#if defined DO0143FAT01
// Arduino_GFX *gfx = new Arduino_SH8601(bus, LCD_RST /* RST */,
//                                       0 /* rotation */, false /* IPS */, LCD_WIDTH, LCD_HEIGHT);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite sprite2 = TFT_eSprite(&tft);

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
  std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
#elif defined H0175Y003AM
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite sprite2 = TFT_eSprite(&tft);

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
  std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

#elif defined(SQUARE_AMOLED)
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite sprite2 = TFT_eSprite(&tft);
LilyGo_Class amoled;

#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

  
#endif
// TFT_eSPI tft = TFT_eSPI();
// Adafruit_ST7789 tft = Adafruit_ST7789(SOC_GPIO_PIN_SS_TFT, SOC_GPIO_PIN_DC_TFT, SOC_GPIO_PIN_MOSI_TFT, SOC_GPIO_PIN_SCK_TFT, -1);
buddy_info_t buddies[] = {
  { 0x201076, "XCT_Vlad" },
  { 0x86D7FD, "T-Echo" },
  { 0xE18990, "ESP-Stick" },
  { 0x46CBDC, "SenseCap" },
  { 0xFFFFFFFF, NULL } // Sentinel value
};

unsigned long drawTime = 0;
void draw_first()
{
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.setFreeFont(&FreeMonoBold24pt7b);
  // sprite.setTextSize(2);
  sprite.drawString("SkyView",225,140,4);
  Serial.print("SkyView width: ");
  Serial.println(sprite.textWidth("SkyView"));
  Serial.print("SkyView height: ");
  Serial.println(sprite.fontHeight(4));
  sprite.setFreeFont(&FreeSansBold12pt7b);
  Serial.print("powered by... width: ");
  Serial.println(sprite.textWidth("powered by SoftRF"));
  Serial.print("powered by ... height: ");
  Serial.println(sprite.fontHeight(4));
  sprite.fillRect(114,200,66,66,TFT_RED);
  sprite.fillRect(200,200,66,66,TFT_GREEN);
  sprite.fillRect(286,200,66,66,TFT_BLUE); 
  sprite.setTextSize(1);

  sprite.drawString("powered by SoftRF",233,286,4);
  amoled.pushColors(0, 0, 450, 450, (uint16_t*)sprite.getPointer());
  for (int i = 0; i <= 255; i++)
  {
    amoled.setBrightness(i);
    delay(3);
  }
  delay(2000);

}

void TFT_setup(void) {
  // pinMode(LCD_EN, OUTPUT);
  // digitalWrite(LCD_EN, HIGH);
  // delay(30);
  // pinMode(SENSOR_RST, OUTPUT);
  // digitalWrite(SENSOR_RST, LOW);
  // delay(30);
  // digitalWrite(SENSOR_RST, HIGH);
  // delay(50);
  // Wire.begin(SENSOR_SDA, SENSOR_SCL);
  // CO5300_init();
  Serial.println("TFT_setup. AMOLED Init ");
  bool rslt = false;
  rslt = amoled.beginAMOLED_241();

  if (!rslt) {
      while (1) {
          Serial.println("There is a problem with the device!~"); delay(1000);
      }
  }
  amoled.setRotation(3);
  sprite.setColorDepth(16);
  Serial.print("TFT_setup. PSRAM_ENABLE: ");
  Serial.println(sprite.getAttribute(PSRAM_ENABLE));
  sprite.setAttribute(PSRAM_ENABLE, 1);

  Serial.printf("Free heap: %d bytes\n", esp_get_free_heap_size());
  Serial.printf("Largest block: %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  sprite.createSprite(450, 450);    // full screen landscape sprite in psram

  if (sprite.createSprite(450, 450) == NULL) {
    Serial.println("Failed to create sprite. Not enough memory.");
    delay(5000);
  }
  else {
    Serial.print("TFT_setup. Created Sprite| Free Heap: ");
    Serial.println(esp_get_free_heap_size());
  }

  // TFT_view_mode = settings->vmode;
  draw_first();
  // TFT_radar_setup();
}

void TFT_loop(void) {
  switch (TFT_view_mode)
  {
  case VIEW_MODE_RADAR:
    TFT_radar_loop();
    break;
  case VIEW_MODE_TEXT:
    TFT_text_loop();
    break;
  case VIEW_MODE_COMPASS:
    TFT_compass_loop();
    break;
  default:
    break;
  }
  
  yield();  // Ensure the watchdog gets reset
  delay(20);
}

void TFT_Mode(boolean next)
{
  if (hw_info.display == DISPLAY_TFT) {

    if (TFT_view_mode == VIEW_MODE_RADAR) {
      if (next) {
      TFT_view_mode = VIEW_MODE_TEXT;
      EPD_display_frontpage = false;
      }
      else {
        TFT_view_mode = VIEW_MODE_COMPASS;
        EPD_display_frontpage = false;
      }

}   else if (TFT_view_mode == VIEW_MODE_TEXT) {
        if (next) {
          TFT_view_mode = VIEW_MODE_COMPASS;
          EPD_display_frontpage = false;
        }
        else {  
          TFT_view_mode = VIEW_MODE_RADAR;
          EPD_display_frontpage = false;
      }
    }
    else if (TFT_view_mode == VIEW_MODE_COMPASS) {
      if (next) {
        TFT_view_mode = VIEW_MODE_RADAR;
        EPD_display_frontpage = false;
      }
      else {
        TFT_view_mode = VIEW_MODE_TEXT; 
        EPD_display_frontpage = false;
    }
  }
}
}

void TFT_Up()
{
  if (hw_info.display == DISPLAY_TFT) {
    switch (TFT_view_mode)
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
    switch (TFT_view_mode)
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