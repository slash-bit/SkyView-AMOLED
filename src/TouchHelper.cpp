#include <Arduino.h>
// #include "TouchDrvCST92xx.h"
#include <driver/touch/TouchDrvCSTXXX.hpp>
#include "pin_config.h"
#include "TouchHelper.h"
#include "TFTHelper.h"
#include "View_Radar_TFT.h"

// Create an instance of the CST9217 class

TouchDrvCST92xx touchSensor;

uint8_t touchAddress = 0x5A;


int16_t endX = -1, endY = -1;
static int16_t startX = -1, startY = -1;
static uint32_t startTime = 0;
int16_t currentX[5], currentY[5];

// Task Handle
TaskHandle_t touchTaskHandle = NULL;

bool IIC_Interrupt_Flag = false;

void Touch_setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);

  
    attachInterrupt(TP_INT, []()
    { IIC_Interrupt_Flag = true; }, FALLING);
  

  
        touchSensor.setPins(SENSOR_RST, SENSOR_IRQ);
  if (touchSensor.begin(Wire, touchAddress, IIC_SDA, IIC_SCL) == false)
  {
      Serial.println("CST9217 initialization failed");
  }
  else
  {
      Serial.print("Model :");
      Serial.println(touchSensor.getModelName());
      touchSensor.setMaxCoordinates(466, 466); // Set touch max xy
  }
  xTaskCreatePinnedToCore(touchTask, "Touch Task", 4096, NULL, 1, &touchTaskHandle, 1);
  }


void touchTask(void *parameter) {
    

    while(true) {
    
  
    if (IIC_Interrupt_Flag) {
    //   Serial.println("Interrupt triggered!");
        IIC_Interrupt_Flag = false; // Reset interrupt flag
      uint8_t points = touchSensor.getPoint(currentX, currentY, 1); // Read single touch point
  
      if (points > 0) {
        // Record the starting touch position and time
        if (startX == -1 && startY == -1) {
          startX = currentX[0];
          startY = currentY[0];
          startTime = millis();

        }
  
        // Continuously update the end position
        endX = currentX[0];
        endY = currentY[0];
      } else {
        // If no more points are detected, process swipe
        if (startX != -1 && startY != -1) {
          uint32_t duration = millis() - startTime;

  
          int16_t deltaX = endX - startX;
          int16_t deltaY = endY - startY;
  
          // Swipe detection logic
          if (duration < 500) { // Limit gesture duration
            if (abs(deltaX) > abs(deltaY)) { // Horizontal swipe
              if (deltaX > 30) {
                Serial.println("Swipe Left");
                TFT_Mode(true);
              } else if (deltaX < -30) {
                Serial.println("Swipe Right");
                TFT_Mode(false);
              }
            } else if (abs(deltaX) < abs(deltaY)) { // Vertical swipe
                if (deltaY > 50) {
                  Serial.println("Swipe Up - Radar Zoom Out");
                  TFT_Up();

                } else if (deltaY < -50) {
                  Serial.println("Swipe Down - Radar Zoom In");
                  TFT_Down();
                }
              } else if (abs(deltaX) < 50 && abs(deltaY) < 50) {
                Serial.println("Tap");
              }
  
            }
            else if (duration > 500 && duration < 2000 && abs(deltaX) < 50 && abs(deltaY) < 50) {
              Serial.println("Long Press");
            }
          // Reset variables for next swipe detection
          startX = startY = -1;
          startTime = 0;
          endX = endY = -1;
        }
      }
  
      
    }
  
    delay(50); // Polling delay
  }
}
  






