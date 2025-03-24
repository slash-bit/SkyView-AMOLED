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
#include "Speed.h"
#include "altitude2.h"
#include "aircrafts.h"

extern uint16_t read_voltage();
extern TFT_eSPI tft;
extern TFT_eSprite sprite;
TFT_eSprite bearingSprite = TFT_eSprite(&tft);
TFT_eSprite TextPopSprite = TFT_eSprite(&tft);


uint8_t TFT_current = 1;
uint8_t pages =0;
uint32_t battery = 0;
uint8_t batteryPercentage = 0;
uint16_t lock_x = 110;
uint16_t lock_y = 345;
bool     isLocked = true;
uint16_t lock_color = TFT_GREEN;
uint8_t lock_open = 0;
extern buddy_info_t buddies[];
const char* buddy_name = "Unknown";
int   disp_alt, vertical;



void TFT_draw_text() {
  int j=0;
  int bearing;
  // char info_line [TEXT_VIEW_LINE_LENGTH];
  char id2_text  [TEXT_VIEW_LINE_LENGTH];

  for (int i=0; i < MAX_TRACKING_OBJECTS; i++) {
    if (Container[i].ID && (now() - Container[i].timestamp) <= TFT_EXPIRATION_TIME) {

      traffic[j].fop = &Container[i];
      traffic[j].distance = Container[i].distance;
      // traffic[j].altitude = (int)((float)Container[i].altitude * 3.0);
      traffic[j].altitude = traffic[j].fop->altitude;
      traffic[j].climbrate = Container[i].ClimbRate;
      traffic[j].acftType = Container[i].AcftType;
      traffic[j].lastSeen = now() - Container[i].timestamp;
      j++;
    }
  }
  // uint_fast16_t lastSeen = now() -  Container[i].timestamp;
  if (j > 0) {
    // uint8_t db;
    // const char *u_dist, *u_alt, *u_spd;
    // char id2_text[20];
    pages = j;
    // snprintf(id2_text, sizeof(id_text), "%02X%02X%02X", (traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, (traffic[TFT_current - 1].fop->ID >> 8) & 0xFF, (traffic[TFT_current - 1].fop->ID) & 0xFF);      // Extract low byte
    snprintf(id2_text, sizeof(id2_text), "ID: %02X%02X%02X", (traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, (traffic[TFT_current - 1].fop->ID >> 8) & 0xFF, (traffic[TFT_current - 1].fop->ID) & 0xFF);      // Extract low byte
    }
    if (TFT_current > j) {
      TFT_current = j;
    
    }
    bearing = (int) (traffic[TFT_current - 1].fop->RelativeBearing);  // relative to our track

    if (bearing < 0)    bearing += 360;
    if (bearing > 360)  bearing -= 360;

    for (size_t i = 0; buddies[i].id != 0xFFFFFFFF; i++) {
      if (buddies[i].id == traffic[TFT_current - 1].fop->ID) {
          buddy_name = buddies[i].name;
          printf("ID: 0x%06X, Name: %s\n", buddies[i].id, buddies[i].name);
      }
    // int oclock = ((bearing + 15) % 360) / 30;
    vertical = (int) traffic[TFT_current - 1].fop->RelativeVertical;
    int disp_alt = (int)((vertical + ThisAircraft.altitude) * 3);  //converting meter to feet
    float traffic_vario = (traffic[TFT_current - 1].climbrate);
    float speed = (traffic[TFT_current - 1].fop->GroundSpeed);


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
    Serial.print(F(" Altitude= "));  Serial.println(traffic[TFT_current - 1].altitude);
    Serial.print(F(" ClimbRate= "));  Serial.println(traffic[TFT_current - 1].climbrate);
    Serial.print(F(" Distance= "));  Serial.println(traffic[TFT_current - 1].fop->distance);
    Serial.print(F(" Speed= "));  Serial.println(speed);
    Serial.print(F(" lastSeen= "));  Serial.println(traffic[TFT_current - 1].lastSeen);
    

#endif
  sprite.setTextDatum(TL_DATUM);
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  sprite.drawString(traffic[TFT_current - 1].acftType == 7 ? "PG" : traffic[TFT_current - 1].acftType == 1 ? "G" : traffic[TFT_current - 1].acftType == 3 ? "H" : "GA", 87, 93, 4);
  sprite.drawSmoothRoundRect(84, 82, 6, 5, 40, 40, TFT_WHITE);
  
  sprite.drawString(buddy_name, 140, 58, 4);
  sprite.drawString(id2_text, 140, 93, 4);
  sprite.setCursor(140,123,4);
  sprite.printf("Last seen: %ds ago", traffic[TFT_current - 1].lastSeen);

  sprite.drawRoundRect(150, 160, 170, 10, 5, TFT_CYAN);
  sprite.fillRoundRect(150, 160, traffic[TFT_current - 1].lastSeen > 120 ? 1 : 170 - traffic[TFT_current - 1].lastSeen * 2, 10, 5, TFT_CYAN);

  // sprite.drawString("Vertical", 27, 180, 4);
  sprite.drawNumber((int)(traffic[TFT_current - 1].fop->RelativeVertical) * 3, 30, 213, 7);
  
  sprite.drawSmoothArc(233, 233, 230, 225, 0, 360, TFT_DARKGREY, TFT_BLACK, true);

  sprite.drawString("Climbrate ", 327, 180, 4);
  // sprite.drawString(" o'clock ", 217, 280, 4);

  sprite.setTextDatum(TR_DATUM);
  sprite.drawFloat(traffic_vario, 1, 425, 213, 7);
  sprite.drawNumber(disp_alt, 90, 280, 4);
  sprite.setTextDatum(TL_DATUM);
  sprite.drawString("amsl", 100, 280, 2);
  sprite.drawString("m", 430, 215, 4);
  sprite.drawString("s", 435, 240, 4);

  sprite.drawWideLine(430, 240, 450, 241, 3, TFT_WHITE);

  sprite.setSwapBytes(true);
  sprite.pushImage(40, 160, 32, 32, altitude2);

  sprite.drawFloat((traffic[TFT_current - 1].fop->distance / 1000.0), 1, 170, 285, 7);
  sprite.drawString("km", 250, 285, 4);
    
  sprite.setSwapBytes(true);
  sprite.pushImage(300, 300, 32, 24, Speed);
  sprite.drawFloat(speed, 0, 340, 300, 6);
  sprite.drawString("km", 400, 300, 4); //speed km
  sprite.drawString("h", 400, 325, 4);  //speed h
  sprite.drawWideLine(400, 322, 420, 322, 3, TFT_WHITE);

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
  // Lock page
  sprite.drawSmoothRoundRect(lock_x, lock_y, 6, 4, 30, 30, lock_color, TFT_BLACK);
  sprite.drawArc(lock_x + lock_open + 15, lock_y, 12, 10, 90, 270, lock_color, TFT_BLACK);
  sprite.setSwapBytes(true);
  sprite.pushImage(190, 370, 32, 32, aircrafts);
  sprite.drawNumber(Traffic_Count(), 240, 365, 6);

  bearingSprite.createSprite(78, 54);
  bearingSprite.fillSprite(TFT_BLACK);
  sprite.setPivot(233, 233);

  bearingSprite.fillRect(0, 12, 40, 30, TFT_CYAN);
  bearingSprite.fillTriangle(40, 0, 40, 53, 78, 26, TFT_CYAN);
  bearingSprite.setPivot(39, 27);
  bearingSprite.pushRotated(&sprite, bearing - 90);
  //Battery indicator
  uint16_t battery_x = 295;
  uint16_t battery_y = 35;
    // draw battery symbol
  sprite.drawRoundRect(battery_x, battery_y, 32, 20, 3, TFT_CYAN);
  sprite.fillRect(battery_x + 32, battery_y + 7, 2, 7, TFT_CYAN);
  
  if (isTimeToBattery()) {
    battery = read_voltage();
    Serial.print(F(" Battery= "));  Serial.println(battery);
    batteryPercentage = ((battery - 3300) / (4200 - 3300)) * 100.0;
    Serial.print(F(" Batterypercentage= "));  Serial.println(batteryPercentage);
  }
  sprite.fillRect(battery_x , battery_y + 2, (int)(batteryPercentage / 5) + 5, 14, TFT_CYAN);
  sprite.setCursor(battery_x, battery_y + 24, 4);
  sprite.printf("%d%%", batteryPercentage); // Use %% to print the % character
  if (pages > 1) {
    for (int i = 1; i <= pages; i++)
    { uint16_t wd = (pages -1) * 18; // width of frame 8px per circle + 8px between circles

      sprite.fillCircle((233 - wd /2) + i * 18, 425, 5, i == TFT_current ? TFT_WHITE : TFT_DARKGREY);

    }
  }
  lcd_brightness(200);
  lcd_PushColors(6, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t*)sprite.getPointer());        
  }
  Serial.print("TFT_current: "); Serial.println(TFT_current);
  Serial.print("pages: "); Serial.println(pages);
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
    if (TFT_current < pages) {
      TFT_current++;
    }
    else {
      TFT_current = 1;
    }
    Serial.print("TFT_current: ");
    Serial.println(TFT_current);
    TextPopSprite.createSprite(233, 40);
    TextPopSprite.fillSprite(TFT_BLACK);
    TextPopSprite.setTextColor(TFT_ORANGE, TFT_BLACK);
    TextPopSprite.setTextDatum(MC_DATUM);
    TextPopSprite.setCursor(115, 20, 8);
    TextPopSprite.printf("NEXT");

    
    TextPopSprite.pushToSprite(&sprite, 50, 260, TFT_BLACK);
    lcd_PushColors(0, 0, 466, 466, (uint16_t*)sprite.getPointer());
    // delay(500);
    TextPopSprite.deleteSprite();
  }
}


void TFT_text_prev()
{
  TextPopSprite.createSprite(233, 40);
  TextPopSprite.fillSprite(TFT_BLACK);
  TextPopSprite.setTextColor(TFT_ORANGE, TFT_BLACK);
  TextPopSprite.setTextDatum(MC_DATUM);

  if (TFT_current > 1) {
    TFT_current--;
    Serial.print("TFT_current: ");
    Serial.println(TFT_current);
    TextPopSprite.drawString("PREV", 115, 20, 8);
  } else {
    TextPopSprite.drawString("NO PREV", 115, 20, 8);
  }
  TextPopSprite.pushToSprite(&sprite, 50, 130, TFT_BLACK);
  lcd_PushColors(0, 0, 466, 466, (uint16_t*)sprite.getPointer());
  // delay(500);
  TextPopSprite.deleteSprite();
  
}