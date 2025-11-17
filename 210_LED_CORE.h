//////////////////////////////////
//       LED CORE CODE          //
//////////////////////////////////
#pragma once
#include <Arduino.h>

/**
 * @file LedCore.h
 * @brief Logical LED core for fixed pixel count systems (static allocation).
 *
 * Requirements:
 *  - Define LED_COUNT before including this header.
 *
 * Exposes:
 *  - LED::CORE::State
 *  - LED::CORE::Vars
 *  - LED::CORE::Init()
 *  - LED::CORE::SetBrightness()/GetBrightness()
 *  - LED::CORE::GetState()/GetVars()
 *  - LED::CORE::MapCubic100to255(int)
 */


#ifndef LED_COUNT
#error "LED_COUNT must be defined before including LedCore.h"
#endif



namespace LED {
namespace CORE {


bool SetPixelRGBW(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);

void MarkChangeInConfig();
void ProvokeImmediateSaveOfConfig();
void ShiftScaleChannel(float newValue, int8_t channel, bool forward);

/**
 * @brief Supported gradient interpolation modes.
 */
enum GradientMode {
  LINEAR = 0,
  LINEAR_PADDING = 1,
  SINGLE_COLOR = 2,    ///< Use a single color across the entire strip (respects inversion).
  MIDPOINT_SPLIT = 3,  ///< Hard switch at midpoint between primary and secondary color.
  EDGE_CENTER = 4,     ///< Primary color on both edges, secondary in the center.
};

enum class InterpolationMode : uint8_t {
  Linear = 0,
  Smooth = 1,
};


/**
 * @brief Per-pixel representation (AoS) of bytes
 *
 */
struct Pixel_byte {
  uint8_t R;
  uint8_t G;
  uint8_t B;
  uint8_t W;
};

/**
 * @brief Per-pixel representation (AoS) of floats
 *
 */
struct Pixel_float {
  float R;
  float G;
  float B;
  float W;
};


struct Effect_Container {
  float prev;
  float next;
  float currentOutput;
  uint32_t numSteps;
  uint32_t currentStep;
  bool hold;
};

/* --- Types --- */

struct State {
  bool active = true;
  uint32_t processingLastExecutionMs = 0;
  uint32_t effectLastExecutionMs = 0;
};


struct Config {

  Pixel_float colorOneStaging;
  Pixel_float colorTwoStaging;
  float brightnessStaging;
  float onoffStaging;

  float colorIncrement = 1.0;
  float brightnessIncrement = 1.0;
  float onoffIncrement = 0.01;

  uint32_t processingIntervalMs = 10;
  uint32_t effectIntervalMs = 10;

  // Gradient Mode LINEAR_PADDING
  // can be between 0.0 and 0.4 - will start linear blend (is mirrored for the other color)
  float gradientPaddingBegin = 0.1;
  // can be between 0.0 and 1.0 - will set the color amplitude in the padded area (note that for values <1.0, the other color is applied to a sum of 1.0)
  float gradientPaddingValue = 0.95;

  // Gradient Mode EDGE_CENTER
  // percentage (0.0 - 0.5) that stays at the edge color on each side
  float gradientMiddleEdgeSize = 0.0;
  // percentage (0.0 - 1.0) that stays at the center color in the middle
  float gradientMiddleCenterSize = 0.05;
  InterpolationMode gradientInterpolationMode = InterpolationMode::Smooth;

  GradientMode gradientMode = LINEAR_PADDING;
  bool gradientInvertColors = false;



  float effectMinAmplitude = 0.6;
  float effectMaxAmplitude = 1.2;

  float effectEvolveMinSteps = 100;
  float effectEvolveMaxSteps = 200;

  float effectHoldMinSteps = 10;
  float effectHoldMaxSteps = 30;

  bool effectActive = true;



  uint8_t count = LED_COUNT;

  // following var are used to save the settings
  uint32_t changeCounter = 0;   // persistent
  uint32_t lastModifiedMs = 0;  // timestamp for debounce
};




struct Vars {
  Pixel_byte Pixels[LED_COUNT];
  Pixel_byte Colors[LED_COUNT];
  Pixel_float Scale[LED_COUNT];

  // computed end-values, from which Colors[] is built
  Pixel_float colorOne;
  Pixel_float colorTwo;

  float brightness = 255.0;
  float onoffFactor = 1.0f;

  constexpr static size_t Capacity = LED_COUNT;
  size_t Count = Capacity;

  Effect_Container Effect[4];
};

/* compile-time sanity */
static_assert(Vars::Capacity > 0, "LED_COUNT must be > 0");



/* --- Singletons (function-local statics) --- */

/**
 * @brief Returns singleton State object.
 */
inline State &GetState() {
  static State s;
  return s;
}

/**
 * @brief Returns singleton Config object.
 */
inline Config &GetConfig() {
  static Config c;
  return c;
}

/**
 * @brief Returns singleton Vars object.
 */
inline Vars &GetVars() {
  static Vars v;
  return v;
}

/* --- API (all inside namespace so unqualified calls work) --- */

/**
 * Initialize CORE: verify Count, clear buffers, set default state/timing.
 */
inline bool Init() {
  State &s = GetState();
  Vars &v = GetVars();
  Config &c = GetConfig();

  if (v.Count == 0 || v.Count > Vars::Capacity) return false;

  c.brightnessStaging = 255.0;
  c.onoffStaging = 1.0f;
  s.active = true;
  s.processingLastExecutionMs = millis();
  s.effectLastExecutionMs = millis();

  for (size_t i = 0; i < v.Count; ++i) {
    v.Pixels[i].R = 0;
    v.Pixels[i].G = 0;
    v.Pixels[i].B = 0;
    v.Pixels[i].W = 0;

    v.Colors[i].R = 0;
    v.Colors[i].G = 0;
    v.Colors[i].B = 0;
    v.Colors[i].W = 0;

    v.Scale[i].R = 1.0;
    v.Scale[i].G = 1.0;
    v.Scale[i].B = 1.0;
    v.Scale[i].W = 1.0;
  }

  v.colorOne.R = 0.0;
  v.colorOne.G = 0.0;
  v.colorOne.B = 0.0;
  v.colorOne.W = 0.0;

  v.colorTwo.R = 0.0;
  v.colorTwo.G = 0.0;
  v.colorTwo.B = 0.0;
  v.colorTwo.W = 0.0;

  v.onoffFactor = 1.0f;

  c.colorOneStaging.R = 255.0;
  c.colorOneStaging.G = 0.0;
  c.colorOneStaging.B = 0.0;
  c.colorOneStaging.W = 0.0;

  c.colorTwoStaging.R = 0.0;
  c.colorTwoStaging.G = 255.0;
  c.colorTwoStaging.B = 0.0;
  c.colorTwoStaging.W = 0.0;


  for (int n = 0; n < 4; n++) {
    v.Effect[n].prev = 1.0;
    v.Effect[n].next = 1.0;
    v.Effect[n].currentOutput = 1.0;
    v.Effect[n].numSteps = 0;
    v.Effect[n].currentStep = 1;
    v.Effect[n].hold = 0;
  }

  return true;
}



/**
 * Effect Handler function.
 */
inline void Effect() {

  Vars &v = GetVars();
  Config &c = GetConfig();

  auto randomInt = [](int x, int y) {
    return random(x, y + 1);
  };

  auto randomFloat = [](float minValue, float maxValue) {
    const float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minValue + r * (maxValue - minValue);
  };

  auto mapFloatToFloat = [](float x, float x0, float x1, float y0, float y1) {
    if (x <= x0) return y0;
    if (x >= x1) return y1;
    const float t = (x - x0) / (x1 - x0);
    return y0 + t * (y1 - y0);
  };

  auto applySmoothInterpolation = [](float t) {
    t = constrain(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
  };


  if (c.effectActive) {

    for (int n = 0; n < 4; n++) {
      if (v.Effect[n].currentStep > v.Effect[n].numSteps) {
        v.Effect[n].prev = v.Effect[n].next;

        if (v.Effect[n].hold) {
          v.Effect[n].numSteps = randomInt(c.effectHoldMinSteps, c.effectHoldMaxSteps);

          v.Effect[n].hold = !v.Effect[n].hold;
        } else {
          v.Effect[n].next = randomFloat(c.effectMinAmplitude, c.effectMaxAmplitude);
          float diff = v.Effect[n].next - v.Effect[n].prev;
          float diffMappedSteps = mapFloatToFloat(diff, c.effectMinAmplitude, c.effectMaxAmplitude, c.effectEvolveMinSteps, c.effectEvolveMaxSteps);

          v.Effect[n].numSteps = (uint32_t)(diffMappedSteps * randomFloat(0.8, 1.2));

          v.Effect[n].hold = !v.Effect[n].hold;
        }

        v.Effect[n].currentStep = 0;
      }

      float diff = v.Effect[n].next - v.Effect[n].prev;
      float progress = (float)v.Effect[n].currentStep / (float)v.Effect[n].numSteps;
      v.Effect[n].currentOutput = v.Effect[n].prev + applySmoothInterpolation(progress) * diff;


      ++v.Effect[n].currentStep;
    }

    ShiftScaleChannel(v.Effect[0].currentOutput, 0, true);
    ShiftScaleChannel(v.Effect[1].currentOutput, 1, false);
    ShiftScaleChannel(v.Effect[2].currentOutput, 2, true);
    ShiftScaleChannel(v.Effect[3].currentOutput, 3, false);
  } else {

    for (int n = 0; n < 4; n++) {
      ShiftScaleChannel(1.0, 0, true);
    }
  }
  //Serial.println(v.Effect.currentOutput);
}





/**
 * Set logical brightness (0..255).
 */
inline void SetStagingBrightness(float brightness) {
  Config &c = GetConfig();

  c.brightnessStaging = brightness;

  MarkChangeInConfig();
}

/**
 * Get logical brightness (0..255).
 */
inline uint8_t GetBrightness() {
  return GetVars().brightness;
}



/**
 * @brief Move a value towards a target by at most `step`.
 *
 * If the distance to target is <= step, target is returned (completes the move).
 * Otherwise a single step towards target is returned.
 *
 * @param current Current value.
 * @param target  Desired target value.
 * @param step    Maximum absolute step to take (must be >= 0).
 * @return New value after stepping.
 */
inline float StepTowards(float current, float target, float step) {
  if (step <= 0.0f) return target;  // immediate jump if step is zero or negative (defensive)
  const float diff = target - current;
  if (fabsf(diff) <= step) return target;
  return current + (diff > 0.0f ? step : -step);
}


/**
 * @brief Perform fading (one step) of brightness and the two color-sets towards staging values.
 *
 * - Applies `brightnessIncrement` to brightness staging.
 * - Applies `colorIncrement` to each RGBA channel of colorOne and colorTwo.
 * - Clamps resulting channels to [0.0f, 255.0f].
 *
 * @return Number of channels that changed during this call (0 = no change / finished).
 */
inline uint8_t Fade() {
  auto &c = GetConfig();
  auto &v = GetVars();

  uint8_t changes = 0;

  // brightness (float)
  {
    const float prev = v.brightness;
    v.brightness = StepTowards(prev, c.brightnessStaging, c.brightnessIncrement);
    v.brightness = constrain(v.brightness, 0.0f, 255.0f);
    if (fabsf(v.brightness - prev) > 1e-5f) ++changes;
  }

  // on/off fade factor (0..1)
  {
    const float prev = v.onoffFactor;
    v.onoffFactor = StepTowards(prev, c.onoffStaging, c.onoffIncrement);
    v.onoffFactor = constrain(v.onoffFactor, 0.0f, 1.0f);
    if (fabsf(v.onoffFactor - prev) > 1e-5f) ++changes;
  }

  // helper lambda to step & clamp a single channel and count change
  auto stepChannel = [&](float &channel, float target) {
    const float prev = channel;
    channel = StepTowards(prev, target, c.colorIncrement);
    channel = constrain(channel, 0.0f, 255.0f);
    if (fabsf(channel - prev) > 1e-5f) ++changes;
  };

  // color one
  stepChannel(v.colorOne.R, c.colorOneStaging.R);
  stepChannel(v.colorOne.G, c.colorOneStaging.G);
  stepChannel(v.colorOne.B, c.colorOneStaging.B);
  stepChannel(v.colorOne.W, c.colorOneStaging.W);

  // color two
  stepChannel(v.colorTwo.R, c.colorTwoStaging.R);
  stepChannel(v.colorTwo.G, c.colorTwoStaging.G);
  stepChannel(v.colorTwo.B, c.colorTwoStaging.B);
  stepChannel(v.colorTwo.W, c.colorTwoStaging.W);

  return changes;
}




inline void ComputeGradient(GradientMode mode, bool invertColors) {
  auto &v = GetVars();

  if (v.Count == 0) return;

  auto ApplyInterpolation = [](float t, InterpolationMode mode) {
    t = constrain(t, 0.0f, 1.0f);
    switch (mode) {
      case InterpolationMode::Smooth:
        return t * t * (3.0f - 2.0f * t);
      case InterpolationMode::Linear:
      default:
        return t;
    }
  };


  const Pixel_float &primaryColor = invertColors ? v.colorTwo : v.colorOne;
  const Pixel_float &secondaryColor = invertColors ? v.colorOne : v.colorTwo;

  auto applyColor = [&](size_t index, const Pixel_float &src) {
    v.Colors[index].R = static_cast<uint8_t>(src.R);
    v.Colors[index].G = static_cast<uint8_t>(src.G);
    v.Colors[index].B = static_cast<uint8_t>(src.B);
    v.Colors[index].W = static_cast<uint8_t>(src.W);
  };

  auto blendColors = [&](const Pixel_float &a, const Pixel_float &b, float t) {
    t = constrain(t, 0.0f, 1.0f);
    Pixel_float result;
    result.R = a.R + (b.R - a.R) * t;
    result.G = a.G + (b.G - a.G) * t;
    result.B = a.B + (b.B - a.B) * t;
    result.W = a.W + (b.W - a.W) * t;
    return result;
  };

  switch (mode) {
    case SINGLE_COLOR:
      {
        for (size_t i = 0; i < v.Count; ++i) {
          applyColor(i, primaryColor);
        }
        break;
      }

    case MIDPOINT_SPLIT:
      {
        const size_t splitIndex = (v.Count + 1) / 2;
        for (size_t i = 0; i < v.Count; ++i) {
          const auto &src = (i < splitIndex) ? primaryColor : secondaryColor;
          applyColor(i, src);
        }
        break;
      }

    case LINEAR_PADDING:
      {
        const auto &c = GetConfig();
        const float padStart = constrain(c.gradientPaddingBegin, 0.0f, 0.4f);
        const float padValue = constrain(c.gradientPaddingValue, 0.0f, 1.0f);

        const size_t n = v.Count;
        if (n == 0) {
          break;
        }

        if (n == 1) {
          const float w2 = 0.5f;
          Pixel_float blended = blendColors(primaryColor, secondaryColor, w2);
          applyColor(0, blended);
          break;
        }

        const float startIdx = padStart * static_cast<float>(n - 1);
        const float endIdx = (1.0f - padStart) * static_cast<float>(n - 1);
        const float range = endIdx - startIdx;

        for (size_t i = 0; i < n; ++i) {
          float w1;
          if (static_cast<float>(i) <= startIdx) {
            w1 = padValue;
          } else if (static_cast<float>(i) >= endIdx || range <= 0.0f) {
            w1 = 1.0f - padValue;
          } else {
            const float t = (static_cast<float>(i) - startIdx) / range;
            w1 = padValue + (1.0f - 2.0f * padValue) * constrain(t, 0.0f, 1.0f);
          }

          float w2 = 1.0f - w1;
          w1 = constrain(w1, 0.0f, 1.0f);
          w2 = constrain(w2, 0.0f, 1.0f);

          Pixel_float blended = blendColors(primaryColor, secondaryColor, w2);
          applyColor(i, blended);
        }
        break;
      }

    case EDGE_CENTER:
      {
        const auto &c = GetConfig();
        float edgeSize = constrain(c.gradientMiddleEdgeSize, 0.0f, 0.5f);
        float centerSize = constrain(c.gradientMiddleCenterSize, 0.0f, 1.0f);
        const float maxCenter = 1.0f - 2.0f * edgeSize;
        if (centerSize > maxCenter) centerSize = maxCenter;
        if (centerSize < 0.0f) centerSize = 0.0f;

        float transitionTotal = 1.0f - (2.0f * edgeSize + centerSize);
        if (transitionTotal < 0.0f) transitionTotal = 0.0f;
        const float halfTransition = transitionTotal * 0.5f;

        const float leftEdgeEnd = edgeSize;
        const float leftTransitionEnd = leftEdgeEnd + halfTransition;
        const float centerEnd = leftTransitionEnd + centerSize;
        const float rightTransitionEnd = centerEnd + halfTransition;

        const size_t n = v.Count;
        for (size_t i = 0; i < n; ++i) {
          float x = (n <= 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(n - 1);

          if (x <= leftEdgeEnd || halfTransition <= 1e-6f) {
            applyColor(i, primaryColor);
            continue;
          }

          if (x < leftTransitionEnd && halfTransition > 1e-6f) {
            float t = (x - leftEdgeEnd) / halfTransition;
            float blend = ApplyInterpolation(t, c.gradientInterpolationMode);
            Pixel_float blended = blendColors(primaryColor, secondaryColor, blend);
            applyColor(i, blended);
            continue;
          }

          if (x < centerEnd) {
            applyColor(i, secondaryColor);
            continue;
          }

          if (x < rightTransitionEnd && halfTransition > 1e-6f) {
            float t = (x - centerEnd) / halfTransition;
            float blend = ApplyInterpolation(t, c.gradientInterpolationMode);
            Pixel_float blended = blendColors(secondaryColor, primaryColor, blend);
            applyColor(i, blended);
            continue;
          }

          applyColor(i, primaryColor);
        }
        break;
      }

    case LINEAR:
    default:
      {
        if (v.Count <= 1) {
          for (size_t i = 0; i < v.Count; ++i) {
            applyColor(i, primaryColor);
          }
          break;
        }

        for (size_t i = 0; i < v.Count; ++i) {
          const float t = static_cast<float>(i) / static_cast<float>(v.Count - 1);

          Pixel_float blended = blendColors(primaryColor, secondaryColor, t);
          applyColor(i, blended);
        }
        break;
      }
  }
}




/**
 * @brief Apply per-pixel scaling and global intensity factors to Colors[] and write result into Pixels[].
 *
 * For each pixel i:
 *   Pixels[i].<chan> = round( Colors[i].<chan> * Scale[i].<chan> * (Brightness / 255.0f) * OnOffFactor )
 *
 * Float math is used for scale. Result is clamped to [0,255].
 */
inline void ApplyOutputScaling() {
  auto &v = GetVars();

  const float brightnessNorm = constrain(v.brightness, 0.0f, 255.0f) / 255.0f;
  const float onOff = v.onoffFactor;

  const size_t n = v.Count;
  for (size_t i = 0; i < n; ++i) {
    const float scaledR = static_cast<float>(v.Colors[i].R) * v.Scale[i].R;
    const float scaledG = static_cast<float>(v.Colors[i].G) * v.Scale[i].G;
    const float scaledB = static_cast<float>(v.Colors[i].B) * v.Scale[i].B;
    const float scaledW = static_cast<float>(v.Colors[i].W) * v.Scale[i].W;

    const float baseR = constrain(scaledR, 0.0f, 255.0f);
    const float baseG = constrain(scaledG, 0.0f, 255.0f);
    const float baseB = constrain(scaledB, 0.0f, 255.0f);
    const float baseW = constrain(scaledW, 0.0f, 255.0f);

    const float finalR = baseR * brightnessNorm * onOff;
    const float finalG = baseG * brightnessNorm * onOff;
    const float finalB = baseB * brightnessNorm * onOff;
    const float finalW = baseW * brightnessNorm * onOff;

    int ri = static_cast<int>(finalR + 0.5f);
    int gi = static_cast<int>(finalG + 0.5f);
    int bi = static_cast<int>(finalB + 0.5f);
    int wi = static_cast<int>(finalW + 0.5f);

    SetPixelRGBW(i, ri, gi, bi, wi);
  }
}


/**
 * Set a single pixel RGBW (0-based index). Returns false if index invalid.
 */
inline bool SetPixelRGBW(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {

  Vars &v = GetVars();
  if (index >= v.Count || index < 0) return false;

  if (r < 0) r = 0;
  else if (r > 255) r = 255;
  if (g < 0) g = 0;
  else if (g > 255) g = 255;
  if (b < 0) b = 0;
  else if (b > 255) b = 255;
  if (w < 0) w = 0;
  else if (w > 255) w = 255;

  v.Pixels[index].R = r;
  v.Pixels[index].G = g;
  v.Pixels[index].B = b;
  v.Pixels[index].W = w;

  return true;
}



/**
 * @brief Mark the config as dirty by setting the lastModifiedMs and increasing the changeCounter
 * @param NONE
 * @return NONE
 */
inline void MarkChangeInConfig() {
  auto &cfg = GetConfig();

  ++cfg.changeCounter;
  cfg.lastModifiedMs = millis();
}


/**
 * @brief Provoke an immediate save of config by setting the lastModifiedMs to one hour in the past and increasing the changeCounter
 * @param NONE
 * @return NONE
 */
inline void ProvokeImmediateSaveOfConfig() {
  auto &cfg = GetConfig();

  ++cfg.changeCounter;
  cfg.lastModifiedMs = millis() - 3600000;
}








inline void ShiftScaleChannel(float newValue, int8_t channel, bool forward) {
  Vars &v = GetVars();

  if (v.Count == 0) {
    return;
  }

  if (channel < 0 || channel > 3) {
    return;
  }


  float Pixel_float::*channelPtr = &Pixel_float::R;
  switch (channel) {
    case 0:
      channelPtr = &Pixel_float::R;
      break;
    case 1:
      channelPtr = &Pixel_float::G;
      break;
    case 2:
      channelPtr = &Pixel_float::B;
      break;
    case 3:
      channelPtr = &Pixel_float::W;
      break;
    default:
      break;
  }

  const size_t lastIndex = v.Count - 1;

  if (forward) {
    for (size_t i = lastIndex; i > 0; --i) {
      v.Scale[i].*channelPtr = v.Scale[i - 1].*channelPtr;
    }
    v.Scale[0].*channelPtr = newValue;
  } else {
    for (size_t i = 0; i < lastIndex; ++i) {
      v.Scale[i].*channelPtr = v.Scale[i + 1].*channelPtr;
    }
    v.Scale[lastIndex].*channelPtr = newValue;
  }
}














/**
 * Clear active pixels to zero.
 */
inline void Clear() {
  Vars &v = GetVars();
  for (size_t i = 0; i < v.Count; ++i) {
    v.Colors[i].R = 0;
    v.Colors[i].G = 0;
    v.Colors[i].B = 0;
    v.Colors[i].W = 0;
  }
}

/**
 * Map cubic 0..100 -> 0..255 (rounded)
 */
inline int MapCubic100to255(int x) {
  constexpr int inputMin = 0;
  constexpr int inputMax = 100;
  constexpr int outputMax = 255;

  if (x <= inputMin) return 0;
  if (x >= inputMax) return outputMax;

  const float normalized = static_cast<float>(x) / static_cast<float>(inputMax);
  const float scaled = normalized * normalized * normalized;
  return static_cast<int>(scaled * outputMax + 0.5f);
}

}  // namespace CORE
}  // namespace LED
