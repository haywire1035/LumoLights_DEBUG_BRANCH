//////////////////////////////////
//   LINKER CODE FOR LED CORE   //
//////////////////////////////////
/**
 * @file LedLinker.h
 * @brief Hardware linker for LED module using Adafruit_NeoPixel.
 *
 * Exposes the public LED:: API used by the application.
 */

#pragma once

////////// Header Files //////////
#include "050_HAL.h"

#ifndef LED_COUNT
#define LED_COUNT HAL::kLedCount
#endif

#include "210_LED_CORE.h"



namespace LED {

using PixelByte = CORE::Pixel_byte;
using PixelFloat = CORE::Pixel_float;
using Pixel_byte = CORE::Pixel_byte;
using Pixel_float = CORE::Pixel_float;
using Config = CORE::Config;
using Vars = CORE::Vars;
using State = CORE::State;
using GradientMode = CORE::GradientMode;
using InterpolationMode = CORE::InterpolationMode;

inline constexpr GradientMode LINEAR = static_cast<GradientMode>(CORE::LINEAR);
inline constexpr GradientMode LINEAR_PADDING = static_cast<GradientMode>(CORE::LINEAR_PADDING);
inline constexpr GradientMode SINGLE_COLOR = static_cast<GradientMode>(CORE::SINGLE_COLOR);
inline constexpr GradientMode MIDPOINT_SPLIT = static_cast<GradientMode>(CORE::MIDPOINT_SPLIT);
inline constexpr GradientMode EDGE_CENTER = static_cast<GradientMode>(CORE::EDGE_CENTER);

inline Config& GetConfig();
inline Vars& GetVars();
inline State& GetState();
inline void MarkChangeInConfig();
inline void ProvokeImmediateSaveOfConfig();

/**
     * @brief Initialize LED hardware and core state.
     */
inline bool Init();


/**
     * @brief Apply logical brightness to hardware and show LEDs.
     */
inline void Update();


inline void UpdateColor();


/**
     * @brief Set brightness (0–255).
     */
inline void SetBrightness(uint8_t brightness);


enum SetTarget {
  BRIGHTNESS,
  COLOR_ONE,
  COLOR_TWO
};
bool Set(SetTarget target, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
bool Set(SetTarget target, const PixelByte& pix);
bool Set(SetTarget target, uint8_t value);

inline void Clear();
}

/* -------------------------------------------------------------------------- */
/* Implementation                                                             */
/* -------------------------------------------------------------------------- */

inline LED::Config& LED::GetConfig() { return CORE::GetConfig(); }

inline LED::Vars& LED::GetVars() { return CORE::GetVars(); }

inline LED::State& LED::GetState() { return CORE::GetState(); }

inline void LED::MarkChangeInConfig() { CORE::MarkChangeInConfig(); }

inline void LED::ProvokeImmediateSaveOfConfig() {
  CORE::ProvokeImmediateSaveOfConfig();
}

inline bool LED::Init() {
  if (!CORE::Init()) return false;
  if (!HAL::InitLedHardware()) return false;
  HAL::ClearLedHardware();
  HAL::ShowLedHardware();
  return true;
}


/**
 * @brief Update LED logic and push final RGBW values to hardware.
 *
 * Sequence:
 *  1. Compute gradient or pattern into CORE::Vars::Colors[]
 *  2. Apply per-pixel scaling and logical brightness → CORE::Vars::Pixels[]
 *  3. Write Pixels[] to hardware strip via UpdateColor()
 */
inline void LED::Update() {
  //auto& state = CORE::GetState();
  //if (!state.active) return;  // skip if inactive

  auto& s = CORE::GetState();
  auto& c = CORE::GetConfig();

  if ((millis() - s.processingLastExecutionMs) > c.processingIntervalMs) {

    // --- Step 1: Update timing metadata ---
    s.processingLastExecutionMs = millis();

    // --- Step 2: Update timing metadata ---
    CORE::Fade();
    
    // --- Step 3: Compute color distribution (e.g., gradient) ---
    CORE::ComputeGradient(c.gradientMode, c.gradientInvertColors);

    // --- Step 4: Apply scaling and brightness ---
    CORE::ApplyOutputScaling();

    // --- Step 5: Push to physical LEDs ---
    UpdateColor();
  }

  

  if ((millis() - s.effectLastExecutionMs) > c.effectIntervalMs) {

    // --- Step 1: Update timing metadata ---
    s.effectLastExecutionMs = millis();

    // --- Step 2: Update effect Function ---
    CORE::Effect();
  }

}



/**
 * @brief Write final pixel values (CORE::Vars::Pixels) to the physical strip.
 *
 * This function performs *only* the hardware write. It assumes CORE::Pixels
 * already contain final uint8_t values (0..255) and does not modify CORE state.
 */
inline void LED::UpdateColor() {
  auto& v = CORE::GetVars();
  const size_t count = v.Count;
  if (count == 0) return;

  for (size_t i = 0; i < count; ++i) {
    const auto& p = v.Pixels[i];
    //HAL::SetPixelColor(static_cast<uint16_t>(i), p.R, p.G, p.B, p.W);
  }

  //HAL::ShowLedHardware();
}



inline void LED::SetBrightness(uint8_t brightness) {
  CORE::SetStagingBrightness(brightness);
}



/**
 * @brief Set a target color (ColorOne / ColorTwo).
 *
 * @param target Which color to set (ColorOne or ColorTwo).
 * @param r,g,b,w Channel values (0..255).
 * @param autoApply If false (default) write into the Transfer field (used for fades).
 *                   If false, write directly to the active ColorOne/ColorTwo.
 * @return true on success.
 */
inline bool LED::Set(SetTarget target, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  auto& c = CORE::GetConfig();

  // build pixel locally
  CORE::Pixel_float pix;
  pix.R = (float)r;
  pix.G = (float)g;
  pix.B = (float)b;
  pix.W = (float)w;

  bool applied = false;

  if (target == COLOR_ONE) {
    c.colorOneStaging = pix;
    applied = true;
  } else if (target == COLOR_TWO) {
    c.colorTwoStaging = pix;
    applied = true;
  }

  if (applied) {
    CORE::MarkChangeInConfig();
    return true;
  }

  return false;
}

/**
 * @brief Overload: set using a CORE::Pixel.
 */
inline bool LED::Set(LED::SetTarget target, const PixelByte& pix) {
  return Set(target, pix.R, pix.G, pix.B, pix.W);
}

inline bool LED::Set(SetTarget target, uint8_t value) {
  if (target != LED::BRIGHTNESS) return false;

  // apply immediately to logical brightness
  CORE::SetStagingBrightness(value);
  CORE::MarkChangeInConfig();

  return true;
}



inline void LED::Clear() {
  CORE::Clear();
  HAL::ClearLedHardware();
  HAL::ShowLedHardware();
}
