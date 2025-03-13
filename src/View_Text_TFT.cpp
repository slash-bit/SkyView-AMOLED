#include "TFTHelper.h"

// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold18pt7b.h>

#include <TimeLib.h>

#include "TrafficHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"
#include "BatteryHelper.h"

unsigned long Battery_TimeMarker = 0;

#include "SkyView.h"

extern TFT_eSPI tft;
extern TFT_eSprite sprite;
TFT_eSprite bearingSprite = TFT_eSprite(&tft);
TFT_eSprite TextPopSprite = TFT_eSprite(&tft);


static int TFT_current = 1;
static int pages =0;
static uint8_t battery = 0;
void TFT_draw_text() {
  int j=0;
  int bearing;
  char info_line [TEXT_VIEW_LINE_LENGTH];
  char id_text   [TEXT_VIEW_LINE_LENGTH];
  char id2_text  [TEXT_VIEW_LINE_LENGTH];

  for (int i=0; i < MAX_TRACKING_OBJECTS; i++) {
    if (Container[i].ID && (now() - Container[i].timestamp) <= TFT_EXPIRATION_TIME) {

      traffic[j].fop = &Container[i];
      traffic[j].distance = Container[i].distance;
      traffic[j].altitude = Container[i].altitude;
      traffic[j].climbrate = Container[i].ClimbRate;
      traffic[j].lastSeen = now() - Container[i].timestamp;
      j++;
    }
  }
  // uint_fast16_t lastSeen = now() -  Container[i].timestamp;
  if (j > 0) {
    uint8_t db;
    const char *u_dist, *u_alt, *u_spd;
    float disp_dist;
    int   disp_alt, disp_spd, vertical;
    char id_text[20];
    // snprintf(id2_text, sizeof(id_text), "%02X%02X%02X", (traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, (traffic[TFT_current - 1].fop->ID >> 8) & 0xFF, (traffic[TFT_current - 1].fop->ID) & 0xFF);      // Extract low byte
    snprintf(id2_text, sizeof(id2_text), "ID: %02X%02X%02X", (traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, (traffic[TFT_current - 1].fop->ID >> 8) & 0xFF, (traffic[TFT_current - 1].fop->ID) & 0xFF);      // Extract low byte
    if (TFT_current > j) {
      TFT_current = j;
      pages = j;
    }
    bearing = (int) (traffic[TFT_current - 1].fop->RelativeBearing);  // relative to our track

    if (bearing < 0)    bearing += 360;
    if (bearing > 360)  bearing -= 360;

    int oclock = ((bearing + 15) % 360) / 30;
    vertical = (int) traffic[TFT_current - 1].fop->RelativeVertical;
    float traffic_vario = (traffic[TFT_current - 1].climbrate);
    if (isTimeToBattery()) {
      battery = (int) (Battery_voltage() * 10.0);
    }

#if defined(DEBUG_CONTAINER)
    Serial.print(F(" ID="));
    Serial.print((traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, HEX);
    Serial.print((traffic[TFT_current - 1].fop->ID >>  8) & 0xFF, HEX);
    Serial.print((traffic[TFT_current - 1].fop->ID      ) & 0xFF, HEX);

    Serial.println();
    // Serial.print(F(" RelativeNorth=")); Serial.println(traffic[TFT_current - 1].fop->RelativeNorth);
    // Serial.print(F(" RelativeEast="));  Serial.println(traffic[TFT_current - 1].fop->RelativeEast);
    // Serial.print(F(" RelativeBearing="));  Serial.println(traffic[TFT_current - 1].fop->RelativeBearing);
    // Serial.print(F(" RelativeVertical="));  Serial.println(traffic[TFT_current - 1].fop->RelativeVertical);
    Serial.print(F(" Altitude= "));  Serial.println(traffic[TFT_current - 1].fop->altitude);
    Serial.print(F(" ClimbRate= "));  Serial.println(traffic[TFT_current - 1].climbrate);
    Serial.print(F(" Distance= "));  Serial.println(traffic[TFT_current - 1].fop->distance);
    Serial.print(F(" lastSeen= "));  Serial.println(traffic[TFT_current - 1].lastSeen);
    Serial.print(F(" Battery= "));  Serial.println(battery);

#endif
  sprite.setTextDatum(TL_DATUM);
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  sprite.drawString((traffic[TFT_current - 1].fop->ID == settings->team) ? "TEAM Pilot" : "", 140, 58, 4);
  sprite.drawString(id2_text, 140, 93, 4);
  sprite.setCursor(140,123,4);
  sprite.printf("Last seen: %ds ago", traffic[TFT_current - 1].lastSeen);

  sprite.drawRoundRect(150, 160, 170, 10, 5, TFT_CYAN);
  sprite.fillRoundRect(150, 160, traffic[TFT_current - 1].lastSeen > 120 ? 1 : 170 - traffic[TFT_current - 1].lastSeen * 2, 10, 5, TFT_CYAN);

  sprite.drawString("Vertical", 27, 180, 4);
  sprite.drawNumber((int)(traffic[TFT_current - 1].fop->RelativeVertical) * 3, 30, 213, 7);
  
  sprite.drawSmoothArc(233, 233, 230, 225, 0, 360, TFT_DARKGREY, TFT_BLACK, true);

  sprite.drawString("Climbrate ", 327, 180, 4);
  sprite.drawString(" o'clock ", 217, 280, 4);

  sprite.setTextDatum(TR_DATUM);
  sprite.drawFloat(traffic_vario, 1, 425, 213, 7);
  sprite.setTextDatum(TL_DATUM);
 
  sprite.drawString("m", 430, 215, 4);
  sprite.drawString("s", 435, 240, 4);

  sprite.drawWideLine(430, 240, 450, 241, 3, TFT_WHITE); //draw \

  sprite.drawNumber(oclock, 180, 280, 7);

  sprite.setCursor(190, 410, 4);
  sprite.printf("Alt: %d' ", (ThisAircraft.altitude * 3));

  sprite.setCursor(60, 410, 4);
  sprite.printf("%d V", battery);

  sprite.setTextDatum(BL_DATUM);


  sprite.drawFloat((traffic[TFT_current - 1].fop->distance / 1000.0), 1, 230, 380, 7);
  sprite.drawString("km", 310, 375, 4);
    
  sprite.setTextDatum(TL_DATUM);
  if (vertical > 55) {
    sprite.drawSmoothArc(233, 233, 230, 225, 90, vertical > 3000 ? 150 : (90 + vertical / 50), vertical > 500 ? TFT_CYAN : TFT_RED, TFT_BLACK, true);
    sprite.drawString("+", 15, 226, 7);
    sprite.drawWideLine(15, 236, 25, 236, 6, TFT_WHITE); //draw plus
    sprite.drawWideLine(20, 231, 20, 241, 6, TFT_WHITE);
  }
  else if (vertical < -55) {
    sprite.drawSmoothArc(233, 233, 230, 225, abs(vertical) > 3000 ? 150 : 90 - abs(vertical) / 50, 90, vertical < -500 ? TFT_GREEN : TFT_RED, TFT_BLACK, true);
    // sprite.drawWideLine(15, 231, 25, 231, 6, TFT_WHITE); //draw minus
  }
  if (traffic_vario < -0.5) {
    sprite.drawSmoothArc(233, 233, 230, 225, 270, 270 + abs(traffic_vario) * 12, traffic_vario < 2.5 ? TFT_BLUE : traffic_vario < 1 ? TFT_CYAN : TFT_GREEN, TFT_BLACK, true);
    
  }
  else if (traffic_vario > 0.3) { //exclude 0 case so not to draw 360 arch
    sprite.drawSmoothArc(233, 233, 230, 225, 270 - abs(traffic_vario) * 12, 270, traffic_vario > 3.5 ? TFT_RED : traffic_vario > 2 ? TFT_ORANGE : TFT_YELLOW, TFT_BLACK, true);
  }

  bearingSprite.createSprite(78, 54);
  bearingSprite.fillSprite(TFT_BLACK);
  sprite.setPivot(233, 233);

  bearingSprite.fillRect(0, 12, 40, 30, TFT_CYAN);
  bearingSprite.fillTriangle(40, 0, 40, 53, 78, 26, TFT_CYAN);
  bearingSprite.setPivot(39, 27);
  bearingSprite.pushRotated(&sprite, bearing - 90);
  // sprite.setCursor(300,50,4);
  // sprite.printf("%.1f V", ESP32_Battery_voltage());

  lcd_brightness(200);
  lcd_PushColors(6, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t*)sprite.getPointer());        
}
}
  
void TFT_text_Draw_Message(const char *msg1, const char *msg2)
{
    // int16_t  tbx, tby;
    // uint16_t tbw, tbh;
    // uint16_t x, y;

  if (msg1 != NULL && strlen(msg1) != 0) {
    // uint16_t radar_x = 0;
    // uint16_t radar_y = 466 / 2;
    // uint16_t radar_w = 466;
    sprite.setTextDatum(MC_DATUM);
    sprite.fillSprite(TFT_BLACK);
    sprite.setTextColor(TFT_CYAN, TFT_BLACK);

    if (msg2 == NULL) {
      sprite.drawString(msg1, LCD_WIDTH / 2, LCD_HEIGHT / 2, 4);
    } else {
      sprite.drawString(msg1, LCD_WIDTH / 2, LCD_HEIGHT / 2 - 26, 4);
      sprite.drawString(msg2, LCD_WIDTH / 2, LCD_HEIGHT / 2 + 26, 4);
    }
      lcd_brightness(0);
      lcd_PushColors(display_column_offset, 0, 466, 466, (uint16_t*)sprite.getPointer());
      for (int i = 0; i <= 255; i++)
      {
        lcd_brightness(i);
          delay(2);
      }
      delay(200);
      for (int i = 255; i >= 0; i--)
      {
        lcd_brightness(i);
          delay(2);
      }
  }
}

void TFT_text_loop()
{
  if (isTimeToDisplay()) {
    Serial.println("TFT_text_loop - Time to display");
    bool hasData = settings->protocol == PROTOCOL_NMEA  ? NMEA_isConnected()  :
                   settings->protocol == PROTOCOL_GDL90 ? GDL90_isConnected() :
                   false;

    if (hasData) {

      bool hasFix = settings->protocol == PROTOCOL_NMEA  ? isValidGNSSFix()   :
                    settings->protocol == PROTOCOL_GDL90 ? GDL90_hasOwnShip() :
                    false;

      if (hasFix) {
        if (Traffic_Count() > 0) {
          TFT_draw_text();
        } else {
          TFT_text_Draw_Message("NO", "TRAFFIC");
        }
      } else {
        TFT_text_Draw_Message(NO_FIX_TEXT, NULL);
      }
    } else {
      TFT_text_Draw_Message(NO_DATA_TEXT, NULL);
    }

    EPDTimeMarker = millis();
  }
}

void TFT_text_next()
{
  if (TFT_current < MAX_TRACKING_OBJECTS) {
    TFT_current++;
    Serial.print("TFT_current: ");
    Serial.println(TFT_current);
    TextPopSprite.createSprite(233, 35);
    TextPopSprite.fillSprite(TFT_BLACK);
    TextPopSprite.setTextColor(TFT_ORANGE, TFT_BLACK);
    TextPopSprite.setTextDatum(MC_DATUM);
    TextPopSprite.setCursor(115, 15, 4);
    TextPopSprite.printf("NEXT");

    
    TextPopSprite.pushToSprite(&sprite, 50, 260, TFT_BLACK);
    lcd_PushColors(0, 0, 466, 466, (uint16_t*)sprite.getPointer());
    // delay(500);
    TextPopSprite.deleteSprite();
  }
}

void TFT_text_prev()
{
  if (TFT_current > 1) {
    TFT_current--;
    Serial.print("TFT_current: ");
    Serial.println(TFT_current);
    TextPopSprite.createSprite(233, 35);
    TextPopSprite.fillSprite(TFT_BLACK);
    TextPopSprite.setTextColor(TFT_ORANGE, TFT_BLACK);
    TextPopSprite.setTextDatum(MC_DATUM);
    TextPopSprite.setCursor(115, 15, 4);
    TextPopSprite.printf("PREV");

    
    TextPopSprite.pushToSprite(&sprite, 50, 130, TFT_BLACK);
    lcd_PushColors(0, 0, 466, 466, (uint16_t*)sprite.getPointer());
    // delay(500);
    TextPopSprite.deleteSprite();
  }
}