//////////////////////////////////
//      LED/DEVICE BRIDGE       //
//////////////////////////////////
#pragma once

#include <Arduino.h>
#include <math.h>

#include "200_LED_LINKER.h"

struct Mirror {
  bool onoff = false;
  int level = 50;
  float hue1 = 0.0f;
  float sat1 = 0.0f;
  float hue2 = 0.0f;
  float sat2 = 0.0f;
};

extern Mirror mirror;



namespace MAIN {

bool InitDeviceBridge();
void UpdateDeviceBridge();
void ApplyMirrorToCoreConfig();
void ApplyCoreConfigToMirror();
bool SetMirrorColorFromPixel(LED::SetTarget target, const LED::Pixel_byte& pix);
bool SetMirrorBrightness(uint8_t brightness);
bool SetMirrorOnOff(bool on);
void MirrorUpdated();
bool MirrorRgbwConversionEnabled();
void SetMirrorRgbwConversion(bool enabled);
bool ToggleMirrorRgbwConversion();

namespace detail {

Mirror SanitizeMirror(const Mirror& source);
LED::Pixel_float MirrorColorToPixel(float hueDeg, float satPercent);
struct HsvColor { float hue; float saturation; };
HsvColor PixelToHsv(const LED::Pixel_float& pix);
float MirrorLevelToBrightness(int level);
int BrightnessToMirrorLevel(float brightness);
float NormalizeHue(float hue);
bool& RgbwConversionEnabled();

}  // namespace detail

inline bool InitDeviceBridge() {
  ApplyCoreConfigToMirror();
  return true;
}

inline void UpdateDeviceBridge() {
  // Mirror changes are applied immediately by setters and device callbacks.
}

inline void ApplyMirrorToCoreConfig() {
  auto sanitized = detail::SanitizeMirror(mirror);
  mirror = sanitized;

  auto& cfg = LED::GetConfig();
  cfg.colorOneStaging = detail::MirrorColorToPixel(sanitized.hue1, sanitized.sat1);
  cfg.colorTwoStaging = detail::MirrorColorToPixel(sanitized.hue2, sanitized.sat2);
  cfg.brightnessStaging = detail::MirrorLevelToBrightness(sanitized.level);
  cfg.onoffStaging = sanitized.onoff ? 1.0f : 0.0f;

  LED::MarkChangeInConfig();
}

inline void ApplyCoreConfigToMirror() {
  const auto& cfg = LED::GetConfig();

  const auto hsvOne = detail::PixelToHsv(cfg.colorOneStaging);
  const auto hsvTwo = detail::PixelToHsv(cfg.colorTwoStaging);

  mirror.hue1 = hsvOne.hue;
  mirror.sat1 = hsvOne.saturation;
  mirror.hue2 = hsvTwo.hue;
  mirror.sat2 = hsvTwo.saturation;
  mirror.level = detail::BrightnessToMirrorLevel(cfg.brightnessStaging);
  mirror.onoff = cfg.onoffStaging >= 0.5f;

  auto sanitized = detail::SanitizeMirror(mirror);
  mirror = sanitized;
}

inline void MirrorUpdated() { ApplyMirrorToCoreConfig(); }

inline bool MirrorRgbwConversionEnabled() { return detail::RgbwConversionEnabled(); }

inline void SetMirrorRgbwConversion(bool enabled) {
  auto& mode = detail::RgbwConversionEnabled();
  if (mode == enabled) return;
  mode = enabled;
  MirrorUpdated();
}

inline bool ToggleMirrorRgbwConversion() {
  auto& mode = detail::RgbwConversionEnabled();
  const bool newState = !mode;
  SetMirrorRgbwConversion(newState);
  return detail::RgbwConversionEnabled();
}

inline bool SetMirrorColorFromPixel(LED::SetTarget target, const LED::Pixel_byte& pix) {
  if (target != LED::COLOR_ONE && target != LED::COLOR_TWO) return false;

  LED::Pixel_float floatPix;
  floatPix.R = static_cast<float>(pix.R);
  floatPix.G = static_cast<float>(pix.G);
  floatPix.B = static_cast<float>(pix.B);
  floatPix.W = static_cast<float>(pix.W);

  const auto hsv = detail::PixelToHsv(floatPix);

  if (target == LED::COLOR_ONE) {
    mirror.hue1 = hsv.hue;
    mirror.sat1 = hsv.saturation;
  } else {
    mirror.hue2 = hsv.hue;
    mirror.sat2 = hsv.saturation;
  }

  MirrorUpdated();

  return true;
}

inline bool SetMirrorBrightness(uint8_t brightness) {
  const int newLevel = detail::BrightnessToMirrorLevel(static_cast<float>(brightness));
  if (mirror.level == newLevel) return false;

  mirror.level = newLevel;
  MirrorUpdated();
  return true;
}

inline bool SetMirrorOnOff(bool on) {
  if (mirror.onoff == on) return false;

  mirror.onoff = on;
  MirrorUpdated();
  return true;
}

}  // namespace MAIN

namespace MAIN {
namespace detail {

inline bool& RgbwConversionEnabled() {
  static bool enabled = false;
  return enabled;
}

inline float NormalizeHue(float hue) {
  float normalized = fmodf(hue, 360.0f);
  if (normalized < 0.0f) normalized += 360.0f;
  return normalized;
}

inline float MirrorLevelToBrightness(int level) {
  const int clamped = constrain(level, 0, 100);
  return (static_cast<float>(clamped) / 100.0f) * 255.0f;
}

inline int BrightnessToMirrorLevel(float brightness) {
  const float clamped = constrain(brightness, 0.0f, 255.0f);
  return static_cast<int>(lroundf((clamped / 255.0f) * 100.0f));
}

inline Mirror SanitizeMirror(const Mirror& source) {
  Mirror sanitized = source;
  sanitized.level = constrain(sanitized.level, 0, 100);
  sanitized.hue1 = NormalizeHue(sanitized.hue1);
  sanitized.hue2 = NormalizeHue(sanitized.hue2);
  sanitized.sat1 = constrain(sanitized.sat1, 0.0f, 100.0f);
  sanitized.sat2 = constrain(sanitized.sat2, 0.0f, 100.0f);
  sanitized.onoff = source.onoff;
  return sanitized;
}

inline LED::Pixel_float MirrorColorToPixel(float hueDeg, float satPercent) {
  const float hue = NormalizeHue(hueDeg);
  const float saturation = constrain(satPercent, 0.0f, 100.0f) / 100.0f;
  const float value = 1.0f;
  const float c = value * saturation;
  const float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
  const float m = value - c;

  float r1 = 0.0f;
  float g1 = 0.0f;
  float b1 = 0.0f;

  if (hue < 60.0f) {
    r1 = c;
    g1 = x;
  } else if (hue < 120.0f) {
    r1 = x;
    g1 = c;
  } else if (hue < 180.0f) {
    g1 = c;
    b1 = x;
  } else if (hue < 240.0f) {
    g1 = x;
    b1 = c;
  } else if (hue < 300.0f) {
    r1 = x;
    b1 = c;
  } else {
    r1 = c;
    b1 = x;
  }

  const float finalR = r1 + m;
  const float finalG = g1 + m;
  const float finalB = b1 + m;

  LED::Pixel_float pix;
  if (RgbwConversionEnabled()) {
    const float whiteness = fminf(finalR, fminf(finalG, finalB));
    const float red = finalR - whiteness;
    const float green = finalG - whiteness;
    const float blue = finalB - whiteness;

    pix.R = constrain(red, 0.0f, 1.0f) * 255.0f;
    pix.G = constrain(green, 0.0f, 1.0f) * 255.0f;
    pix.B = constrain(blue, 0.0f, 1.0f) * 255.0f;
    pix.W = constrain(whiteness, 0.0f, 1.0f) * 255.0f;
  } else {
    pix.R = constrain(finalR, 0.0f, 1.0f) * 255.0f;
    pix.G = constrain(finalG, 0.0f, 1.0f) * 255.0f;
    pix.B = constrain(finalB, 0.0f, 1.0f) * 255.0f;
    pix.W = 0.0f;
  }

  return pix;
}

inline HsvColor PixelToHsv(const LED::Pixel_float& pix) {
  const float r = constrain(pix.R, 0.0f, 255.0f);
  const float g = constrain(pix.G, 0.0f, 255.0f);
  const float b = constrain(pix.B, 0.0f, 255.0f);
  const float w = constrain(pix.W, 0.0f, 255.0f);

  const float rMix = constrain(r + w, 0.0f, 255.0f);
  const float gMix = constrain(g + w, 0.0f, 255.0f);
  const float bMix = constrain(b + w, 0.0f, 255.0f);

  const float rNorm = rMix / 255.0f;
  const float gNorm = gMix / 255.0f;
  const float bNorm = bMix / 255.0f;

  const float maxVal = max(rNorm, max(gNorm, bNorm));
  const float minVal = min(rNorm, min(gNorm, bNorm));
  const float delta = maxVal - minVal;

  HsvColor out;
  out.hue = 0.0f;

  if (delta > 1e-5f) {
    if (maxVal == rNorm) {
      out.hue = 60.0f * fmodf(((gNorm - bNorm) / delta), 6.0f);
    } else if (maxVal == gNorm) {
      out.hue = 60.0f * (((bNorm - rNorm) / delta) + 2.0f);
    } else {
      out.hue = 60.0f * (((rNorm - gNorm) / delta) + 4.0f);
    }
  }

  if (out.hue < 0.0f) out.hue += 360.0f;

  const float saturation = (maxVal <= 1e-5f) ? 0.0f : (delta / maxVal);
  out.saturation = saturation * 100.0f;

  return out;
}

}  // namespace detail
}  // namespace MAIN

