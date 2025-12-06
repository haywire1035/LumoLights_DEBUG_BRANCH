#pragma once

//////////////////////////////////
//   DEVICE-SPECIFIC SERVICES   //
//////////////////////////////////
// implements Apple HomeKit-specific functions

#include "100_DEVICE_LINKER.h"

namespace DEVICE {


bool InitializeDevice();


////////////////////////////////////
// Identify Service Wrapper
////////////////////////////////////

struct DEV_Identify : Service::AccessoryInformation {

  int nBlinks;                   // number of times to blink built-in LED in identify routine
  SpanCharacteristic *identify;  // reference to the Identify Characteristic

  DEV_Identify(const char *name,
               const char *manu,
               const char *sn,
               const char *model,
               const char *version,
               int nBlinks)
    : Service::AccessoryInformation() {

    new Characteristic::Name(name);
    new Characteristic::Manufacturer(manu);
    new Characteristic::SerialNumber(sn);
    new Characteristic::Model(model);
    new Characteristic::FirmwareRevision(version);
    identify = new Characteristic::Identify();

    this->nBlinks = nBlinks;

    pinMode(homeSpan.getStatusPin(), OUTPUT);
  }

  boolean update() {

    for (int i = 0; i < nBlinks; i++) {
      digitalWrite(homeSpan.getStatusPin(), LOW);
      delay(250);
      digitalWrite(homeSpan.getStatusPin(), HIGH);
      delay(250);
    }

    return true;
  }
};

////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////


// Custom Struct for Color1 Light
////////////////////////////////////
struct DEV_Color1_Light : Service::LightBulb {

  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};

  WS2801_LED* pixel;
  int nPixels;

  uint32_t timer_RGB = 0;

  WS2801_LED::Color* colors;

DEV_Color1_Light(uint8_t dataPin, uint8_t clockPin, int count)
  : Service::LightBulb(), nPixels(count) {

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new WS2801_LED(dataPin,clockPin);          // creates Dot LED on specified pins
    colors = new WS2801_LED::Color[nPixels];

    //this->nPixels=nPixels;                    // save number of Pixels in this LED Strand
    
    update();                                 // manually call update() to set pixel with restored initial values
    update();                                 // call second update() a second time - DotStar seems to need to be "refreshed" upon start-up
  }

  boolean update() override {

    int p = power.getNewVal();            // 0 or 1
    float h = H.getNewVal<float>();       // [0..360]
    float s = S.getNewVal<float>();       // [0..100]
    float v = V.getNewVal<float>();       // [0..100]

    WS2801_LED::Color c;
    c.HSV(h * p, s * p, v * p);           // power=0 → black

    for (int i = 0; i < nPixels; i++) {
      colors[i] = c;
    }

    pixel->set(colors, nPixels);

    mirror.onoff     = p;
    mirror.level     = v;
    mirror.hue1      = h;
    mirror.sat1      = s;

    MAIN::MirrorUpdated();

    return true;
  }

  void loop() override {
    if (millis() - timer_RGB < 50)
      return;

    timer_RGB = millis();

    // Example effect: faint blue pulse (only for demonstration)
    static uint8_t pulse = 0;
    pulse = (pulse + 1) % 255;

    for (int i = 0; i < nPixels; i++) {
      colors[i].RGB(0, 0, pulse);
    }

    pixel->set(colors, nPixels);
  }
};





/*

  SpanCharacteristic *power;
  SpanCharacteristic *level;
  SpanCharacteristic *hue;
  SpanCharacteristic *sat;

  long timer_RGB = 0;

  DEV_Color1_Light()
    : Service::LightBulb() {

    // Name for this light in HomeKit
    new Characteristic::Name("Color 1");

    power = new Characteristic::On(0);

    level = new Characteristic::Brightness(50);
    level->setRange(5, 100, 1);

    hue = new Characteristic::Hue();
    hue->setRange(0, 360, 1);

    sat = new Characteristic::Saturation();
    sat->setRange(0, 100, 1);

    if (DEBUG_SERIAL) Serial.println("Configuring RGB Light (Color 1)");
  }

  void loop() {
    if (millis() - timer_RGB > 500) {
      timer_RGB = millis();

      power->setVal(mirror.onoff);
      level->setVal(mirror.level);
      hue->setVal(mirror.hue1);
      sat->setVal(mirror.sat1);
    }
  }


};*/

////////////////////////////////////
// Custom Struct for Color2 Light //
////////////////////////////////////
struct DEV_Color2_Light : Service::LightBulb {

  SpanCharacteristic *power;
  SpanCharacteristic *level;
  SpanCharacteristic *hue;
  SpanCharacteristic *sat;

  long timer_RGB = 0;

  DEV_Color2_Light()
    : Service::LightBulb() {

    // Name for this light in HomeKit
    new Characteristic::Name("Color 2");

    power = new Characteristic::On(0);

    level = new Characteristic::Brightness(50);
    level->setRange(5, 100, 1);

    hue = new Characteristic::Hue();
    hue->setRange(0, 360, 1);

    sat = new Characteristic::Saturation();
    sat->setRange(0, 100, 1);

    if (DEBUG_SERIAL) Serial.println("Configuring RGB Light (Color 2)");
  }

  void loop() {
    if (millis() - timer_RGB > 500) {
      timer_RGB = millis();

      power->setVal(mirror.onoff);
      level->setVal(mirror.level);
      hue->setVal(mirror.hue2);
      sat->setVal(mirror.sat2);
    }
  }

  boolean update() {

    //if (DEBUG_SERIAL) Serial.println("Update on RGB values (Color 2).");

    mirror.onoff     = power->getNewVal();
    mirror.level     = level->getNewVal();
    mirror.hue2      = hue->getNewVal();
    mirror.sat2      = sat->getNewVal();

    MAIN::MirrorUpdated();

    return true;
  }
};

////////////////////////////////////
// Initialization Helper
////////////////////////////////////

bool InitializeDevice() {

  homeSpan.setSketchVersion(SKETCH_VERSION);

  homeSpan.begin(Category::Lighting, "LumoLight_Tube");

  // Single accessory representing the physical device
  new SpanAccessory();
  new DEV_Identify("TUBE",
                   "Lucas Grodd",
                   "TUBE_SN001",
                   "TUBE",
                   SKETCH_VERSION,
                   0);
  new Service::HAPProtocolInformation();
  new Characteristic::Version(SKETCH_VERSION);

  // Two LightBulb services on the same accessory for the two colors
  new DEV_Color1_Light(HAL_SINGLE_WS2801_DATA_PIN, HAL_SINGLE_WS2801_CLOCK_PIN, HAL_SINGLE_WS2801_LED_COUNT);
  new DEV_Color2_Light();

  return true;
}

}

