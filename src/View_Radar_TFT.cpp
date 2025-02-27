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

#include <Fonts/Picopixel.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

#include <TimeLib.h>

#include "TrafficHelper.h"
#include "BatteryHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"
#include "ApproxMath.h"

#include "SkyView.h"

// #include <TFT_eSPI.h> // Include the TFT_eSPI library
#include <Adafruit_GFX.h> // Include the Adafruit_GFX library
#include <Adafruit_ST7789.h> // Include the Adafruit ST7789 library

extern Adafruit_ST7789 tft;

extern GFXcanvas16 canvas_radar;

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
#endif //ICON_AIRPLANE
#define ICON_TARGETS_POINTS 5
const float epd_Target[ICON_TARGETS_POINTS][2] = {{4,4},{0,-6},{-4,4},{-5,-3},{0,2}};

#define MAX_DRAW_POINTS 12

// 2D rotation
void EPD_2D_Rotate(float &tX, float &tY, float tCos, float tSin)
{
    float tTemp;
    tTemp = tX * tCos - tY * tSin;
    tY = tX * tSin + tY *  tCos;
    tX = tTemp;
}

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
    tft.fillRoundRect(top_navboxes_x, top_navboxes_y,
                      top_navboxes_w, top_navboxes_h, 2,
                      NAVBOX_COLOR);

    tft.drawRoundRect( navbox1.x + 1, navbox1.y + 1,
                            navbox1.width - 2, navbox1.height - 2,
                            4, NAVBOX_FRAME_COLOR2);

    tft.drawRoundRect( navbox2.x + 1, navbox2.y + 1,
                            navbox2.width - 2, navbox2.height - 2,
                            4, NAVBOX_FRAME_COLOR2);

    // draw title text in the navboxes, colour - #FF9933 - Deep Saffron
    tft.setTextColor(NAVBOX_TEXT_COLOR);
    tft.setTextSize(1);

    // tft.getTextBounds(navbox1.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    tft.setCursor(navbox1.x + 6, navbox1.y + 6);
    tft.print(navbox1.title);

    // tft.getTextBounds(navbox2.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    tft.setCursor(navbox2.x + 6, navbox2.y + 6);
    tft.print(navbox2.title);

    // Draw information text Deep Saffron
    tft.setTextColor(NAVBOX_TEXT_COLOR);
    tft.setTextSize(3);

    tft.setCursor(navbox1.x + 70, navbox1.y + 10);
    tft.print(navbox1.value);

    // Same for box 2
    tft.setTextColor(NAVBOX_TEXT_COLOR);
    tft.setTextSize(2);

    tft.setCursor(navbox2.x + 44, navbox2.y + 12);
    tft.print(navbox2.value == PROTOCOL_NMEA  ? "NMEA" :
                   navbox2.value == PROTOCOL_GDL90 ? " GDL" : " UNK" );
  }

  uint16_t bottom_navboxes_x = navbox3.x;
  uint16_t bottom_navboxes_y = navbox3.y;
  uint16_t bottom_navboxes_w = navbox3.width + navbox4.width;
  uint16_t bottom_navboxes_h = maxof2(navbox3.height, navbox4.height);

  {
    tft.fillRoundRect(bottom_navboxes_x, bottom_navboxes_y,
                      bottom_navboxes_w, bottom_navboxes_h, 2,
                      NAVBOX_COLOR);

    tft.drawRoundRect( navbox3.x + 1, navbox3.y + 1,
                            navbox3.width - 2, navbox3.height - 2,
                            4, NAVBOX_FRAME_COLOR2);
    tft.drawRoundRect( navbox4.x + 1, navbox4.y + 1,
                            navbox4.width - 2, navbox4.height - 2,
                            4, NAVBOX_FRAME_COLOR2);

    // Title text color orange
    tft.setTextColor(NAVBOX_TEXT_COLOR);
    tft.setTextSize(1);

    // tft.getTextBounds(navbox3.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    tft.setCursor(navbox3.x + 6, navbox3.y + 6);
    tft.print(navbox3.title);

    // tft.getTextBounds(navbox4.title, 0, 0, &tbx, &tby, &tbw, &tbh);
    tft.setCursor(navbox4.x + 6, navbox4.y + 6);
    tft.print(navbox4.title);

    // Info text
    tft.setTextColor(NAVBOX_TEXT_COLOR);
    tft.setTextSize(3);

    tft.setCursor(navbox3.x + 50, navbox3.y + 10);
    Serial.print(navbox3.x);
    Serial.print(navbox3.y);
    Serial.println(navbox3.value);

    if (settings->units == UNITS_METRIC || settings->units == UNITS_MIXED) {
      tft.print(navbox3.value == ZOOM_LOWEST ? "10km" :
                     navbox3.value == ZOOM_LOW    ? " 5km" :
                     navbox3.value == ZOOM_MEDIUM ? " 2km" :
                     navbox3.value == ZOOM_HIGH   ? " 1km" : "");
    } else {
      tft.print(navbox3.value == ZOOM_LOWEST ? " 5nm" :
                     navbox3.value == ZOOM_LOW    ? "2.5nm" :
                     navbox3.value == ZOOM_MEDIUM ? " 1nm" :
                     navbox3.value == ZOOM_HIGH   ? "0.5nm" : "");
    }

    // Set color for battery text ( TBD chnage according to charge level)
    tft.setTextColor(NAVBOX_TEXT_COLOR);
    tft.setTextSize(3);

    tft.setCursor(navbox4.x + 40, navbox4.y + 10);
    tft.print((float) navbox4.value);
  }
}

void TFT_radar_Draw_Message(const char *msg1, const char *msg2)
{
  int16_t  tbx, tby;
  uint16_t tbw, tbh;
  uint16_t x, y;

  if (msg1 != NULL && strlen(msg1) != 0) {
    uint16_t radar_x = 0;
    uint16_t radar_y = (320 - 240) / 2;
    uint16_t radar_w = 240;
    tft.setTextColor(ST77XX_RED);
    // tft.setFreeFont(&FreeMonoBold18pt7b);


    tft.fillRect(radar_x, radar_y, radar_w, radar_w, ST77XX_BLACK);

    if (msg2 == NULL) {
      tft.getTextBounds(msg1, 0, 0, &tbx, &tby, &tbw, &tbh);
      x = (radar_w - tbw) / 2;
      y = radar_y + (radar_w + tbh) / 2;
      tft.setCursor(x, y);
      tft.print(msg1);
    } else {
      tft.getTextBounds(msg1, 0, 0, &tbx, &tby, &tbw, &tbh);
      x = (radar_w - tbw) / 2;
      y = radar_y + radar_w / 2 - tbh;
      tft.setCursor(x, y);
      tft.print(msg1);

      tft.getTextBounds(msg2, 0, 0, &tbx, &tby, &tbw, &tbh);
      x = (radar_w - tbw) / 2;
      y = radar_y + radar_w / 2 + tbh;
      tft.setCursor(x, y);
      tft.print(msg2);
      }
    
  }
}

static void EPD_Draw_Radar()
{
  Serial.println("EPD_Draw_Radar");
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  uint16_t x;
  uint16_t y;
  char cog_text[6];
  int16_t scale;
  
  /* divider is distance range */
  int32_t divider = 2000; // default 2000m 

  // draw radar
  // tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 40, 240, 240, ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  // tft.setTextColor(ST77XX_GREEN);
  // tft.setTextSize(2);
  tft.getTextBounds("N", 0, 0, &tbx, &tby, &tbw, &tbh);

  uint16_t radar_x = 0;
  uint16_t radar_y = (tft.height() - tft.width()) / 2;
  uint16_t radar_w = tft.width();
  // Colour codes can be obtained from https://www.webfx.com/web-design/color-picker/color-chart/ values have to be converted to 565 hex format. Example #536878 > 0x534F
  // tft.fillRect(radar_x, radar_y, radar_w, radar_w, ST77XX_BLACK);

  uint16_t radar_center_x = radar_w / 2;
  uint16_t radar_center_y = radar_y + radar_w / 2;
  uint16_t radius = radar_w / 2 - 2;

  float epd_Points[MAX_DRAW_POINTS][2];

  if (settings->units == UNITS_METRIC || settings->units == UNITS_MIXED) {
    switch (EPD_zoom)
    {
      case ZOOM_LOWEST:
        divider = 10000; /* 20 KM */
        break;
      case ZOOM_LOW:
        divider =  5000; /* 10 KM */
        break;
      case ZOOM_HIGH:
        divider =  1000; /*  2 KM */
        break;
      case ZOOM_MEDIUM:
      default:
        divider =  2000;  /* 4 KM */
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

  {
    float trSin = sin_approx(-ThisAircraft.Track);
    float trCos = cos_approx(-ThisAircraft.Track);
    Serial.println("Calculating traffic");
    for (int i=0; i < MAX_TRACKING_OBJECTS; i++) {
      if (Container[i].ID && (now() - Container[i].timestamp) <= EPD_EXPIRATION_TIME) {

        float rel_x;
        float rel_y;
        float tgtSin;
        float tgtCos;
        float climb;
        int Acfttype;
        int color;

        bool isTeam = (Container[i].ID == settings->team);

        rel_x = Container[i].RelativeEast;
        rel_y = Container[i].RelativeNorth;
        climb = Container[i].ClimbRate;
        Acfttype = Container[i].AcftType;
        tgtSin = sin_approx(Container[i].Track);
        tgtCos = cos_approx(Container[i].Track);

        for (int i=0; i < ICON_TARGETS_POINTS; i++) {
          epd_Points[i][0] = epd_Target[i][0];
          epd_Points[i][1] = epd_Target[i][1];
          EPD_2D_Rotate(epd_Points[i][0], epd_Points[i][1], tgtCos, tgtSin);
        }
#if defined(DEBUG_CONTAINER)
        Serial.print(F(" ID="));
        Serial.print((Container[i].ID >> 16) & 0xFF, HEX);
        Serial.print((Container[i].ID >>  8) & 0xFF, HEX);
        Serial.print((Container[i].ID      ) & 0xFF, HEX);
        Serial.println();

        Serial.print(F(" RelativeNorth=")); Serial.println(Container[i].RelativeNorth);
        Serial.print(F(" RelativeEast="));  Serial.println(Container[i].RelativeEast);
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
        Serial.println("EPD_Draw_Radar: Draw  Traffic");
        scale = Container[i].alarm_level + 1;
        //color based on ClimbRate
        if (Container[i].RelativeVertical >  EPD_RADAR_V_THRESHOLD) {
          color = ST77XX_GREEN;
        } else if (Container[i].RelativeVertical < -EPD_RADAR_V_THRESHOLD) {
          color = ST77XX_BLUE;
        } else {
          color = ST77XX_ORANGE;
        }
        switch (Container[i].alarm_level)
        {
        case 0:
          switch (Acfttype)
          {
          case 7: //Paraglider -  draw target as triangle
          // based on climb/sink rate point triangle up or down
            if (climb >= 0) {
              tft.fillTriangle(radar_center_x + x + (int)epd_Points[0][0],
                                radar_center_y - y + (int)epd_Points[0][1],
                                radar_center_x + x + (int)epd_Points[1][0],
                                radar_center_y - y + (int)epd_Points[1][1],
                                radar_center_x + x + (int)epd_Points[4][0],
                                radar_center_y - y + (int)epd_Points[4][1],
                                color);
            } else {
              tft.fillTriangle(radar_center_x + x + (int)epd_Points[2][0],
                                radar_center_y - y + (int)epd_Points[2][1],
                                radar_center_x + x + (int)epd_Points[1][0],
                                radar_center_y - y + (int)epd_Points[1][1],
                                radar_center_x + x + (int)epd_Points[4][0],
                                radar_center_y - y + (int)epd_Points[4][1],
                                color);
            }
            //draw circle around it, if it is team
            if (isTeam) {
              tft.drawCircle(radar_center_x + x,
                              radar_center_y - y,
                              12, ST77XX_WHITE);
              if (Container[i].RelativeVertical >  EPD_RADAR_V_THRESHOLD) {
                // draw a '+' next to target triangle if buddy's relative height is more than 500ft
                tft.drawLine(radar_center_x + x - 2 + (int) epd_Points[3][0],
                            radar_center_y - y     + (int) epd_Points[3][1],
                            radar_center_x + x + 2 + (int) epd_Points[3][0],
                            radar_center_y - y     + (int) epd_Points[3][1],
                            ST77XX_WHITE);
                tft.drawLine(radar_center_x + x     + (int) epd_Points[3][0],
                            radar_center_y - y + 2 + (int) epd_Points[3][1],
                            radar_center_x + x     + (int) epd_Points[3][0],
                            radar_center_y - y - 2 + (int) epd_Points[3][1],
                            ST77XX_WHITE);
              } else if (Container[i].RelativeVertical < -EPD_RADAR_V_THRESHOLD) {
                // draw a '-' next to target triangle
                tft.drawLine(radar_center_x + x - 2 + (int) epd_Points[3][0],
                            radar_center_y - y     + (int) epd_Points[3][1],
                            radar_center_x + x + 2 + (int) epd_Points[3][0],
                            radar_center_y - y     + (int) epd_Points[3][1],
                            ST77XX_WHITE);
              } else {
                    // TBD
                }
                    }
            // tft.fillTriangle(radar_center_x + x + (int)epd_Points[0][0],
            //                     radar_center_y - y + (int)epd_Points[0][1],
            //                     radar_center_x + x + (int)epd_Points[1][0],
            //                     radar_center_y - y + (int)epd_Points[1][1],
            //                     radar_center_x + x + (int)epd_Points[2][0],
            //                     radar_center_y - y + (int)epd_Points[2][1],
            //                     color);
            break;
          default: //Glider, Hanglider, ULM, Balloon
            // Draw GA aircraft on radar
            tft.fillCircle(radar_center_x + x,
                            radar_center_y - y,
                            10, ST77XX_RED);
            tft.fillRect(radar_center_x + x - 9, radar_center_y - y - 1, 18, 3, ST77XX_BLACK);
            tft.fillCircle(radar_center_x + x,
                            radar_center_y - y + 2,
                            3, ST77XX_BLACK);
            break;
          }
            break;
        case 1:
        //break;
        case 2:
        //break;
        case 3:
            tft.fillTriangle(radar_center_x + x + scale * ((int)epd_Points[0][0]),
                                radar_center_y - y + scale * ((int)epd_Points[0][1]),
                                radar_center_x + x + scale * ((int)epd_Points[1][0]),
                                radar_center_y - y + scale * ((int)epd_Points[1][1]),
                                radar_center_x + x + scale * ((int)epd_Points[4][0]),
                                radar_center_y - y + scale * ((int)epd_Points[4][1]),
                                color);
            tft.fillTriangle(radar_center_x + x + scale * ((int)epd_Points[2][0]),
                                radar_center_y - y + scale * ((int)epd_Points[2][1]),
                                radar_center_x + x + scale * ((int)epd_Points[1][0]),
                                radar_center_y - y + scale * ((int)epd_Points[1][1]),
                                radar_center_x + x + scale * ((int)epd_Points[4][0]),
                                radar_center_y - y + scale * ((int)epd_Points[4][1]),
                                color);
            break;
        default:
            break;
        }

      }
    }
    yield();
    Serial.print("EPD_Draw_Radar: Draw Radar circles");
    // draw range circles
    tft.drawCircle(radar_center_x, radar_center_y, radius - 1,   RADAR_CIRCLES_COLOR);
    tft.drawCircle(radar_center_x, radar_center_y, (radius / 2) - 1, RADAR_CIRCLES_COLOR);

        //draw distance marker as numbers on radar circles diaganolly from center to bottom right
    uint16_t circle_mark1_x = radar_center_x + abs(radius/2 * 0.7);
    uint16_t circle_mark1_y = radar_center_y + abs(radius/2 * 0.7);

    uint16_t circle_mark2_x = radar_center_x + abs(radius * 0.7);
    uint16_t circle_mark2_y = radar_center_y + abs(radius * 0.7);
    uint16_t scale = (navbox3.value == ZOOM_LOWEST ? 10 :
                     navbox3.value == ZOOM_LOW    ? 5 :
                     navbox3.value == ZOOM_MEDIUM ? 2 :
                     navbox3.value == ZOOM_HIGH   ? 1 : 1);
    tft.setCursor(circle_mark1_x, circle_mark1_y);
    //draw black rectangles as background
    tft.fillRect(circle_mark1_x - 10, circle_mark1_y - 10, 20, 20, ST77XX_BLACK);
    tft.setTextSize(2);
    // divide scale by 2 , if resul hs a decimal point, print only point and the decimal
    if (scale < 2) {
      tft.print(".5");
    } else {
      tft.print(scale / 2);
    }
    tft.setCursor(circle_mark2_x, circle_mark2_y);
    tft.fillRect(circle_mark2_x - 10, circle_mark2_y - 10, 20, 20, ST77XX_BLACK);
    tft.print(scale);
    Serial.print("EPD_Draw_Radar: Draw Radar circles done");
    Serial.println("EPD_Draw_Radar: Draw Airplane");
    delay(1000);

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
    tft.drawLine(radar_center_x + (int) epd_Points[0][0],
                radar_center_y + (int) epd_Points[0][1],
                radar_center_x + (int) epd_Points[1][0],
                radar_center_y + (int) epd_Points[1][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[2][0],
                radar_center_y + (int) epd_Points[2][1],
                radar_center_x + (int) epd_Points[3][0],
                radar_center_y + (int) epd_Points[3][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[4][0],
                radar_center_y + (int) epd_Points[4][1],
                radar_center_x + (int) epd_Points[5][0],
                radar_center_y + (int) epd_Points[5][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[6][0],
                radar_center_y + (int) epd_Points[6][1],
                radar_center_x + (int) epd_Points[7][0],
                radar_center_y + (int) epd_Points[7][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[8][0],
                radar_center_y + (int) epd_Points[8][1],
                radar_center_x + (int) epd_Points[9][0],
                radar_center_y + (int) epd_Points[9][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[10][0],
                radar_center_y + (int) epd_Points[10][1],
                radar_center_x + (int) epd_Points[11][0],
                radar_center_y + (int) epd_Points[11][1],
                ST77XX_WHITE);
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
    tft.drawLine(radar_center_x + (int) epd_Points[0][0],
                radar_center_y + (int) epd_Points[0][1],
                radar_center_x + (int) epd_Points[1][0],
                radar_center_y + (int) epd_Points[1][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[1][0],
                radar_center_y + (int) epd_Points[1][1],
                radar_center_x + (int) epd_Points[2][0],
                radar_center_y + (int) epd_Points[2][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[2][0],
                radar_center_y + (int) epd_Points[2][1],
                radar_center_x + (int) epd_Points[3][0],
                radar_center_y + (int) epd_Points[3][1],
                ST77XX_WHITE);
    tft.drawLine(radar_center_x + (int) epd_Points[3][0],
                radar_center_y + (int) epd_Points[3][1],
                radar_center_x + (int) epd_Points[0][0],
                radar_center_y + (int) epd_Points[0][1],
                ST77XX_WHITE);
#endif //ICON_AIRPLANE

  Serial.println("EPD_Draw_Radar: Orientation symbols");

    switch (settings->orientation)
    {
    case DIRECTION_NORTH_UP:
        // draw W, E, N, S
        x = radar_x + radar_w / 2 - radius + tbw / 2;
        y = radar_y + (radar_w + tbh) / 2;
        tft.setCursor(x, y);
        tft.print("W");
        x = radar_x + radar_w / 2 + radius - (3 * tbw) / 2;
        y = radar_y + (radar_w + tbh) / 2;
        tft.setCursor(x, y);
        tft.print("E");
        x = radar_x + (radar_w - tbw) / 2;
        y = radar_y + radar_w / 2 - radius + (3 * tbh) / 2;
        tft.setCursor(x, y);
        tft.print("N");
        x = radar_x + (radar_w - tbw) / 2;
        y = radar_y + radar_w / 2 + radius - tbh / 2;
        tft.setCursor(x, y);
        tft.print("S");
        break;
    case DIRECTION_TRACK_UP:
        // draw L, R, B
        x = radar_x + radar_w / 2 - radius + tbw / 2;
        y = radar_y + (radar_w + tbh) / 2;
        tft.setTextSize(1);
        tft.setCursor(x, y);
        tft.print("L");
        x = radar_x + radar_w / 2 + radius - (3 * tbw) / 2;
        y = radar_y + (radar_w + tbh) / 2;
        tft.setCursor(x, y);
        tft.print("R");
        // x = radar_x + (radar_w - tbw) / 2;
        // y = radar_y + radar_w / 2 + radius - tbh / 2;
        // tft.setCursor(x, y);
        // tft.print("B");
        Serial.print("EPD_Draw_Radar: Draw Aircraft Heading");
        // draw aircraft heading
        tft.setTextColor(NAVBOX_TEXT_COLOR);
        tft.setTextSize(2);
        snprintf(cog_text, sizeof(cog_text), "%03d", ThisAircraft.Track);
        tft.getTextBounds(cog_text, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = radar_x + (radar_w - tbw) / 2 - 5;
        y = radar_y + radar_w / 2 - radius + (3 * tbh) / 2 - 16;
        tft.setCursor(x, y);
        tft.print(cog_text);
        tft.drawRoundRect(x - 2, y - tbh - 2, tbw + 8, tbh + 6, 4, NAVBOX_FRAME_COLOR2);
        break;
    default:
  /* TBD */
    break;
        }
    }

    Serial.println("EPD_Draw_Radar: Draw Radar done");
    // Serial.println("EPD_Draw_Radar: drawRGBBitmap");
    // tft.drawRGBBitmap(0,40, canvas_radar.getBuffer(),canvas_radar.width(), canvas_radar.height());
}



void TFT_radar_setup()
{
  EPD_zoom = settings->zoom;

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
}

void TFT_radar_loop()
{
  if (isTimeToDisplay()) {
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());


    bool hasData = settings->protocol == PROTOCOL_NMEA  ? NMEA_isConnected()  :
                   settings->protocol == PROTOCOL_GDL90 ? GDL90_isConnected() :
                   false;

    if (hasData) {

      bool hasFix = settings->protocol == PROTOCOL_NMEA  ? isValidGNSSFix()   :
                    settings->protocol == PROTOCOL_GDL90 ? GDL90_hasOwnShip() :
                    false;

      if (hasFix) {
        EPD_Draw_Radar();
      } else {
        TFT_radar_Draw_Message(NO_FIX_TEXT, NULL);
      }
    } else {
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

    EPD_Draw_NavBoxes();
    yield();

    EPDTimeMarker = millis();
  }
}

void TFT_radar_zoom()
{
  if (EPD_zoom < ZOOM_HIGH) EPD_zoom++;
}

void TFT_radar_unzoom()
{
  if (EPD_zoom > ZOOM_LOWEST) EPD_zoom--;
}
#endif /* USE_TFT */