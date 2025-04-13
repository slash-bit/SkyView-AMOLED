/*
 * View_Radar_TFT.cpp
 * Copyright (C) 2019-2021 Linar Yusupov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// this version follows
//   https://github.com/nbonniere/SoftRF/tree/master/software/firmware/source/SkyView
#if defined(USE_TFT)
#include "TFTHelper.h"
// #include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
// #include <Fonts/Picopixel.h>
// #include <Fonts/FreeMono24pt7b.h>
// #include <Fonts/FreeMonoBold12pt7b.h>
// #include <Fonts/FreeMonoBold24pt7b.h>

#include <TimeLib.h>

#include "TrafficHelper.h"
#include "BatteryHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"
#include "ApproxMath.h"

#include "SkyView.h"

#include <TFT_eSPI.h> // Include the TFT_eSPI library
#include <driver/display/CO5300.h>
#include "helicopter_image.h"
#include "glider_image.h"
#include "GA.h"
#include "aircrafts.h"
#include "aircraft_small.h"
#include "settings.h"
// #include <Adafruit_GFX.h> // Include the Adafruit_GFX library
// #include <Adafruit_ST7789.h> // Include the Adafruit ST7789 library

// extern Adafruit_ST7789 tft;
//define gfx
extern TFT_eSPI tft;
extern TFT_eSprite sprite;
extern TFT_eSprite sprite2;
TFT_eSprite arrowSprite = TFT_eSprite(&tft);
TFT_eSprite ownAcrft = TFT_eSprite(&tft);
TFT_eSprite pg = TFT_eSprite(&tft);
TFT_eSprite pgSprite = TFT_eSprite(&tft);
TFT_eSprite gaSprite = TFT_eSprite(&tft);
TFT_eSprite Acrft = TFT_eSprite(&tft);
TFT_eSprite helicopterSprite = TFT_eSprite(&tft);
TFT_eSprite gliderSprite = TFT_eSprite(&tft);
extern TFT_eSprite batterySprite;
TFT_eSprite aircraft = TFT_eSprite(&tft);



static navbox_t navbox1;
static navbox_t navbox2;
static navbox_t navbox3;
static navbox_t navbox4;

static int EPD_zoom = ZOOM_MEDIUM;

// #define ICON_AIRPLANE

#if defined(ICON_AIRPLANE)
//#define ICON_AIRPLANE_POINTS 6
//const int16_t epd_Airplane[ICON_AIRPLANE_POINTS][2] = {{0,-4},{0,10},{-8,0},{8,0},{-3,8},{3,8}};
#define ICON_AIRPLANE_POINTS 12
const float epd_Airplane[ICON_AIRPLANE_POINTS][2] = {{0,-4},{0,10},{-8,0},{9,0},{-3,8},{4,8},{1,-4},{1,10},{-10,1},{11,1},{-2,9},{3,9}};
#else  //ICON_AIRPLANE
#define ICON_ARROW_POINTS 4
const float epd_Arrow[ICON_ARROW_POINTS][2] = {{-6,5},{0,-6},{6,5},{0,2}};

const float own_Points[ICON_ARROW_POINTS][2] = {{-6,5},{0,-6},{6,5},{0,2}};
#endif //ICON_AIRPLANE
#define ICON_TARGETS_POINTS 5
const float epd_Target[ICON_TARGETS_POINTS][2] = {{4,4},{0,-6},{-4,4},{-5,-3},{0,2}};
// const float pg_Points[ICON_ARROW_POINTS][2] = {{-12,14},{0,-16},{12,14},{0,4}};
const float pg_Points[ICON_ARROW_POINTS][2] = {{-36, 42}, {0, -48}, {36, 42}, {0, 12}};

#define PG_TARGETS_POINTS 3 // triangle
const float pg_PointsUp[ICON_TARGETS_POINTS][2] = {{-10,8},{10,8},{0,-9}};
const float pg_PointsDown[ICON_TARGETS_POINTS][2] = {{-10,8},{10,8},{0,25}};

#define MAX_DRAW_POINTS 12

uint8_t sprite_center_x = 18;
uint8_t sprite_center_y = 18;

// 2D rotation
void EPD_2D_Rotate(float &tX, float &tY, float tCos, float tSin)
{
    float tTemp;
    tTemp = tX * tCos - tY * tSin;
    tY = tX * tSin + tY *  tCos;
    tX = tTemp;
}
#if !defined(ROUND_AMOLED)
static void EPD_Draw_NavBoxes()
{
  int16_t  tbx, tby;
  uint16_t tbw, tbh;

  uint16_t top_navboxes_x = navbox1.x;
  uint16_t top_navboxes_y = navbox1.y;
  uint16_t top_navboxes_w = navbox1.width + navbox2.width;
  uint16_t top_navboxes_h = maxof2(navbox1.height, navbox2.height);

  {
    //draw round boxes for navboxes #72A0C1 Air Superiority Blue
    gfx->fillRoundRect(top_navboxes_x, top_navboxes_y,
                      top_navboxes_w, top_navboxes_h, 2,
                      NAVBOX_COLOR);

    gfx->drawRoundRect( navbox1.x + 1, navbox1.y + 1,
                            navbox1.width - 2, navbox1.height - 2,
                            4, NAVBOX_FRAME_COLOR2);

    gfx->drawRoundRect( navbox2.x + 1, navbox2.y + 1,
                            navbox2.width - 2, navbox2.height - 2,
                            4, NAVBOX_FRAME_COLOR2);

    // draw title text in the navboxes, colour - #FF9933 - Deep Saffron
    gfx->setTextColor(NAVBOX_TEXT_COLOR);
    gfx->setTextSize(1);

    // gfx->getTextBounds(navbox1.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    gfx->setCursor(navbox1.x + 6, navbox1.y + 6);
    gfx->print(navbox1.title);

    // gfx->getTextBounds(navbox2.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    gfx->setCursor(navbox2.x + 6, navbox2.y + 6);
    gfx->print(navbox2.title);

    // Draw information text Deep Saffron
    gfx->setTextColor(NAVBOX_TEXT_COLOR);
    gfx->setTextSize(3);

    gfx->setCursor(navbox1.x + 70, navbox1.y + 10);
    gfx->print(navbox1.value);

    // Same for box 2
    gfx->setTextColor(NAVBOX_TEXT_COLOR);
    gfx->setTextSize(2);

    gfx->setCursor(navbox2.x + 44, navbox2.y + 12);
    gfx->print(navbox2.value == PROTOCOL_NMEA  ? "NMEA" :
                   navbox2.value == PROTOCOL_GDL90 ? " GDL" : " UNK" );
  }

  uint16_t bottom_navboxes_x = navbox3.x;
  uint16_t bottom_navboxes_y = navbox3.y;
  uint16_t bottom_navboxes_w = navbox3.width + navbox4.width;
  uint16_t bottom_navboxes_h = maxof2(navbox3.height, navbox4.height);

  {
    gfx->fillRoundRect(bottom_navboxes_x, bottom_navboxes_y,
                      bottom_navboxes_w, bottom_navboxes_h, 2,
                      NAVBOX_COLOR);

    gfx->drawRoundRect( navbox3.x + 1, navbox3.y + 1,
                            navbox3.width - 2, navbox3.height - 2,
                            4, NAVBOX_FRAME_COLOR2);
    gfx->drawRoundRect( navbox4.x + 1, navbox4.y + 1,
                            navbox4.width - 2, navbox4.height - 2,
                            4, NAVBOX_FRAME_COLOR2);

    // Title text color orange
    gfx->setTextColor(NAVBOX_TEXT_COLOR);
    gfx->setTextSize(1);

    // gfx->getTextBounds(navbox3.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    gfx->setCursor(navbox3.x + 6, navbox3.y + 6);
    gfx->print(navbox3.title);

    // gfx->getTextBounds(navbox4.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    gfx->setCursor(navbox4.x + 6, navbox4.y + 6);
    gfx->print(navbox4.title);

    // Info text
    gfx->setTextColor(NAVBOX_TEXT_COLOR);
    gfx->setTextSize(3);

    gfx->setCursor(navbox3.x + 50, navbox3.y + 10);
    Serial.print(navbox3.x);
    Serial.print(navbox3.y);
    Serial.println(navbox3.value);

    if (settings->units == UNITS_METRIC || settings->units == UNITS_MIXED) {
      gfx->print(navbox3.value == ZOOM_LOWEST ? "10km" :
                     navbox3.value == ZOOM_LOW    ? " 5km" :
                     navbox3.value == ZOOM_MEDIUM ? " 2km" :
                     navbox3.value == ZOOM_HIGH   ? " 1km" : "");
    } else {
      gfx->print(navbox3.value == ZOOM_LOWEST ? " 5nm" :
                     navbox3.value == ZOOM_LOW    ? "2.5nm" :
                     navbox3.value == ZOOM_MEDIUM ? " 1nm" :
                     navbox3.value == ZOOM_HIGH   ? "0.5nm" : "");
    }

    // Set color for battery text ( TBD chnage according to charge level)
    gfx->setTextColor(NAVBOX_TEXT_COLOR);
    gfx->setTextSize(3);

    gfx->setCursor(navbox4.x + 40, navbox4.y + 10);
    gfx->print((float) navbox4.value);
  }
}
#endif /* !defined(ROUND_AMOLED) */
void TFT_radar_Draw_Message(const char *msg1, const char *msg2)
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
      //Battery indicator
    // battery_draw();
    // uint16_t battery_x = 300;
    // uint16_t battery_y = 5;
    // batterySprite.pushToSprite(&sprite, battery_x, battery_y, TFT_BLACK);

    if (msg2 == NULL) {
      sprite.drawString(msg1, LCD_WIDTH / 2, LCD_HEIGHT / 2, 4);
    } else {
      sprite.drawString(msg1, LCD_WIDTH / 2, LCD_HEIGHT / 2 - 66, 4);
      sprite.drawString(msg2, LCD_WIDTH / 2, LCD_HEIGHT / 2 + 26, 4);
    }
      //draw settings icon
    sprite.setSwapBytes(true);
    sprite.pushImage(320, 360, 36, 36, settings_icon_small);
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

static void TFT_Draw_Radar()
{
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  uint16_t x;
  uint16_t y;
  char cog_text[6];
  int16_t scale;
  
  /* divider is distance range */
  int32_t divider = 2000; // default 2000m 

  // draw radar
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_GREEN, TFT_BLACK);
  sprite.setTextDatum(MC_DATUM);
  lcd_brightness(255);
  sprite.setTextSize(1);

  uint16_t radar_x = 0;
  uint16_t radar_y = (LCD_HEIGHT - LCD_WIDTH) / 2;
  uint16_t radar_w = LCD_WIDTH;

  uint16_t radar_center_x = radar_w / 2;
  uint16_t radar_center_y = radar_y + radar_w / 2;
  uint16_t radius = radar_w / 2 - 2;

  float epd_Points[MAX_DRAW_POINTS][2];
  float pg_Points[MAX_DRAW_POINTS][2];

  if (settings->units == UNITS_METRIC || settings->units == UNITS_MIXED) {
    switch (EPD_zoom)
    {
      case ZOOM_LOWEST:
        divider = 9000; /* 9 km 20 KM */
        break;
      case ZOOM_LOW:
        divider =  6000; /* 6KM was 10 KM */
        break;
      case ZOOM_HIGH:
        divider =  900; /* 0.9km was 2 KM */
        break;
      case ZOOM_MEDIUM:
      default:
        divider =  3000;  /* 3KM was 4 KM */
        break;
    }
  } else {
    switch (EPD_zoom) {
      case ZOOM_LOWEST:
        divider = 9260;  /* 10 NM */
        break;
      case ZOOM_LOW:
        divider = 4630;  /*  5 NM */
        break;
      case ZOOM_HIGH:
        divider =  926;  /*  1 NM */
        break;
      case ZOOM_MEDIUM:  /*  2 NM */
      default:
        divider = 1852;
        break;
    }
  }
    // draw range circles
    // draw range circles in grey
    sprite.drawSmoothCircle(radar_center_x, radar_center_y, radius - 5,   NAVBOX_FRAME_COLOR2, TFT_BLACK);
    sprite.drawSmoothCircle(radar_center_x, radar_center_y, round(radius / 3), NAVBOX_FRAME_COLOR2, TFT_BLACK);
    sprite.drawSmoothCircle(radar_center_x, radar_center_y, round(radius / 3 * 2), NAVBOX_FRAME_COLOR2, TFT_BLACK);
    //draw crosshair in the center
    // sprite.drawLine(radar_center_x - 10, radar_center_y, radar_center_x + 10, radar_center_y, NAVBOX_FRAME_COLOR2);
    // sprite.drawLine(radar_center_x, radar_center_y - 10, radar_center_x, radar_center_y + 10, NAVBOX_FRAME_COLOR2);

        //draw distance marker as numbers on radar circles diaganolly from center to bottom right
    //draw distance marker as numbers on radar circles diaganolly from center to bottom right
    uint16_t circle_mark1_x = radar_center_x + round(radius/3 * 0.7071);
    uint16_t circle_mark1_y = radar_center_y + round(radius/3 * 0.7071);

    uint16_t circle_mark2_x = radar_center_x + round(radius/3*2 * 0.7071);
    uint16_t circle_mark2_y = radar_center_y + round(radius/3*2 * 0.7071);

    uint16_t circle_mark3_x = radar_center_x + round(radius * 0.7071) - 4;
    uint16_t circle_mark3_y = radar_center_y + round(radius * 0.7071) - 4;

    uint16_t zoom = (navbox3.value == ZOOM_LOWEST ? 9 :
                    navbox3.value == ZOOM_LOW    ? 6 :
                    navbox3.value == ZOOM_MEDIUM ? 3 :
                    navbox3.value == ZOOM_HIGH   ? 1 : 1);

    sprite.setCursor(circle_mark1_x, circle_mark1_y);
    // divide scale by 2 , if resul hs a decimal point, print only point and the decimal
    sprite.drawString(zoom == 1 ? ".3" : zoom == 3 ? "1" : zoom == 6 ? "2" : "3", circle_mark1_x, circle_mark1_y, 4);   
    sprite.drawString(zoom == 1 ? ".6" : zoom == 3 ? "2" : zoom == 6 ? "4" : "6", circle_mark2_x, circle_mark2_y, 4); ;
    sprite.drawNumber(zoom, circle_mark3_x, circle_mark3_y, 4);
#if defined(DEBUG_HEAP) 
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
#endif
    switch (settings->orientation)
    {
    case DIRECTION_NORTH_UP:
        // draw W, E, N, S
        tbw = tft.textWidth("W", 4);
        tbh = tft.fontHeight(4);
        x = radar_x + radar_w / 2 - radius + tbw / 2;
        y = radar_y + (radar_w + tbh) / 2;
        sprite.drawString("W", x, y, 4);
        x = radar_x + radar_w / 2 + radius - tbw / 2;
        y = radar_y + radar_w / 2;
        sprite.drawString("E", x, y, 4);
        x = radar_x + radar_w / 2;
        y = radar_y + radar_w / 2 - radius + tbh / 2;
        sprite.drawString("N", x, y, 4);
        x = radar_x + radar_w / 2;
        y = radar_y + radar_w / 2 + radius - tbh / 2; 
        sprite.drawString("S", x, y, 4);
        break;
    case DIRECTION_TRACK_UP:
        // draw L, R, B
        x = radar_x + radar_w / 2 - radius + tbw + 2;
        y = radar_y + (radar_w + tbh) / 2;
        sprite.drawString("L", x, y, 4);
        x = radar_x + radar_w / 2 + radius - tbw - 2;
        y = radar_y + (radar_w + tbh) / 2;
        sprite.drawString("R", x, y, 4);
        // x = radar_x + (radar_w - tbw) / 2;
        // y = radar_y + radar_w / 2 + radius - tbh / 2;
        // gfx->setCursor(x, y);
        // gfx->print("B");

        // draw aircraft heading
        sprite.setTextColor(TFT_ORANGE, TFT_BLACK);
        snprintf(cog_text, sizeof(cog_text), "%03d", ThisAircraft.Track);
        // gfx->getTextBounds(cog_text, 0, 0, &tbx, &tby, &tbw, &tbh);
        tbw = tft.textWidth(cog_text, 7);
        tbh = tft.fontHeight(7);
        x = radar_x + radar_w / 2;
        y = radar_y + radar_w / 2 - radius + tbh / 2 + 4;
        sprite.drawSmoothRoundRect(x - tbw /2 - 2, y - tbh /2, 3, 4, tbw + 4, tbh + 4, NAVBOX_FRAME_COLOR2, TFT_BLACK);
        sprite.drawString(cog_text, x, y, 7);
        break;
    default:
  /* TBD */
    break;
        }

  {
    // Calculating traffic
    float trSin = sin_approx(-ThisAircraft.Track);
    float trCos = cos_approx(-ThisAircraft.Track);    
    for (int i=0; i < MAX_TRACKING_OBJECTS; i++) {
      if (Container[i].ID && (now() - Container[i].timestamp) <= TFT_EXPIRATION_TIME && Container[i].ID != settings->team) {

        float rel_x;
        float rel_y;
        float tgtSin;
        float tgtCos;
        float climb;
        int color;

        bool isTeam = (Container[i].ID == settings->team);

        rel_x = Container[i].RelativeEast;
        rel_y = Container[i].RelativeNorth;
        climb = Container[i].ClimbRate;
        tgtSin = sin_approx(Container[i].Track);
        tgtCos = cos_approx(Container[i].Track);

        for (int i=0; i < ICON_TARGETS_POINTS; i++) {
          epd_Points[i][0] = epd_Target[i][0];
          epd_Points[i][1] = epd_Target[i][1];
          EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], tgtCos, tgtSin);
        }
        // for (int i=0; i < PG_TARGETS_POINTS; i++) {
        //   pg_Points[i][0] = pg_Target[i][0];
        //   pg_Points[i][1] = pg_Target[i][1];
        // }
#if defined(DEBUG_CONTAINER)
        Serial.print(F(" ID="));
        Serial.print((Container[i].ID >> 16) & 0xFF, HEX);
        Serial.print((Container[i].ID >>  8) & 0xFF, HEX);
        Serial.print((Container[i].ID      ) & 0xFF, HEX);
        Serial.println();

        Serial.print(F(" RelativeNorth=")); Serial.println(Container[i].RelativeNorth);
        Serial.print(F(" RelativeEast="));  Serial.println(Container[i].RelativeEast);
        Serial.print(F(" RelativeVertical="));  Serial.println(Container[i].RelativeVertical);
        Serial.print(F(" ClimbRate="));  Serial.println(Container[i].ClimbRate);
        Serial.print(F(" AcftType="));  Serial.println(Container[i].AcftType);

#endif
        switch (settings->orientation) {
          case DIRECTION_NORTH_UP:
            break;
          case DIRECTION_TRACK_UP:
            // rotate relative to ThisAircraft.Track
            EPD_2D_Rotate(rel_x, rel_y, trCos, trSin);
            for (int i=0; i < ICON_TARGETS_POINTS; i++)
               EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], trCos, -trSin);
            for (int i=0; i < PG_TARGETS_POINTS; i++)
                EPD_2D_Rotate(pg_Points[i][0], pg_Points[i][1], trCos, -trSin);
            break;
          default:
            /* TBD */
            break;
        }

#if defined(DEBUG_POSITION)
      Serial.print(F("Debug "));
      Serial.print(trSin);
      Serial.print(F(", "));
      Serial.print(trCos);
      Serial.print(F(", "));
      Serial.print(rel_x);
      Serial.print(F(", "));
      Serial.print(rel_y);
      Serial.println();
      Serial.flush();
#endif
#if defined(DEBUG_POSITION)
      Serial.print(F("Debug "));
      Serial.print(tgtSin);
      Serial.print(F(", "));
      Serial.print(tgtCos);
      Serial.print(F(", "));
      Serial.print(epd_Points[1][0]);
      Serial.print(F(", "));
      Serial.print(epd_Points[1][1]);
      Serial.println();
      Serial.flush();
#endif

        int16_t x = constrain((rel_x * radius) / divider, -32768, 32767);
        int16_t y = constrain((rel_y * radius) / divider, -32768, 32767);
#if defined(DEBUG_HEAP)
        Serial.print("Free heap: ");
        Serial.println(ESP.getFreeHeap());
        Serial.println("TFT_Draw_Radar: Draw  Traffic");
#endif
        scale = Container[i].alarm_level + 1;
        //color based on ClimbRate
        if (now() - Container[i].timestamp >= TEAM_EXPIRATION_TIME) {
          color = TFT_DARKGREY;
        } else if (Container[i].RelativeVertical >  TFT_RADAR_V_THRESHOLD) {
          color = TFT_CYAN;
        } else if (Container[i].RelativeVertical < -TFT_RADAR_V_THRESHOLD) {
          color = TFT_GREEN;
        } else {
          color = TFT_RED;
        }
        switch (Container[i].AcftType)
        {
          case 1: //Glider
                  //Container[i].alarm_level
              gliderSprite.createSprite(70, 62);
              gliderSprite.fillSprite(TFT_BLACK);
              gliderSprite.setSwapBytes(true);
              gliderSprite.pushImage(0, 0, 70, 62, glider);
              gliderSprite.setPivot(35, 31);
              sprite.setPivot(radar_center_x + x, radar_center_y - y);
              gliderSprite.pushRotated(&sprite, Container[i].Track), TFT_BLACK;
             
            break;

          case 3:    //helicopter
            helicopterSprite.createSprite(48, 64);
            helicopterSprite.fillSprite(TFT_BLACK);
            helicopterSprite.setSwapBytes(true);
            helicopterSprite.pushImage(0, 0, 48, 64, helicopter);  
            helicopterSprite.setPivot(24, 32);
            sprite.setPivot(radar_center_x + x, radar_center_y - y);
        
            helicopterSprite.pushRotated(&sprite, Container[i].Track, TFT_BLACK);

            break;
          // case 6: //hang glider


          //   break;
          case 7: //Paraglider -  draw target as triangle
            sprite_center_x = 18;
            sprite_center_y = 18;
            color = TFT_RED;
            pgSprite.createSprite(36, 36); 
            pgSprite.fillSprite(TFT_BLACK);
            pgSprite.setSwapBytes(1);
            pgSprite.setPivot(18, 18);
   
            // const float pg_Points[ICON_ARROW_POINTS][2] = {{-12,14},{0,-16},{12,14},{0,4}};
            pgSprite.drawWideLine(6, 32, 18, 2, 2, color, TFT_BLACK);
            pgSprite.drawWideLine(18, 2, 30, 32, 2, color, TFT_BLACK);
            pgSprite.drawWideLine(30, 32, 18, 22, 2, color, TFT_BLACK);
            pgSprite.drawWideLine(18, 22, 6, 32, 2, color, TFT_BLACK);
            if (isTeam) {
              pgSprite.drawSmoothCircle(sprite_center_x,
                sprite_center_y,
                14, TFT_LIGHTGREY, TFT_BLACK);
            }


            sprite.setPivot(radar_center_x + x, radar_center_y - y);
            pgSprite.pushRotated(&sprite, Container[i].Track, TFT_BLACK);
            break;
            case 9: //Aircraft
              aircraft.createSprite(60, 48);
              aircraft.setSwapBytes(1);
              aircraft.fillSprite(TFT_BLACK);
              aircraft.pushImage(0, 0, 60, 48, aircraft_small);
              aircraft.setPivot(30, 24);
              sprite.setPivot(radar_center_x + x, radar_center_y - y);
              aircraft.pushRotated(&sprite, Container[i].Track, TFT_BLACK);
            break;
          case 11: //Baloon
            sprite.fillSmoothCircle(radar_center_x + x,
                          radar_center_y - y,
                            18, TFT_RED);
            sprite.fillRect(radar_center_x + x - 17, radar_center_y - y - 1, 34, 4, TFT_BLACK);
            sprite.fillCircle(radar_center_x + x,
                            radar_center_y - y + 2,
                            4, TFT_BLACK);
            break;
          default:
            Serial.print(F("Unexpected AcftType value: "));
            Serial.println(Container[i].AcftType);
            gaSprite.fillSprite(TFT_BLACK);
            gaSprite.setSwapBytes(true);
            gaSprite.pushImage(0, 0, 70, 70, GA);
            gaSprite.setPivot(35, 35);
            sprite.setPivot(radar_center_x + x, radar_center_y - y);
            gaSprite.pushRotated(&sprite, Container[i].Track, TFT_BLACK);

            break;
        }

      }
    }

#if defined(ICON_AIRPLANE)
    /* draw little airplane */
    for (int i=0; i < ICON_AIRPLANE_POINTS; i++) {
        epd_Points[i][0] = epd_Airplane[i][0];
        epd_Points[i][1] = epd_Airplane[i][1];
    }
    switch (settings->orientation)
    {
    case DIRECTION_NORTH_UP:
        // rotate relative to ThisAircraft.Track
        for (int i=0; i < ICON_AIRPLANE_POINTS; i++) {
        EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], trCos, trSin);
        }
        break;
    case DIRECTION_TRACK_UP:
        break;
    default:
        /* TBD */
        break;
    }
    sprite.drawLine(radar_center_x + (int) epd_Points[0][0],
                radar_center_y + (int) epd_Points[0][1],
                radar_center_x + (int) epd_Points[1][0],
                radar_center_y + (int) epd_Points[1][1],
                WHITE);
    sprite.drawLine(radar_center_x + (int) epd_Points[2][0],
                radar_center_y + (int) epd_Points[2][1],
                radar_center_x + (int) epd_Points[3][0],
                radar_center_y + (int) epd_Points[3][1],
                WHITE);
    sprite.drawLine(radar_center_x + (int) epd_Points[4][0],
                radar_center_y + (int) epd_Points[4][1],
                radar_center_x + (int) epd_Points[5][0],
                radar_center_y + (int) epd_Points[5][1],
                WHITE);
    sprite.drawLine(radar_center_x + (int) epd_Points[6][0],
                radar_center_y + (int) epd_Points[6][1],
                radar_center_x + (int) epd_Points[7][0],
                radar_center_y + (int) epd_Points[7][1],
                WHITE);
    sprite.drawLine(radar_center_x + (int) epd_Points[8][0],
                radar_center_y + (int) epd_Points[8][1],
                radar_center_x + (int) epd_Points[9][0],
                radar_center_y + (int) epd_Points[9][1],
                WHITE);
    sprite.drawLine(radar_center_x + (int) epd_Points[10][0],
                radar_center_y + (int) epd_Points[10][1],
                radar_center_x + (int) epd_Points[11][0],
                radar_center_y + (int) epd_Points[11][1],
                WHITE);
#else  //ICON_AIRPLANE
    /* draw arrow tip */
    for (int i=0; i < ICON_ARROW_POINTS; i++) {
        epd_Points[i][0] = epd_Arrow[i][0];
        epd_Points[i][1] = epd_Arrow[i][1];
    }
    switch (settings->orientation)
    {
    case DIRECTION_NORTH_UP:
    // rotate relative to ThisAircraft.Track
    for (int i=0; i < ICON_ARROW_POINTS; i++) {
      EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], trCos, trSin);
    }
        break;
    case DIRECTION_TRACK_UP:
        break;
    default:
        /* TBD */
        break;
    }
    // draw own aircaft
    ownAcrft.createSprite(36, 36);  
    ownAcrft.fillSprite(TFT_BLACK);
    ownAcrft.setPivot(18, 18);
    sprite_center_x = 18;
    sprite_center_y = 18;
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[0][0],
    sprite_center_y + 3 * (int) own_Points[0][1],
    sprite_center_x + 3 * (int) own_Points[1][0],
    sprite_center_y + 3 * (int) own_Points[1][1],4,
    TFT_DARKGREY, TFT_DARKGREY);
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[1][0],
    sprite_center_y + 3 * (int) own_Points[1][1],
    sprite_center_x + 3 * (int) own_Points[2][0],
    sprite_center_y + 3 * (int) own_Points[2][1],4,
    TFT_DARKGREY, TFT_DARKGREY);
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[2][0],
    sprite_center_y + 3 * (int) own_Points[2][1],
    sprite_center_x + 3 * (int) own_Points[3][0],
    sprite_center_y + 3 * (int) own_Points[3][1],4,
    TFT_DARKGREY, TFT_DARKGREY);
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[3][0],
    sprite_center_y + 3 * (int) own_Points[3][1],
    sprite_center_x + 3 * (int) own_Points[0][0],
    sprite_center_y + 3 * (int) own_Points[0][1],4,
    TFT_DARKGREY, TFT_DARKGREY);
        /////// double line
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[0][0],
    sprite_center_y + 3 * (int) own_Points[0][1],
    sprite_center_x + 3 * (int) own_Points[1][0],
    sprite_center_y + 3 * (int) own_Points[1][1],2,
    TFT_WHITE, TFT_DARKGREY);
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[1][0],
    sprite_center_y + 3 * (int) own_Points[1][1],
    sprite_center_x + 3 * (int) own_Points[2][0],
    sprite_center_y + 3 * (int) own_Points[2][1],2,
    TFT_WHITE, TFT_DARKGREY); 
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[2][0],
    sprite_center_y + 3 * (int) own_Points[2][1],
    sprite_center_x + 3 * (int) own_Points[3][0],
    sprite_center_y + 3 * (int) own_Points[3][1],2,
    TFT_WHITE, TFT_DARKGREY);
    ownAcrft.drawWideLine(sprite_center_x + 3 * (int) own_Points[3][0],
    sprite_center_y + 3 * (int) own_Points[3][1],
    sprite_center_x + 3 * (int) own_Points[0][0],
    sprite_center_y + 3 * (int) own_Points[0][1],2,
    TFT_WHITE, TFT_DARKGREY);
    sprite.setPivot(233, 233);                             
    ownAcrft.pushRotated(&sprite, ThisAircraft.Track, TFT_BLACK);

#endif //ICON_AIRPLANE
  lcd_PushColors(6, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t*)sprite.getPointer()); 

    }
}


void TFT_radar_setup()
{
  EPD_zoom = settings->zoom;
#if !defined(ROUND_AMOLED)
  uint16_t radar_x = 0;
  uint16_t radar_y = 0;
  uint16_t radar_w = 0;

  radar_y = (320 - 240) / 2;
  radar_w = 240;


  memcpy(navbox1.title, NAVBOX1_TITLE, strlen(NAVBOX1_TITLE));
  navbox1.x = 0;
  navbox1.y = 0;


  navbox1.width  = 240 / 2;
  navbox1.height = (320 - 240) / 2;
  

  navbox1.value      = 0;
  navbox1.timestamp  = millis();

  memcpy(navbox2.title, NAVBOX2_TITLE, strlen(NAVBOX2_TITLE));
  navbox2.x = navbox1.width;
  navbox2.y = navbox1.y;
  navbox2.width  = navbox1.width;
  navbox2.height = navbox1.height;
  navbox2.value      = PROTOCOL_NONE;
  navbox2.timestamp  = millis();

  memcpy(navbox3.title, NAVBOX3_TITLE, strlen(NAVBOX3_TITLE));
  navbox3.x = 0;
  navbox3.y = radar_y + radar_w;
  navbox3.width  = navbox1.width;
  navbox3.height = navbox1.height;
  navbox3.value      = EPD_zoom;
  navbox3.timestamp  = millis();

  memcpy(navbox4.title, NAVBOX4_TITLE, strlen(NAVBOX4_TITLE));
  navbox4.x = navbox3.width;
  navbox4.y = navbox3.y;
  navbox4.width  = navbox3.width;
  navbox4.height = navbox3.height;
  navbox4.value      = (int) (Battery_voltage() * 10.0);
  navbox4.timestamp  = millis();
  #endif /* (ROUND_AMOLED) */
}

void TFT_radar_loop()
{
  if (isTimeToDisplay()) {



    bool hasData = settings->protocol == PROTOCOL_NMEA  ? NMEA_isConnected()  :
                   settings->protocol == PROTOCOL_GDL90 ? GDL90_isConnected() :
                   false;

    if (hasData) {

      bool hasFix = settings->protocol == PROTOCOL_NMEA  ? isValidGNSSFix()   :
                    settings->protocol == PROTOCOL_GDL90 ? GDL90_hasOwnShip() :
                    false;

      if (hasFix) {
        TFT_Draw_Radar();
      } else {
        Serial.println("No FIX");
        TFT_radar_Draw_Message(NO_FIX_TEXT, NULL);
      }
    } else {
      Serial.println("No DATA");
      // String ipAddress = WiFi.softAPIP().toString();
      // const char* msg2 = ("Wifi IP Address " + ipAddress).c_str();
      
      TFT_radar_Draw_Message(NO_DATA_TEXT, NULL);
    }

    navbox1.value = Traffic_Count();

    switch (settings->protocol)
    {
    case PROTOCOL_GDL90:
      navbox2.value = GDL90_hasHeartBeat() ?
                      PROTOCOL_GDL90 : PROTOCOL_NONE;
      break;
    case PROTOCOL_NMEA:
    default:
      navbox2.value = (NMEA_hasFLARM() || NMEA_hasGNSS()) ?
                      PROTOCOL_NMEA  : PROTOCOL_NONE;
      break;
    }

    navbox3.value = EPD_zoom;
    navbox4.value = (int) (Battery_voltage() * 10.0);

    // EPD_Draw_NavBoxes();
    yield();
#if defined(DEBUG_HEAP)
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
#endif /* DEBUG_HEAP */
    TFTTimeMarker = millis();
  }
}

void TFT_radar_zoom()
{
  if (EPD_zoom < ZOOM_HIGH) EPD_zoom++;
  sprite2.createSprite(120, 40);
  sprite2.fillSprite(TFT_BLACK);
  sprite2.setTextColor(TFT_ORANGE, TFT_BLACK);
  sprite2.setTextDatum(MC_DATUM);
  sprite2.setSwapBytes(true);
  sprite2.drawString("ZOOM IN", 60, 20, 4);
  sprite2.pushToSprite(&sprite, 173, 120, TFT_BLACK);
  lcd_PushColors(6, 0, 466, 466, (uint16_t*)sprite.getPointer());
  sprite2.deleteSprite();
  // delay(500);
  sprite2.deleteSprite();
}

void TFT_radar_unzoom()
{
  if (EPD_zoom > ZOOM_LOWEST) EPD_zoom--;
  sprite2.createSprite(120, 40);
  sprite2.fillSprite(TFT_BLACK);
  sprite2.setTextColor(TFT_ORANGE, TFT_BLACK);
  sprite2.setTextDatum(MC_DATUM);
  sprite2.setSwapBytes(true);
  sprite2.drawString("ZOOM OUT", 60, 20, 4);
  
  sprite2.pushToSprite(&sprite, 173, 330, TFT_BLACK);
  lcd_PushColors(6, 0, 466, 466, (uint16_t*)sprite.getPointer());
  // delay(500);
  sprite2.deleteSprite();
}
#endif /* USE_TFT */