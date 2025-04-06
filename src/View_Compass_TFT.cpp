#include "SkyView.h"
#include "TFTHelper.h"
#include "NMEAHelper.h"
#include "TrafficHelper.h"
#include "Compas450x450.h"
#include "SatDish.h"
#include "Speed.h"
#include "altitude2.h"

extern TFT_eSPI tft;
extern LilyGo_Class amoled;
extern TFT_eSprite sprite;
TFT_eSprite compasSprite = TFT_eSprite(&tft);
TFT_eSprite compas2Sprite = TFT_eSprite(&tft);
    

void TFT_compass_loop() {
    if (isTimeToDisplay()) {
        compasSprite.createSprite(450, 450);
        compas2Sprite.createSprite(450, 450);
        compasSprite.fillSprite(TFT_BLACK);
        sprite.fillSprite(TFT_BLACK);
        compasSprite.setSwapBytes(true);
        compasSprite.pushImage(0, 0, 450, 450, Compas450x450);
        compasSprite.setPivot(225, 225);
        sprite.setPivot(225, 225);
        compas2Sprite.fillSprite(TFT_BLACK);
        
        compas2Sprite.fillTriangle(215, 125, 235, 90, 255, 125, TFT_WHITE);
        compas2Sprite.setCursor(220, 230, 4);
        compas2Sprite.printf("%d kmh", (int)((ThisAircraft.GroundSpeed) * 1.85));
        compas2Sprite.setSwapBytes(true);
        compas2Sprite.pushImage(180, 230, 32, 24, Speed);
        compas2Sprite.setSwapBytes(true);
        compas2Sprite.pushImage(135, 280, 32, 32, SatDishpng);
        compas2Sprite.setCursor(180, 290, 4);
        if (nmea.satellites.isValid()) {
            int satelliteCount = nmea.satellites.value();
            compas2Sprite.printf("%d", satelliteCount);
        }
        else {
            compas2Sprite.setTextColor(TFT_RED, TFT_BLACK);
            compas2Sprite.printf("NO FIX");
            compas2Sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        compas2Sprite.setSwapBytes(true);
        compas2Sprite.pushImage(265, 280, 32, 32, altitude2);
        compas2Sprite.setCursor(310, 290, 4);
        compas2Sprite.printf("%d ft", (int)(ThisAircraft.altitude * 3.28084));
        compasSprite.pushRotated(&sprite, 360 - ThisAircraft.Track, TFT_BLACK);
        compas2Sprite.setCursor(190, 160, 7);
        compas2Sprite.printf("%d", ThisAircraft.Track, TFT_BLACK);
        compas2Sprite.pushToSprite(&sprite, 0, 0, TFT_BLACK);
        amoled.pushColors(0, 0, 450, 450, (uint16_t*)sprite.getPointer());
}

}