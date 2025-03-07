#include "TFTHelper.h"

// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold18pt7b.h>

#include <TimeLib.h>

#include "TrafficHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"

#include "SkyView.h"

extern TFT_eSPI tft;
extern TFT_eSprite sprite;

static int TFT_current = 1;

void TFT_draw_text() {
  Serial.println("TFT_draw_text");
  int j=0;
  int bearing;
  char info_line [TEXT_VIEW_LINE_LENGTH];
  char id_text   [TEXT_VIEW_LINE_LENGTH];
  char id2_text  [TEXT_VIEW_LINE_LENGTH];

  for (int i=0; i < MAX_TRACKING_OBJECTS; i++) {
    if (Container[i].ID && (now() - Container[i].timestamp) <= TFT_EXPIRATION_TIME) {

      traffic[j].fop = &Container[i];
      traffic[j].distance = Container[i].distance;
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
    snprintf(id_text, sizeof(id_text), "ID: %02X%02X%02X", (Container[0].ID >> 16) & 0xFF, (Container[0].ID >> 8) & 0xFF, (Container[0].ID) & 0xFF);      // Extract low byte
    if (TFT_current > j) {
      TFT_current = j;
    }
    bearing = (int) (traffic[TFT_current - 1].fop->RelativeBearing);  // relative to our track

    if (bearing < 0)    bearing += 360;
    if (bearing > 360)  bearing -= 360;

    int oclock = ((bearing + 15) % 360) / 30;
    vertical = (int) traffic[TFT_current - 1].fop->RelativeVertical;

#if defined(DEBUG_CONTAINER)
    Serial.print(F(" ID="));
    Serial.print((traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, HEX);
    Serial.print((traffic[TFT_current - 1].fop->ID >>  8) & 0xFF, HEX);
    Serial.print((traffic[TFT_current - 1].fop->ID      ) & 0xFF, HEX);
    Serial.println();

    Serial.print(F(" RelativeNorth=")); Serial.println(traffic[TFT_current - 1].fop->RelativeNorth);
    Serial.print(F(" RelativeEast="));  Serial.println(traffic[TFT_current - 1].fop->RelativeEast);
    Serial.print(F(" RelativeBearing="));  Serial.println(traffic[TFT_current - 1].fop->RelativeBearing);
    Serial.print(F(" RelativeVertical="));  Serial.println(traffic[TFT_current - 1].fop->RelativeVertical);
    Serial.print(F(" ClimbRate="));  Serial.println(traffic[TFT_current - 1].fop->ClimbRate);
    Serial.print(F(" Distance"));  Serial.println(traffic[TFT_current - 1].fop->distance);
    Serial.print(F(" lastSeen="));  Serial.println(traffic[TFT_current - 1].lastSeen);
    Serial.print("Barry width: ");
    Serial.println(sprite.textWidth("Barry"));
    Serial.print("Barry height: ");
    Serial.println(sprite.fontHeight(4));
    Serial.print("Last seen: ");
    Serial.println(sprite.textWidth("Last seen: "));
    Serial.print("ft Vertical width: ");
    Serial.println(sprite.textWidth("ft Vertical"));
    Serial.print("Climbrate: ");
    Serial.println(sprite.textWidth("Climbrate "));
    Serial.print("Distance: ");
    Serial.println(sprite.textWidth("Distance: "));
#endif
  sprite.setTextDatum(TL_DATUM);
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  sprite.drawString("Barry", 140, 58, 4);
  sprite.drawString(id_text, 140, 93, 4);
  sprite.drawString("Last seen: ", 140, 123, 4);
  sprite.drawNumber(traffic[TFT_current - 1].lastSeen, 260, 123, 4);
  sprite.drawString("s ago", 290, 123, 4);
  sprite.drawRoundRect(140, 160, 170, 10, 5, TFT_CYAN);
  sprite.fillRoundRect(140, 160, traffic[TFT_current - 1].lastSeen > 60 ? 1 : 170 - traffic[TFT_current - 1].lastSeen * 2, 10, 5, TFT_CYAN);
  sprite.drawString("Vertical ft", 27, 180, 4);
  sprite.drawString("+", 20, 226, 4);
  sprite.drawNumber((int)(traffic[TFT_current - 1].fop->RelativeVertical) * 3, 30, 213, 7);
  sprite.drawSmoothArc(233, 233, 230, 225, 0, 360, TFT_DARKGREY, TFT_BLACK, true);
  sprite.drawSmoothArc(233, 233, 230, 225, 90, 90 + vertical * 10, vertical >= 0 ? TFT_CYAN : TFT_LIGHTGREY, TFT_BLACK, true);
  if (vertical >=0) {
    sprite.drawSmoothArc(233, 233, 230, 225, 90, 90 + vertical, vertical > 500 ? TFT_CYAN : TFT_RED, TFT_BLACK, true);
  }
  
  sprite.drawString("Climbrate ", 327, 190, 4);
  sprite.drawFloat((traffic[TFT_current - 1].fop->ClimbRate), 1, 365, 226, 4);
  sprite.drawString("m/s", 395, 230, 4);
  sprite.drawNumber(oclock, 160, 290, 5);
  sprite.drawString(" o'clock ", 190, 297, 4);
  sprite.drawString("Distance:  ", 116, 325, 4);
  sprite.drawFloat((traffic[TFT_current - 1].fop->distance / 1000.0), 1, 250, 325, 4);
  sprite.drawString("km", 280, 325, 4);
  sprite.drawString("Alt:  ", 120, 360, 4);
  sprite.drawNumber(abs((int) traffic[TFT_current - 1].fop->altitude * 3), 230, 360, 4);
  sprite.drawString("ft", 270, 360, 4);
  //place red arrow bitmap ( 1bit)
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
    sprite.setTextColor(TFT_RED, TFT_BLACK);

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
  }
}

void TFT_text_prev()
{
  if (TFT_current > 1) {
    TFT_current--;
  }
}