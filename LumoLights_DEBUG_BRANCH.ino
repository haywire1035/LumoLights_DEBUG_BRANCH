//////////////////////////////////
//         LUMO LIGHTS          //
//////////////////////////////////
//
//         Lucas Grodd
//             2025
//
//    Apple HomeKit controlled
//          Smartlight
//
//////////////////////////////////
//         MAIN SKETCH          //
//////////////////////////////////

#define ENABLE_COMMAND_LINE_INTERFACE true
#define DEBUG_SERIAL true

// defines for device identification
#define SKETCH_VERSION "V01.03.12"
#define CONFIG_VERSION "V01.09"



/////////// Libraries  ///////////
#include "HomeSpan.h"
#include "SPI.h"
//#include <Arduino.h>




////////// Header Files //////////
#include "050_HAL.h"
#include "100_DEVICE_LINKER.h"
#include "200_LED_LINKER.h"
//#include "220_GAMMA_TABLES.h"

#include "300_CONSOLE.h"
#include "400_SETTINGS.h"
#include "110_DEVICE.h"


Mirror mirror;





void setup() {

  if (ENABLE_COMMAND_LINE_INTERFACE || DEBUG_SERIAL) {
    Serial.begin(115200);
    delay(2000);
  }

  if (DEBUG_SERIAL) {
    Serial.println("");
    Serial.println("");
    Serial.println("");
    Serial.println("+---------------------+");
    Serial.println("  ###  LumoLight  ###  ");
    Serial.println("");

    Serial.print("   Version ");
    Serial.println(SKETCH_VERSION);

    Serial.print("   Hardware Profile: ");
    Serial.println(HAL::GetHardwareConfigLabel());

    Serial.print("   No. of LED: ");
    Serial.println(LED_COUNT);

    if (ENABLE_COMMAND_LINE_INTERFACE) {
      Serial.print("   CLI:   ");
      Serial.println("enabled");
    } else {
      Serial.print("   CLI: ");
      Serial.println("disabled");
    }

    Serial.print("   DEBUG: ");
    Serial.println("enabled");

    Serial.println(" ");
    Serial.println(" Lucas Grodd 2025");
    Serial.println("+--------------------+");
    Serial.println(" ");
  }

  if (ENABLE_COMMAND_LINE_INTERFACE) {
    CONSOLE::InitializeConsoleInterface();
  }



  if (!LED::Init()) {
    // handle error
  }

  SETTINGS::InitAndLoadReport();

  MAIN::InitDeviceBridge();

  // Whatever the config says.. turn the lamp on when power cycling
  MAIN::SetMirrorOnOff(true);

  LED::Update();

  DEVICE::InitializeDevice();
}


//////////////////////////////////////

void loop() {

  homeSpan.poll();

  LED::Update();

  MAIN::UpdateDeviceBridge();

  SETTINGS::Update();

  CONSOLE::Process();

}
