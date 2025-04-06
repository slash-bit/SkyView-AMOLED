#include "TFTHelper.h"

// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold18pt7b.h>

#include <TimeLib.h>

#include "TrafficHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"
#include "BatteryHelper.h"
#include "JSONHelper.h"

#include "SkyView.h"
#include "Speed.h"
#include "altitude2.h"
#include "aircrafts.h"

// extern uint16_t read_voltage();
extern TFT_eSPI tft;
extern LilyGo_Class amoled;
extern TFT_eSprite sprite;
extern TFT_eSprite batterySprite;
extern TFT_eSprite textSprite;
TFT_eSprite bearingSprite = TFT_eSprite(&tft);
TFT_eSprite TextPopSprite = TFT_eSprite(&tft);
extern TFT_eSprite bottomSprite;


uint8_t TFT_current = 1;
uint8_t pages =0;
uint16_t lock_x = 40;
uint16_t lock_y = 15;
bool     isLocked = true;
uint16_t lock_color = TFT_GREEN;
uint8_t lock_open = 0;
extern buddy_info_t buddies[];
extern JsonArray labelsArray;
const char* label = "";
const char* buddy_name = "";
int   disp_alt, vertical;



void TFT_draw_text() {
  int j=0;
  int bearing;
  char info_line [TEXT_VIEW_LINE_LENGTH];
  char id2_text  [TEXT_VIEW_LINE_LENGTH];
  char id_text   [TEXT_VIEW_LINE_LENGTH];

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
    uint8_t db = 3;
    // const char *u_dist, *u_alt, *u_spd;
    // char id2_text[20];
    pages = j;
    
    // snprintf(id2_text, sizeof(id2_text), "ID: %02X%02X%02X", (traffic[TFT_current - 1].fop->ID >> 16) & 0xFF, (traffic[TFT_current - 1].fop->ID >> 8) & 0xFF, (traffic[TFT_current - 1].fop->ID) & 0xFF);      // Extract low byte
    
    if (TFT_current > j) {
      TFT_current = j;
    
    }
    bearing = (int) (traffic[TFT_current - 1].fop->RelativeBearing);  // relative to our track

    if (bearing < 0)    bearing += 360;
    if (bearing > 360)  bearing -= 360;
    uint32_t id = traffic[TFT_current - 1].fop->ID;
    long start = micros();
    int found = SoC->DB_query(db, id, id_text, sizeof(id_text), id2_text, sizeof(id2_text));
    if (found == 1) {
#if 1
      Serial.print(F("Time taken: "));
      Serial.println(micros()-start);
      Serial.print(F("Registration of "));
      Serial.print(id);
      Serial.print(F(" is "));
      Serial.println(id_text);
#endif
    } else {
      snprintf(id_text, sizeof(id_text), "ID: %06X", id);
      if (found == 2) {
        // found in DB but record is empty
        strncpy(id2_text, " (blank)", sizeof(id2_text));
      } else if (found == 0) {
        // have database, but ID not found within it
        strncpy(id2_text, " (no reg)", sizeof(id2_text));
      } else if (found == -1) {
        // no database
        strncpy(id2_text, "! NO DB !", sizeof(id2_text));
      } else {
        // SD card not mounted
        strncpy(id2_text, "! NO SD !", sizeof(id2_text));
      }
    }
    // if (!labelsArray || labelsArray.size() == 0) {
    //   Serial.println("[Error] labelsArray is not initialized or empty");
    //   return;
    // }
    // else {
    //   Serial.println("Labels Array size: " + labelsArray.size());
    //   for (size_t i = 0; i < labelsArray.size(); i++) {
    //     if (labelsArray[i]["hex"] == traffic[TFT_current - 1].fop->ID) {
    //       label = labelsArray[i]["label"];
    //       printf("ID: 0x%06X, Name: %s\n", traffic[TFT_current - 1].fop->ID, label);
    //     }
    //   }
    // }
    
    disableLoopWDT();
    
    for (size_t i = 0; buddies[i].id != 0xFFFFFFFF; i++) {
      if (buddies[i].id == traffic[TFT_current - 1].fop->ID) {
          buddy_name = buddies[i].name;
          printf("ID: 0x%06X, Name: %s\n", buddies[i].id, buddies[i].name);
      }
    }
    enableLoopWDT();
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
  textSprite.setTextDatum(TL_DATUM);
  textSprite.fillSprite(TFT_BLACK);
  textSprite.setTextColor(TFT_WHITE, TFT_BLACK);

  // sprite.drawString(traffic[TFT_current - 1].acftType == 7 ? "PG" : traffic[TFT_current - 1].acftType == 1 ? "G" : traffic[TFT_current - 1].acftType == 3 ? "H" : "GA", 87, 93, 4);
  // sprite.drawSmoothRoundRect(84, 82, 6, 5, 40, 40, TFT_WHITE);
  
  textSprite.setCursor(100, 30, 4);
  textSprite.printf("%s     %s",traffic[TFT_current - 1].acftType == 7 ? "PG" : traffic[TFT_current - 1].acftType == 1 ? "G" : traffic[TFT_current - 1].acftType == 3 ? "H" : traffic[TFT_current - 1].acftType == 9 ? "A" : "GA", id_text);
  textSprite. setFreeFont(&Orbitron_Light_32);
  textSprite.setCursor(80, 100);
  textSprite.printf(buddy_name ? buddy_name : info_line ? info_line: label ? label : "Uknown");

  textSprite.setCursor(120, 140, 4);
  textSprite.printf("Last seen: %ds ago", traffic[TFT_current - 1].lastSeen);

  textSprite.drawRoundRect(150, 180, 170, 10, 5, TFT_CYAN);
  textSprite.fillRoundRect(150, 180, traffic[TFT_current - 1].lastSeen > 17 ? 1 : 170 - traffic[TFT_current - 1].lastSeen * 10, 10, 5, TFT_CYAN);

  // sprite.drawString("Vertical", 27, 180, 4);
  textSprite.drawNumber((int)(traffic[TFT_current - 1].fop->RelativeVertical) * 3, 30, 253, 7);
  
  // textSprite.drawSmoothArc(233, 233, 230, 225, 0, 360, TFT_DARKGREY, TFT_BLACK, true);

  textSprite.drawString("Climbrate ", 323, 220, 4);
  // textSprite.drawString(" o'clock ", 217, 280, 4);

  textSprite.setTextDatum(TR_DATUM);
  textSprite.drawFloat(traffic_vario, 1, 405, 253, 7);
  textSprite.drawNumber(disp_alt, 90, 320, 4);
  textSprite.setTextDatum(TL_DATUM);
  textSprite.drawString("amsl", 100, 320, 2);
  textSprite.drawString("m", 400, 260, 4);
  textSprite.drawString("s", 405, 285, 4);

  textSprite.drawWideLine(410, 280, 430, 281, 3, TFT_WHITE);

  textSprite.setSwapBytes(true);
  textSprite.pushImage(40, 210, 32, 32, altitude2);

  textSprite.drawFloat((traffic[TFT_current - 1].fop->distance / 1000.0), 1, 170, 345, 7);
  textSprite.drawString("km", 250, 345, 4);
  /*
  textSprite.setSwapBytes(true);
  textSprite.pushImage(300, 300, 32, 24, Speed);
  textSprite.drawFloat(speed, 0, 340, 300, 6);
  textSprite.drawString("km", 400, 300, 4); //speed km
  textSprite.drawString("h", 400, 325, 4);  //speed h
  textSprite.drawWideLine(400, 322, 420, 322, 3, TFT_WHITE);
*/
/*
  if (vertical > 55) {
    textSprite.drawSmoothArc(225, 225, 230, 225, 90, vertical > 3000 ? 150 : (90 + vertical / 50), vertical > 500 ? TFT_CYAN : TFT_RED, TFT_BLACK, true);
    textSprite.drawString("+", 15, 226, 7);
    textSprite.drawWideLine(15, 236, 25, 236, 6, TFT_WHITE); //draw plus
    textSprite.drawWideLine(20, 231, 20, 241, 6, TFT_WHITE);
  }
  else if (vertical < -55) {
    textSprite.drawSmoothArc(225, 225, 230, 225, abs(vertical) > 3000 ? 150 : 90 - abs(vertical) / 50, 90, vertical < -500 ? TFT_GREEN : TFT_RED, TFT_BLACK, true);
    // textSprite.drawWideLine(15, 231, 25, 231, 6, TFT_WHITE); //draw minus
  }
  if (traffic_vario < -0.5) {
    textSprite.drawSmoothArc(225, 225, 230, 225, 270, 270 + abs(traffic_vario) * 12, traffic_vario < 2.5 ? TFT_BLUE : traffic_vario < 1 ? TFT_CYAN : TFT_GREEN, TFT_BLACK, true);
    
  }
  else if (traffic_vario > 0.3) { //exclude 0 case so not to draw 360 arch
    textSprite.drawSmoothArc(225, 225, 230, 225, 270 - abs(traffic_vario) * 12, 270, traffic_vario > 3.5 ? TFT_RED : traffic_vario > 2 ? TFT_ORANGE : TFT_YELLOW, TFT_BLACK, true);
  } 
  */
  //Bottom of page
  bottomSprite.createSprite(450, 150);
  bottomSprite.fillSprite(TFT_BLACK);
  bottomSprite.drawSmoothRoundRect(lock_x, lock_y, 6, 4, 30, 30, lock_color, TFT_BLACK);
  bottomSprite.drawArc(lock_x + lock_open + 15, lock_y, 12, 10, 90, 270, lock_color, TFT_BLACK);
  bottomSprite.setSwapBytes(true);
  bottomSprite.pushImage(190, 5, 32, 32, aircrafts);
  bottomSprite.drawNumber(Traffic_Count(), 240, 0, 6);
  //pages dots
  if (pages > 1) {
    for (int i = 1; i <= pages; i++)
    { uint16_t wd = (pages -1) * 18; // width of frame 8px per circle + 8px between circles

      bottomSprite.fillCircle((233 - wd /2) + i * 18, 110, 5, i == TFT_current ? TFT_WHITE : TFT_DARKGREY);

    }
  }

  bearingSprite.createSprite(78, 54);
  bearingSprite.fillSprite(TFT_BLACK);
  textSprite.setPivot(225, 280);

  bearingSprite.fillRect(0, 12, 40, 30, TFT_CYAN);
  bearingSprite.fillTriangle(40, 0, 40, 53, 78, 26, TFT_CYAN);
  bearingSprite.setPivot(39, 27);
  bearingSprite.pushRotated(&textSprite, bearing - 90);

  //Battery indicator
  battery_draw();
  uint16_t battery_x = 310;
  uint16_t battery_y = 5;
  batterySprite.pushToSprite(&textSprite, battery_x, battery_y, TFT_BLACK);

  amoled.setBrightness(250);
  amoled.pushColors(0, 0, LCD_WIDTH, 450, (uint16_t*)textSprite.getPointer());  
  amoled.pushColors(0, 450, LCD_WIDTH, 150, (uint16_t*)bottomSprite.getPointer());
  bottomSprite.deleteSprite();
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
    textSprite.setTextDatum(MC_DATUM);
    textSprite.fillSprite(TFT_BLACK);
    textSprite.setTextColor(TFT_CYAN, TFT_BLACK);
      //Battery indicator
    battery_draw();
    uint16_t battery_x = 310;
    uint16_t battery_y = 5;
    batterySprite.pushToSprite(&textSprite, battery_x, battery_y, TFT_BLACK);

    if (msg2 == NULL) {
      textSprite.drawString(msg1, LCD_WIDTH / 2, LCD_HEIGHT / 2, 4);
    } else {
      textSprite.drawString(msg1, LCD_WIDTH / 2, LCD_HEIGHT / 2 - 26, 4);
      textSprite.drawString(msg2, LCD_WIDTH / 2, LCD_HEIGHT / 2 + 26, 4);
    }
      bottomSprite.createSprite(450, 150);
      bottomSprite.fillSprite(TFT_BLACK);
      bottomSprite.setSwapBytes(true);

      bottomSprite.pushToSprite(&sprite, 0, 450, TFT_BLACK);
      amoled.setBrightness(0);
      amoled.pushColors(display_column_offset, 0, 450, 450, (uint16_t*)textSprite.getPointer());
      for (int i = 0; i <= 255; i++)
      {
        amoled.setBrightness(i);
          delay(2);
      }
      delay(200);
      for (int i = 255; i >= 0; i--)
      {
        amoled.setBrightness(i);
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
    amoled.pushColors(0, 0, 450, 450, (uint16_t*)textSprite.getPointer());
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
  TextPopSprite.pushToSprite(&textSprite, 50, 130, TFT_BLACK);
  amoled.pushColors(0, 0, 450, 450, (uint16_t*)textSprite.getPointer());
  // delay(500);
  TextPopSprite.deleteSprite();
  
}