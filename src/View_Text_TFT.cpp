#include "TFTHelper.h"

#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

#include <TimeLib.h>

#include "TrafficHelper.h"
#include "EEPROMHelper.h"
#include "NMEAHelper.h"
#include "GDL90Helper.h"

#include "SkyView.h"

static int TFT_current = 1;

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