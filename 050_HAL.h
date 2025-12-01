//////////////////////////////////
//  HARDWARE ABSTRACTION LAYER  //
//////////////////////////////////
/**
 * @file 050_HAL.h
 * @brief Minimal hardware configurations for the LED linker.
 */

#pragma once



// Select default configuration if nothing else is specified.
#if !defined(HAL_CONFIG_SINGLE_WS2812) && !defined(HAL_CONFIG_DUAL_WS2812) && \
    !defined(HAL_CONFIG_SINGLE_WS2801) && !defined(HAL_CONFIG_DUAL_WS2801)
#define HAL_CONFIG_SINGLE_WS2801
#endif

#if defined(HAL_CONFIG_SINGLE_WS2812) || defined(HAL_CONFIG_DUAL_WS2812)
#include <Adafruit_NeoPixel.h>
#elif defined(HAL_CONFIG_SINGLE_WS2801) || defined(HAL_CONFIG_DUAL_WS2801)

#endif

namespace HAL {

inline bool InitLedHardware();
inline void ClearLedHardware();
inline void SetPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t w);
inline void ShowLedHardware();
inline uint16_t GetLogicalLedCount();
inline const char* GetHardwareConfigLabel();
inline uint8_t MixWhite(uint8_t color, uint8_t white);





#if defined(HAL_CONFIG_SINGLE_WS2812)

#ifndef HAL_SINGLE_WS2812_PIN
#define HAL_SINGLE_WS2812_PIN 3
#endif

#ifndef HAL_SINGLE_WS2812_LED_COUNT
#define HAL_SINGLE_WS2812_LED_COUNT 69
#endif

#ifndef HAL_SINGLE_WS2812_PIXEL_TYPE
#define HAL_SINGLE_WS2812_PIXEL_TYPE (NEO_RGBW + NEO_KHZ800)
#endif

inline constexpr uint8_t kDataPin = HAL_SINGLE_WS2812_PIN;
inline constexpr uint16_t kLedCount = HAL_SINGLE_WS2812_LED_COUNT;

inline Adafruit_NeoPixel g_strip(kLedCount, kDataPin, HAL_SINGLE_WS2812_PIXEL_TYPE);

inline bool InitLedHardware() {
  g_strip.begin();
  g_strip.clear();
  g_strip.setBrightness(255);
  return true;
}

inline void ClearLedHardware() { g_strip.clear(); }

inline void SetPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if (index >= kLedCount) return;
  g_strip.setPixelColor(index, g_strip.Color(g, r, b, w));
}

inline void ShowLedHardware() { g_strip.show(); }

inline const char* GetHardwareConfigLabel() { return "HAL_CONFIG_SINGLE_WS2812"; }





#elif defined(HAL_CONFIG_DUAL_WS2812)

#ifndef HAL_DUAL_WS2812_PIN_ONE
#define HAL_DUAL_WS2812_PIN_ONE 3
#endif

#ifndef HAL_DUAL_WS2812_PIN_TWO
#define HAL_DUAL_WS2812_PIN_TWO 4
#endif

#ifndef HAL_DUAL_WS2812_COUNT_ONE
#define HAL_DUAL_WS2812_COUNT_ONE 69
#endif

#ifndef HAL_DUAL_WS2812_COUNT_TWO
#define HAL_DUAL_WS2812_COUNT_TWO 69
#endif

#ifndef HAL_DUAL_WS2812_PIXEL_TYPE
#define HAL_DUAL_WS2812_PIXEL_TYPE (NEO_RGBW + NEO_KHZ800)
#endif

inline constexpr uint16_t kStripOneCount = HAL_DUAL_WS2812_COUNT_ONE;
inline constexpr uint16_t kStripTwoCount = HAL_DUAL_WS2812_COUNT_TWO;
inline constexpr uint16_t kLedCount = kStripOneCount + kStripTwoCount;

inline Adafruit_NeoPixel g_stripOne(kStripOneCount, HAL_DUAL_WS2812_PIN_ONE,
                                    HAL_DUAL_WS2812_PIXEL_TYPE);
inline Adafruit_NeoPixel g_stripTwo(kStripTwoCount, HAL_DUAL_WS2812_PIN_TWO,
                                    HAL_DUAL_WS2812_PIXEL_TYPE);

inline bool InitLedHardware() {
  g_stripOne.begin();
  g_stripTwo.begin();
  ClearLedHardware();
  g_stripOne.setBrightness(255);
  g_stripTwo.setBrightness(255);
  return true;
}

inline void ClearLedHardware() {
  g_stripOne.clear();
  g_stripTwo.clear();
}

inline void SetPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t w) {
  if (index < kStripOneCount) {
    g_stripOne.setPixelColor(index, g_stripOne.Color(g, r, b, w));
    return;
  }

  const uint16_t localIndex = static_cast<uint16_t>(index - kStripOneCount);
  if (localIndex < kStripTwoCount) {
    g_stripTwo.setPixelColor(localIndex, g_stripTwo.Color(g, r, b, w));
  }
}

inline void ShowLedHardware() {
  g_stripOne.show();
  g_stripTwo.show();
}


inline const char* GetHardwareConfigLabel() {
  return "HAL_CONFIG_DUAL_WS2812";
}





#elif defined(HAL_CONFIG_SINGLE_WS2801)

#ifndef HAL_SINGLE_WS2801_DATA_PIN
#define HAL_SINGLE_WS2801_DATA_PIN 15
#endif

#ifndef HAL_SINGLE_WS2801_CLOCK_PIN
#define HAL_SINGLE_WS2801_CLOCK_PIN 14
#endif

#ifndef HAL_SINGLE_WS2801_LED_COUNT
#define HAL_SINGLE_WS2801_LED_COUNT 31
#endif

inline constexpr uint16_t kLedCount = HAL_SINGLE_WS2801_LED_COUNT;

inline bool InitLedHardware() {

  return true;
}

inline void ClearLedHardware() {

}

inline void SetPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t w) {
  if (index >= kLedCount) return;

}

inline void ShowLedHardware() { }

inline const char* GetHardwareConfigLabel() {
  return "HAL_CONFIG_SINGLE_WS2801";
}


/*


#elif defined(HAL_CONFIG_DUAL_WS2801)

#ifndef HAL_DUAL_WS2801_DATA_PIN_ONE
#define HAL_DUAL_WS2801_DATA_PIN_ONE 15
#endif

#ifndef HAL_DUAL_WS2801_CLOCK_PIN_ONE
#define HAL_DUAL_WS2801_CLOCK_PIN_ONE 14
#endif

#ifndef HAL_DUAL_WS2801_DATA_PIN_TWO
#define HAL_DUAL_WS2801_DATA_PIN_TWO 21
#endif

#ifndef HAL_DUAL_WS2801_CLOCK_PIN_TWO
#define HAL_DUAL_WS2801_CLOCK_PIN_TWO 20
#endif

#ifndef HAL_DUAL_WS2801_COUNT_ONE
#define HAL_DUAL_WS2801_COUNT_ONE 10
#endif

#ifndef HAL_DUAL_WS2801_COUNT_TWO
#define HAL_DUAL_WS2801_COUNT_TWO 10
#endif

#ifndef HAL_DUAL_WS2801_BUTTON_PIN
#define HAL_DUAL_WS2801_BUTTON_PIN 19
#endif

inline constexpr uint16_t kStripOneCount = HAL_DUAL_WS2801_COUNT_ONE;
inline constexpr uint16_t kStripTwoCount = HAL_DUAL_WS2801_COUNT_TWO;
inline constexpr uint16_t kLedCount = kStripOneCount + kStripTwoCount;

//inline Adafruit_WS2801 g_stripOne(kStripOneCount, HAL_DUAL_WS2801_DATA_PIN_ONE,
//                                  HAL_DUAL_WS2801_CLOCK_PIN_ONE);
//inline Adafruit_WS2801 g_stripTwo(kStripTwoCount, HAL_DUAL_WS2801_DATA_PIN_TWO,
//                                  HAL_DUAL_WS2801_CLOCK_PIN_TWO);

inline bool InitLedHardware() {
  g_stripOne.begin();
  g_stripTwo.begin();
  ClearLedHardware();
  return true;
}

inline void ClearLedHardware() {
  for (uint16_t i = 0; i < kStripOneCount; ++i) {
    g_stripOne.setPixelColor(i, 0);
  }
  for (uint16_t i = 0; i < kStripTwoCount; ++i) {
    g_stripTwo.setPixelColor(i, 0);
  }
  g_stripOne.show();
  g_stripTwo.show();
}

inline void SetPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  const uint8_t rr = MixWhite(r, w);
  const uint8_t gg = MixWhite(g, w);
  const uint8_t bb = MixWhite(b, w);

  const bool isEven = (index % 2 == 0);

  if (!isEven) {
    const uint16_t i = index / 2;
    if (i < kStripOneCount) {
      g_stripOne.setPixelColor(i, rr, gg, bb);
    }
  } else {
    const uint16_t i = index / 2;
    if (i < kStripTwoCount) {
      g_stripTwo.setPixelColor(i, rr, gg, bb);
    }
  }
}


inline void ShowLedHardware() {
  g_stripOne.show();
  g_stripTwo.show();
}

inline const char* GetHardwareConfigLabel() {
  return "HAL_CONFIG_DUAL_WS2801";
}

*/



#else
#error \
    "Select a HAL configuration (HAL_CONFIG_SINGLE_WS2812, HAL_CONFIG_DUAL_WS2812, HAL_CONFIG_SINGLE_WS2801, or HAL_CONFIG_DUAL_WS2801)."
#endif

inline uint8_t MixWhite(uint8_t color, uint8_t white) {
  const uint16_t sum = static_cast<uint16_t>(color) + static_cast<uint16_t>(white);
  return static_cast<uint8_t>(sum > 255 ? 255 : sum);
}

}  // namespace HAL
