//////////////////////////////////
//       CONSOLE SERVICES       //
//////////////////////////////////
#pragma once
#include <Arduino.h>
#include <stdarg.h>
#include <stdlib.h>
#include "200_LED_LINKER.h"  // stellt LED:: APIs zur Verfügung
#include "100_DEVICE_LINKER.h"


// Buffer configuration
#define CMD_BUFFER_CAPACITY 96  // inkl. null-terminator



namespace CONSOLE {

inline bool DebugSerialEnabled();
inline void PrintCommandEcho(const char* line);
inline void PrintResponseLine(const __FlashStringHelper* msg);
inline void PrintResponseLine(const char* msg);
inline void PrintResponseLine(const String& msg);
inline void PrintResponseBlankLine();
inline void PrintResponseLineFmt(const char* fmt, ...);
inline void PrintResponseLineFloat(const __FlashStringHelper* prefix, float value, uint8_t digits);

// --- Forward Declarations (Prototypen) ---
// Console lifecycle
void InitializeConsoleInterface();
void Process();
void EvaluateCommand(const char* line);

// Command handlers
void HandleHELP(const char* pos);
void HandleSET(const char* pos);
void HandleSET_COLOR(const char* pos);
void HandleSET_BRIGHTNESS(const char* pos);
void HandleSET_PARAM(const char* pos);
void HandleSET_GRADIENT(const char* pos);
void HandleTOGGLE(const char* pos);
void HandleSYSTEM(const char* pos);
void HandleSYSTEM_RESET(const char* pos);

// Help output
void PrintHelpTop();
void PrintHelpPredefinedColors();
void PrintHelpSet();
void PrintHelpSetParam();
void PrintHelpSetGradient();
void PrintHelpToggle();
void PrintHelpSystem();
void PrintGradientSettings();

// Parsing / helper utilities
bool ParseColorName(const char* name, LED::Pixel_byte& out);
const char* TrimLeading(const char* s);
bool ParseFourUints(const char* s, int& a, int& b, int& c, int& d);
bool ParseBoolToken(const char* s, bool& out);
bool ParseFloatToken(const char* s, float& out);
const char* GradientModeToString(LED::GradientMode mode);
const char* InterpolationModeToString(LED::InterpolationMode mode);
bool ParseGradientModeToken(const char* s, LED::GradientMode& out);
bool ParseInterpolationModeToken(const char* s, LED::InterpolationMode& out);
void SanitizeEdgeCenterConfig(LED::Config& cfg);
void ScheduleSystemRestart(uint32_t delayMs);
void ProcessPendingRestart();

// Internal buffer & state
static char cmdBuffer[CMD_BUFFER_CAPACITY];
static size_t cmdPos = 0;


static bool systemRestartPending = false;
static uint32_t systemRestartDeadlineMs = 0;








/**
 * @brief Initialize console. Call from setup().
 *        Does NOT call Serial.begin() — let your sketch control baud if desired.
 */
inline void InitializeConsoleInterface() {
  cmdPos = 0;
  cmdBuffer[0] = '\0';
  if (ENABLE_COMMAND_LINE_INTERFACE && !Serial) {
    // If Serial isn't started by sketch, start with a safe default.
    Serial.begin(115200);
    // small delay to let USB/CDC enumerate (helpful for native USB boards)
    delay(100);
  }
}



/**
 * @brief Polling-based processing: call this regularly from loop().
 */
inline void Process() {
  if (!ENABLE_COMMAND_LINE_INTERFACE) {
    ProcessPendingRestart();
    return;
  }

  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) break;
    char ch = static_cast<char>(c);

    // ignore CR, treat LF as end-of-line
    if (ch == '\r') continue;

    if (ch == '\n') {
      cmdBuffer[cmdPos] = '\0';
      if (cmdPos > 0) {
        EvaluateCommand(cmdBuffer);
      }
      // reset for next line
      cmdPos = 0;
      cmdBuffer[0] = '\0';
    } else {
      // append if room (leave 1 byte for null)
      if (cmdPos + 1 < CMD_BUFFER_CAPACITY) {
        cmdBuffer[cmdPos++] = ch;
      } else {
        // overflow -> discard and warn
        PrintResponseLine(F("Command buffer overflow. Discarding current line."));
        cmdPos = 0;
        cmdBuffer[0] = '\0';
      }
    }
  }

  ProcessPendingRestart();
}



/**
 * @brief Evaluate a finished command line (without trailing newline).
 *        This function is executed in loop() context.
 */
inline void EvaluateCommand(const char* line) {
  if (!line || !*line) return;

  PrintCommandEcho(line);

  const char* p = TrimLeading(line);

  // HELP commands
  if (strncasecmp(p, "HELP", 4) == 0) {
    HandleHELP(p + 4);
    PrintResponseBlankLine();
    return;
  }

  // SET commands
  if (strncasecmp(p, "SET", 3) == 0) {
    HandleSET(p + 3);
    PrintResponseBlankLine();
    return;
  }

  // TOGGLE commands
  if (strncasecmp(p, "TOGGLE", 6) == 0) {
    HandleTOGGLE(p + 6);
    PrintResponseBlankLine();
    return;
  }

  // SYSTEM commands
  if (strncasecmp(p, "SYSTEM", 6) == 0) {
    HandleSYSTEM(p + 6);
    PrintResponseBlankLine();
    return;
  }

  // SAVE commands
  if (strncasecmp(p, "SAVE", 4) == 0) {
    //ProvokeImmediateSaveOfConfig();
    LED::ProvokeImmediateSaveOfConfig();
    PrintResponseBlankLine();
    return;
  }


  if (DebugSerialEnabled()) {
    String msg = F("Unknown command: ");
    msg += p;
    msg += F(". Write \"HELP\".");
    PrintResponseLine(msg);
  }
  PrintResponseBlankLine();
}


/**
 * Handle "HELP" commands and subcommands.
 *
 * Accepts:
 *   HELP
 *   HELP PREDEFINED
 *   HELP SET
 *   HELP SET PARAM
 *
 * If the supplied string begins with "HELP", you may pass either:
 *   - the full line pointer (pointing at the 'H' of "HELP"), or
 *   - the pointer after the "HELP" token (e.g. p + 4).
 *
 * The function tolerates leading whitespace.
 */
inline void HandleHELP(const char* pos) {
  if (!pos) {
    PrintHelpTop();
    return;
  }

  // If caller passed the full line (starting with "HELP"), skip that token.
  const char* s = pos;
  // skip leading whitespace
  while (*s == ' ' || *s == '\t') ++s;

  // If the string begins with "HELP" (case-insensitive), skip the token.
  if (strncasecmp(s, "HELP", 4) == 0) {
    s += 4;
  }

  // Skip whitespace after optional "HELP"
  while (*s == ' ' || *s == '\t') ++s;

  // If nothing follows, print top-level help.
  if (!*s) {
    PrintHelpTop();
    return;
  }

  // Check for PREDEFINED
  if (strncasecmp(s, "PREDEFINED", 10) == 0) {
    PrintHelpPredefinedColors();
    return;
  }

  // Check for SET (and possibly subcommand)
  if (strncasecmp(s, "SET", 3) == 0) {
    s += 3;
    // skip whitespace after SET
    while (*s == ' ' || *s == '\t') ++s;
    if (!*s) {
      // "HELP SET"
      PrintHelpSet();
      return;
    }
    // if "PARAM" follows: HELP SET PARAM
    if (strncasecmp(s, "PARAM", 5) == 0) {
      // optionally skip whitespace and any trailing tokens
      PrintHelpSetParam();
      return;
    }
    if (strncasecmp(s, "GRADIENT", 8) == 0) {
      PrintHelpSetGradient();
      return;
    }
    // unknown subtopic after SET -> show SET help
    PrintHelpSet();
    return;
  }

  if (strncasecmp(s, "TOGGLE", 6) == 0) {
    PrintHelpToggle();
    return;
  }

  if (strncasecmp(s, "SYSTEM", 6) == 0) {
    PrintHelpSystem();
    return;
  }

  // Unknown help topic -> fallback to top-level + hint
  PrintResponseLine(F("Unknown HELP topic. Valid: HELP, HELP PREDEFINED, HELP SET, HELP SET PARAM, HELP SET GRADIENT, HELP TOGGLE, HELP SYSTEM"));
  PrintHelpTop();
}



/**
 * Handle "SET" command: top-level dispatcher.
 *
 * Syntax:
 *   SET COLOR ONE <...>
 *   SET BRIGHTNESS <value>
 *   SET PARAM <index> <value>
 */
inline void HandleSET(const char* pos) {
  if (!pos) return;
  while (*pos == ' ' || *pos == '\t') ++pos;  // skip spaces
  if (!*pos) {
    PrintHelpSet();
    return;
  }

  // parse first token (subcommand)
  // accept "COLOR", "BRIGHTNESS", "PARAM" (case-insensitive)
  if (strncasecmp(pos, "COLOR", 5) == 0) {
    pos += 5;
    HandleSET_COLOR(pos);
    return;
  }

  if (strncasecmp(pos, "BRIGHTNESS", 10) == 0) {
    pos += 10;
    HandleSET_BRIGHTNESS(pos);
    return;
  }

  if (strncasecmp(pos, "PARAM", 5) == 0) {
    pos += 5;
    HandleSET_PARAM(pos);
    return;
  }

  if (strncasecmp(pos, "GRADIENT", 8) == 0) {
    pos += 8;
    HandleSET_GRADIENT(pos);
    return;
  }

  PrintResponseLine(F("SET: unknown subcommand. Valid: COLOR, BRIGHTNESS, PARAM, GRADIENT. Type HELP."));
}

inline void HandleTOGGLE(const char* pos) {
  if (!pos) {
    PrintHelpToggle();
    return;
  }

  while (*pos == ' ' || *pos == '\t') ++pos;
  if (!*pos) {
    PrintHelpToggle();
    return;
  }

  char sub[32] = {0};
  size_t idx = 0;
  while (*pos && *pos != ' ' && *pos != '\t' && idx < sizeof(sub) - 1) {
    sub[idx++] = toupper((unsigned char)*pos++);
  }
  sub[idx] = '\0';
  while (*pos == ' ' || *pos == '\t') ++pos;

  auto& cfg = LED::GetConfig();

  if (strcmp(sub, "ONOFF") == 0) {
    const bool newState = !mirror.onoff;
    MAIN::SetMirrorOnOff(newState);
    PrintResponseLine(newState ? F("Output fade target set to ON.") : F("Output fade target set to OFF."));
    return;
  }

  if (strcmp(sub, "GRADIENT_INVERT") == 0 || strcmp(sub, "GRADIENTINVERT") == 0) {
    cfg.gradientInvertColors = !cfg.gradientInvertColors;
    LED::MarkChangeInConfig();
    PrintResponseLine(cfg.gradientInvertColors ? F("Gradient color inversion enabled.") : F("Gradient color inversion disabled."));
    return;
  }

  if (strcmp(sub, "HSL_RGBW") == 0 || strcmp(sub, "RGBW_MODE") == 0) {
    const bool rgbw = MAIN::ToggleMirrorRgbwConversion();
    PrintResponseLine(rgbw ? F("HSL conversion now outputs RGBW (white channel enabled).")
                           : F("HSL conversion now outputs RGB only (white channel disabled)."));
    return;
  }

  if (strcmp(sub, "EFFECT") == 0) {
    cfg.effectActive = !cfg.effectActive;
    LED::MarkChangeInConfig();
    PrintResponseLine(cfg.effectActive ? F("Effect enabled.") : F("Effect disabled."));
    return;
  }

  PrintHelpToggle();
}

inline void HandleSYSTEM(const char* pos) {
  if (!pos) {
    PrintHelpSystem();
    return;
  }

  while (*pos == ' ' || *pos == '\t') ++pos;
  if (!*pos) {
    PrintHelpSystem();
    return;
  }

  if (strncasecmp(pos, "RESET", 5) == 0) {
    HandleSYSTEM_RESET(pos + 5);
    return;
  }

  PrintResponseLine(F("SYSTEM: unknown subcommand. Valid: RESET. Type HELP SYSTEM."));
}

inline void HandleSYSTEM_RESET(const char* pos) {
  if (pos) {
    while (*pos == ' ' || *pos == '\t') ++pos;
    if (*pos) {
      PrintResponseLine(F("SYSTEM RESET takes no arguments."));
      return;
    }
  }

  PrintResponseLine(F("System restart requested. Device will reboot in 10 seconds."));
  ScheduleSystemRestart(10000UL);
}

/* ------------------ SET subcommand handlers --------------------------- */

inline void HandleSET_COLOR(const char* pos) {
  if (!pos) {
    PrintResponseLine(F("SET COLOR: missing arguments"));
    return;
  }

  while (*pos == ' ' || *pos == '\t') ++pos;

  LED::SetTarget target;
  if (strncasecmp(pos, "ONE", 3) == 0) {
    target = LED::COLOR_ONE;
    pos += 3;
  } else if (strncasecmp(pos, "TWO", 3) == 0) {
    target = LED::COLOR_TWO;
    pos += 3;
  } else if (isdigit((unsigned char)*pos)) {
    int idx = atoi(pos);
    if (idx == 1) target = LED::COLOR_ONE;
    else if (idx == 2) target = LED::COLOR_TWO;
    else {
      PrintResponseLine(F("SET COLOR: invalid index (use 1 or 2)"));
      return;
    }
    while (isdigit((unsigned char)*pos)) ++pos;
  } else {
    PrintResponseLine(F("SET COLOR: expected ONE or TWO"));
    return;
  }

  while (*pos == ' ' || *pos == '\t') ++pos;
  if (!*pos) {
    PrintResponseLine(F("SET COLOR: missing color arguments"));
    return;
  }

  int r, g, b, w;
  if (sscanf(pos, " %d %d %d %d", &r, &g, &b, &w) == 4) {
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);
    w = constrain(w, 0, 255);
    LED::Pixel_byte pix;
    pix.R = static_cast<uint8_t>(r);
    pix.G = static_cast<uint8_t>(g);
    pix.B = static_cast<uint8_t>(b);
    pix.W = static_cast<uint8_t>(w);

    if (MAIN::SetMirrorColorFromPixel(target, pix)) {
      if (DebugSerialEnabled()) {
        const char* targetName = (target == LED::COLOR_ONE) ? "ONE" : "TWO";
        PrintResponseLineFmt("Color %s set to [%d, %d, %d, %d].",
                             targetName,
                             pix.R,
                             pix.G,
                             pix.B,
                             pix.W);
      }
    } else {
      PrintResponseLine(F("SET COLOR: failed to set color"));
    }
    return;
  }

  LED::Pixel_byte pix;
  if (ParseColorName(pos, pix)) {
    if (MAIN::SetMirrorColorFromPixel(target, pix)) {
      if (DebugSerialEnabled()) {
        const char* targetName = (target == LED::COLOR_ONE) ? "ONE" : "TWO";
        String msg = F("Color ");
        msg += targetName;
        msg += F(" set to ");
        msg += pos;
        msg += '.';
        PrintResponseLine(msg);
      }
    } else {
      PrintResponseLine(F("SET COLOR: failed to set color"));
    }
    return;
  }

  PrintResponseLine(F("SET COLOR: invalid color. Type HELP PREDEFINED for names."));
}

inline void HandleSET_BRIGHTNESS(const char* pos) {
  if (!pos) {
    PrintResponseLine(F("SET BRIGHTNESS: missing value"));
    return;
  }
  while (*pos == ' ' || *pos == '\t') ++pos;
  int bri = -1;
  if (sscanf(pos, " %d", &bri) == 1) {
    bri = constrain(bri, 0, 255);
    MAIN::SetMirrorBrightness(static_cast<uint8_t>(bri));
    PrintResponseLineFmt("Brightness set to %d.", bri);
  } else {
    PrintResponseLine(F("Syntax: SET BRIGHTNESS <0..255>"));
  }
}

inline void HandleSET_PARAM(const char* pos) {
  if (!pos) {
    PrintHelpSetParam();
    return;
  }

  while (*pos == ' ' || *pos == '\t') ++pos;
  if (!*pos) {
    PrintHelpSetParam();
    return;
  }

  if (isdigit((unsigned char)*pos)) {
    char* endIdx = nullptr;
    long idx = strtol(pos, &endIdx, 10);
    if (endIdx == pos) {
      PrintResponseLine(F("SET PARAM: invalid parameter index"));
      return;
    }
    pos = endIdx;
    while (*pos == ' ' || *pos == '\t') ++pos;
    if (!*pos) {
      PrintResponseLine(F("SET PARAM: missing value"));
      return;
    }

    auto& cfg = LED::GetConfig();

    switch (idx) {
      case 1: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 1: value must be > 0"));
          return;
        }
        cfg.colorIncrement = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Color increment set to %.3f.", static_cast<double>(cfg.colorIncrement));
        return;
      }
      case 2: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 2: value must be > 0"));
          return;
        }
        cfg.brightnessIncrement = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Brightness increment set to %.3f.", static_cast<double>(cfg.brightnessIncrement));
        return;
      }
      case 3: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 3: value must be > 0"));
          return;
        }
        cfg.onoffIncrement = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("On/off increment set to %.3f.", static_cast<double>(cfg.onoffIncrement));
        return;
      }
      case 4: {
        long value = strtol(pos, nullptr, 10);
        if (value <= 0) {
          PrintResponseLine(F("SET PARAM 4: value must be > 0"));
          return;
        }
        cfg.processingIntervalMs = static_cast<uint32_t>(value);
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Processing interval set to %lu ms.", static_cast<unsigned long>(cfg.processingIntervalMs));
        return;
      }
      case 5: {
        long value = strtol(pos, nullptr, 10);
        if (value <= 0) {
          PrintResponseLine(F("SET PARAM 5: value must be > 0"));
          return;
        }
        cfg.effectIntervalMs = static_cast<uint32_t>(value);
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect interval set to %lu ms.", static_cast<unsigned long>(cfg.effectIntervalMs));
        return;
      }
      case 6: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 6: value must be > 0"));
          return;
        }
        cfg.effectMinAmplitude = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect min amplitude set to %.3f.", static_cast<double>(cfg.effectMinAmplitude));
        return;
      }
      case 7: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 7: value must be > 0"));
          return;
        }
        cfg.effectMaxAmplitude = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect max amplitude set to %.3f.", static_cast<double>(cfg.effectMaxAmplitude));
        return;
      }
      case 8: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 8: value must be > 0"));
          return;
        }
        cfg.effectEvolveMinSteps = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect evolve min steps set to %.0f.", static_cast<double>(cfg.effectEvolveMinSteps));
        return;
      }
      case 9: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 9: value must be > 0"));
          return;
        }
        cfg.effectEvolveMaxSteps = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect evolve max steps set to %.0f.", static_cast<double>(cfg.effectEvolveMaxSteps));
        return;
      }
      case 10: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 10: value must be > 0"));
          return;
        }
        cfg.effectHoldMinSteps = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect hold min steps set to %.0f.", static_cast<double>(cfg.effectHoldMinSteps));
        return;
      }
      case 11: {
        float value;
        if (!ParseFloatToken(pos, value) || value <= 0.0f) {
          PrintResponseLine(F("SET PARAM 11: value must be > 0"));
          return;
        }
        cfg.effectHoldMaxSteps = value;
        LED::MarkChangeInConfig();
        PrintResponseLineFmt("Effect hold max steps set to %.0f.", static_cast<double>(cfg.effectHoldMaxSteps));
        return;
      }
      case 12: {
        PrintResponseLine(F("SET PARAM 12 has been replaced. Use TOGGLE EFFECT instead."));
        return;
      }
      default:
        PrintResponseLine(F("SET PARAM: unknown parameter index. Type 'HELP SET PARAM'."));
        return;
    }
  }

  // Backward-compatible named parameters
  char nameTok[32] = {0};
  int i = 0;
  while (*pos && *pos != ' ' && *pos != '\t' && i < (int)sizeof(nameTok) - 1) {
    nameTok[i++] = *pos++;
  }
  nameTok[i] = '\0';
  while (*pos == ' ' || *pos == '\t') ++pos;
  if (!*pos) {
    PrintResponseLine(F("SET PARAM: missing value"));
    return;
  }

  for (char* p = nameTok; *p; ++p) *p = toupper((unsigned char)*p);

  PrintResponseLine(F("SET PARAM: unknown parameter. Type 'HELP SET PARAM' for valid names."));
}

inline void HandleSET_GRADIENT(const char* pos) {
  if (!pos) {
    PrintHelpSetGradient();
    return;
  }

  while (*pos == ' ' || *pos == '\t') ++pos;
  if (!*pos) {
    PrintHelpSetGradient();
    return;
  }

  char sub[32] = {0};
  size_t idx = 0;
  while (*pos && *pos != ' ' && *pos != '\t' && idx < sizeof(sub) - 1) {
    sub[idx++] = toupper((unsigned char)*pos++);
  }
  sub[idx] = '\0';
  while (*pos == ' ' || *pos == '\t') ++pos;

  auto& cfg = LED::GetConfig();

  if (strcmp(sub, "MODE") == 0) {
    if (!*pos) {
      PrintResponseLine(F("SET GRADIENT MODE: missing mode"));
      return;
    }
    LED::GradientMode mode;
    if (!ParseGradientModeToken(pos, mode)) {
      PrintResponseLine(F("SET GRADIENT MODE: unknown mode"));
      return;
    }
    if (cfg.gradientMode != mode) {
      cfg.gradientMode = mode;
      LED::MarkChangeInConfig();
    }
    if (DebugSerialEnabled()) {
      String msg = F("Gradient mode set to ");
      msg += GradientModeToString(cfg.gradientMode);
      msg += '.';
      PrintResponseLine(msg);
    }
    return;
  }

  sub[idx] = '\0';
  while (*pos == ' ' || *pos == '\t') ++pos;

  if (strcmp(sub, "PADDINGBEGIN") == 0 || strcmp(sub, "PADBEGIN") == 0 || strcmp(sub, "BEGIN") == 0) {
    float val;
    if (!ParseFloatToken(pos, val)) {
      PrintResponseLine(F("SET GRADIENT PADDINGBEGIN: invalid number"));
      return;
    }
    float clamped = constrain(val, 0.0f, 0.4f);
    if (cfg.gradientPaddingBegin != clamped) {
      cfg.gradientPaddingBegin = clamped;
      LED::MarkChangeInConfig();
    }
    PrintResponseLineFmt("Gradient padding begin set to %.3f.", static_cast<double>(cfg.gradientPaddingBegin));
    return;
  }

  if (strcmp(sub, "PADDINGVALUE") == 0 || strcmp(sub, "PADVALUE") == 0 || strcmp(sub, "VALUE") == 0) {
    float val;
    if (!ParseFloatToken(pos, val)) {
      PrintResponseLine(F("SET GRADIENT PADDINGVALUE: invalid number"));
      return;
    }
    float clamped = constrain(val, 0.0f, 1.0f);
    if (cfg.gradientPaddingValue != clamped) {
      cfg.gradientPaddingValue = clamped;
      LED::MarkChangeInConfig();
    }
    PrintResponseLineFmt("Gradient padding value set to %.3f.", static_cast<double>(cfg.gradientPaddingValue));
    return;
  }

  if (strcmp(sub, "EDGE") == 0) {
    float val;
    if (!ParseFloatToken(pos, val)) {
      PrintResponseLine(F("SET GRADIENT EDGE: invalid number"));
      return;
    }
    float oldEdge = cfg.gradientMiddleEdgeSize;
    float oldCenter = cfg.gradientMiddleCenterSize;
    cfg.gradientMiddleEdgeSize = constrain(val, 0.0f, 0.5f);
    SanitizeEdgeCenterConfig(cfg);
    if (cfg.gradientMiddleEdgeSize != oldEdge || cfg.gradientMiddleCenterSize != oldCenter) {
      LED::MarkChangeInConfig();
    }
    PrintResponseLineFmt("Gradient edge size set to %.3f.", static_cast<double>(cfg.gradientMiddleEdgeSize));
    return;
  }

  if (strcmp(sub, "CENTER") == 0) {
    float val;
    if (!ParseFloatToken(pos, val)) {
      PrintResponseLine(F("SET GRADIENT CENTER: invalid number"));
      return;
    }
    float oldEdge = cfg.gradientMiddleEdgeSize;
    float oldCenter = cfg.gradientMiddleCenterSize;
    cfg.gradientMiddleCenterSize = constrain(val, 0.0f, 1.0f);
    SanitizeEdgeCenterConfig(cfg);
    if (cfg.gradientMiddleEdgeSize != oldEdge || cfg.gradientMiddleCenterSize != oldCenter) {
      LED::MarkChangeInConfig();
    }
    PrintResponseLineFmt("Gradient center size set to %.3f.", static_cast<double>(cfg.gradientMiddleCenterSize));
    return;
  }

  if (strcmp(sub, "INTERPOLATION") == 0) {
    LED::InterpolationMode interp;
    if (!ParseInterpolationModeToken(pos, interp)) {
      PrintResponseLine(F("SET GRADIENT INTERPOLATION: invalid value"));
      return;
    }
    if (cfg.gradientInterpolationMode != interp) {
      cfg.gradientInterpolationMode = interp;
      LED::MarkChangeInConfig();
    }
    if (DebugSerialEnabled()) {
      String msg = F("Gradient interpolation set to ");
      msg += InterpolationModeToString(cfg.gradientInterpolationMode);
      msg += '.';
      PrintResponseLine(msg);
    }
    return;
  }

  if (strcmp(sub, "SHOW") == 0) {
    PrintGradientSettings();
    return;
  }

  PrintResponseLine(F("SET GRADIENT: unknown subcommand. Type HELP SET GRADIENT."));
}


/* ------------------ Help output functions ------------------------------ */

inline void PrintHelpTop() {
  if (!DebugSerialEnabled()) return;
  PrintResponseLine(F("Commands:"));
  PrintResponseLine(F("  SET <sub> ...          -> set color/brightness/params"));
  PrintResponseLine(F("                            <sub>: COLOR, BRIGHTNESS, PARAM, GRADIENT"));
  PrintResponseLine(F("  TOGGLE <sub> ...       -> toggle features"));
  PrintResponseLine(F("                            <sub>: ONOFF, GRADIENT_INVERT, HSL_RGBW, EFFECT"));
  PrintResponseLine(F("  SYSTEM <sub> ...       -> system maintenance commands"));
  PrintResponseLine(F("                            <sub>: RESET"));
  PrintResponseLine(F("  HELP                   -> this message"));
  PrintResponseLine(F("  HELP PREDEFINED        -> list named colors"));
  PrintResponseLine(F("  HELP SET               -> show SET subcommands"));
  PrintResponseLine(F("  HELP SET PARAM         -> show available parameters"));
  PrintResponseLine(F("  HELP SET GRADIENT      -> show gradient options"));
  PrintResponseLine(F("  HELP TOGGLE            -> show toggle options"));
  PrintResponseLine(F("  HELP SYSTEM            -> show SYSTEM options"));
}

inline void PrintHelpPredefinedColors() {
  if (!DebugSerialEnabled()) return;
  PrintResponseLine(F("Predefined color names (case-insensitive):"));
  PrintResponseLine(F("  RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, ORANGE, PURPLE, PINK,"));
  PrintResponseLine(F("  BLACK, WHITE, FULLWHITE_RGB, WARMWHITE_RGB, COOLWHITE_RGB"));
  PrintResponseLine(F("  Hex: #RRGGBB or 0xRRGGBB"));
}

inline void PrintHelpSet() {
  if (!DebugSerialEnabled()) return;
  PrintResponseLine(F("SET usage:"));
  PrintResponseLine(F("  SET COLOR <ONE|TWO> <r g b w>"));
  PrintResponseLine(F("  SET COLOR <ONE|TWO> <name>"));
  PrintResponseLine(F("  SET BRIGHTNESS <0..255>"));
  PrintResponseLine(F("  SET PARAM <index> <value>"));
  PrintResponseLine(F("  SET GRADIENT <sub> ..."));
  PrintResponseLine(F("Type HELP SET GRADIENT for gradient options"));
  PrintResponseLine(F("Type HELP SET PARAM for available parameters"));
}

inline void PrintHelpSetParam() {
  if (!DebugSerialEnabled()) return;
  const auto& cfg = LED::GetConfig();
  PrintResponseLine(F("SET PARAM available parameters (use SET PARAM <index> <value>):"));
  PrintResponseLineFmt("  1) colorIncrement       | %.3f | Color fade step per update",
                       static_cast<double>(cfg.colorIncrement));
  PrintResponseLineFmt("  2) brightnessIncrement  | %.3f | Brightness fade step per update",
                       static_cast<double>(cfg.brightnessIncrement));
  PrintResponseLineFmt("  3) onoffIncrement       | %.3f | On/off fade step per update",
                       static_cast<double>(cfg.onoffIncrement));
  PrintResponseLineFmt("  4) processingIntervalMs | %lu | LED update interval (ms)",
                       static_cast<unsigned long>(cfg.processingIntervalMs));
  PrintResponseLineFmt("  5) effectIntervalMs     | %lu | Effect timing interval (ms)",
                       static_cast<unsigned long>(cfg.effectIntervalMs));
  PrintResponseLineFmt("  6) effectMinAmplitude   | %.3f | Minimum random amplitude", static_cast<double>(cfg.effectMinAmplitude));
  PrintResponseLineFmt("  7) effectMaxAmplitude   | %.3f | Maximum random amplitude", static_cast<double>(cfg.effectMaxAmplitude));
  PrintResponseLineFmt("  8) effectEvolveMinSteps | %.0f | Minimum evolve steps", static_cast<double>(cfg.effectEvolveMinSteps));
  PrintResponseLineFmt("  9) effectEvolveMaxSteps | %.0f | Maximum evolve steps", static_cast<double>(cfg.effectEvolveMaxSteps));
  PrintResponseLineFmt(" 10) effectHoldMinSteps   | %.0f | Minimum hold steps", static_cast<double>(cfg.effectHoldMinSteps));
  PrintResponseLineFmt(" 11) effectHoldMaxSteps   | %.0f | Maximum hold steps", static_cast<double>(cfg.effectHoldMaxSteps));
  PrintResponseLine(F("Use TOGGLE EFFECT to enable or disable the effect engine."));
}

inline void PrintHelpSetGradient() {
  if (!DebugSerialEnabled()) return;
  PrintResponseLine(F("SET GRADIENT usage:"));
  PrintResponseLine(F("  SET GRADIENT MODE <LINEAR|LINEAR_PADDING|SINGLE_COLOR|MIDPOINT_SPLIT|EDGE_CENTER>"));
  PrintResponseLine(F("  SET GRADIENT PADDINGBEGIN <0.0..0.4>   (LINEAR_PADDING outer padding start)"));
  PrintResponseLine(F("  SET GRADIENT PADDINGVALUE <0.0..1.0>   (LINEAR_PADDING padding mix ratio)"));
  PrintResponseLine(F("  SET GRADIENT EDGE <0.0..0.5>        (EDGE_CENTER mode edge size per side)"));
  PrintResponseLine(F("  SET GRADIENT CENTER <0.0..1.0>      (EDGE_CENTER mode center size)"));
  PrintResponseLine(F("  SET GRADIENT INTERPOLATION <LINEAR|SMOOTH>"));
  PrintResponseLine(F("  SET GRADIENT SHOW                    (display current settings)"));
}

inline void PrintHelpToggle() {
  if (!DebugSerialEnabled()) return;
  PrintResponseLine(F("TOGGLE usage:"));
  PrintResponseLine(F("  TOGGLE ONOFF               (toggle output fade target between on/off)"));
  PrintResponseLine(F("  TOGGLE GRADIENT_INVERT     (toggle gradient color inversion)"));
  PrintResponseLine(F("  TOGGLE HSL_RGBW            (toggle HSL conversion between RGB and RGBW output)"));
  PrintResponseLine(F("  TOGGLE EFFECT              (toggle the effect engine on/off)"));
}

inline void PrintHelpSystem() {
  if (!DebugSerialEnabled()) return;
  PrintResponseLine(F("SYSTEM usage:"));
  PrintResponseLine(F("  SYSTEM RESET"));
  PrintResponseLine(F("    -> schedules a general 10s restart countdown immediately"));
}

inline void PrintGradientSettings() {
  if (!DebugSerialEnabled()) return;

  const auto& cfg = LED::GetConfig();
  PrintResponseLine(F("Current gradient configuration:"));

  String mode = F("  Mode: ");
  mode += GradientModeToString(cfg.gradientMode);
  PrintResponseLine(mode);

  PrintResponseLine(cfg.gradientInvertColors ? F("  Color inversion: enabled") : F("  Color inversion: disabled"));
  PrintResponseLineFmt("  Padding begin: %.3f", static_cast<double>(cfg.gradientPaddingBegin));
  PrintResponseLineFmt("  Padding value: %.3f", static_cast<double>(cfg.gradientPaddingValue));
  PrintResponseLineFmt("  Edge size: %.3f", static_cast<double>(cfg.gradientMiddleEdgeSize));
  PrintResponseLineFmt("  Center size: %.3f", static_cast<double>(cfg.gradientMiddleCenterSize));

  String interp = F("  Interpolation: ");
  interp += InterpolationModeToString(cfg.gradientInterpolationMode);
  PrintResponseLine(interp);
}


/* ------------------ System helper utilities --------------------------- */

inline void ScheduleSystemRestart(uint32_t delayMs) {
  systemRestartPending = true;
  systemRestartDeadlineMs = millis() + delayMs;
}

inline void ProcessPendingRestart() {
  if (!systemRestartPending) return;

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - systemRestartDeadlineMs) < 0) return;

  systemRestartPending = false;
  PrintResponseLine(F("Restarting now as requested..."));
  delay(20);
  ESP.restart();
}


/* ------------------ Parsing helpers ------------------------------------ */

inline bool ParseBoolToken(const char* s, bool& out) {
  if (!s || !*s) return false;
  while (*s == ' ' || *s == '\t') ++s;
  if (strcasecmp(s, "1") == 0 || strcasecmp(s, "true") == 0 || strcasecmp(s, "on") == 0) {
    out = true;
    return true;
  }
  if (strcasecmp(s, "0") == 0 || strcasecmp(s, "false") == 0 || strcasecmp(s, "off") == 0) {
    out = false;
    return true;
  }
  return false;
}

inline bool ParseFloatToken(const char* s, float& out) {
  if (!s) return false;
  while (*s == ' ' || *s == '\t') ++s;
  if (!*s) return false;
  char* end = nullptr;
  out = strtof(s, &end);
  if (s == end) return false;
  return true;
}

inline const char* GradientModeToString(LED::GradientMode mode) {
  switch (mode) {
    case LED::LINEAR: return "LINEAR";
    case LED::LINEAR_PADDING: return "LINEAR_PADDING";
    case LED::SINGLE_COLOR: return "SINGLE_COLOR";
    case LED::MIDPOINT_SPLIT: return "MIDPOINT_SPLIT";
    case LED::EDGE_CENTER: return "EDGE_CENTER";
    default: return "UNKNOWN";
  }
}

inline const char* InterpolationModeToString(LED::InterpolationMode mode) {
  switch (mode) {
    case LED::InterpolationMode::Linear: return "LINEAR";
    case LED::InterpolationMode::Smooth: return "SMOOTH";
    default: return "UNKNOWN";
  }
}

inline bool ParseGradientModeToken(const char* s, LED::GradientMode& out) {
  if (!s) return false;
  while (*s == ' ' || *s == '\t') ++s;
  if (!*s) return false;
  char buf[32] = {0};
  size_t i = 0;
  while (*s && *s != ' ' && *s != '\t' && i < sizeof(buf) - 1) {
    char ch = *s++;
    if (ch == '-') ch = '_';
    buf[i++] = toupper((unsigned char)ch);
  }
  buf[i] = '\0';

  if (strcmp(buf, "LINEAR") == 0) {
    out = LED::LINEAR;
    return true;
  }
  if (strcmp(buf, "LINEAR_PADDING") == 0 || strcmp(buf, "LINEARPADDING") == 0) {
    out = LED::LINEAR_PADDING;
    return true;
  }
  if (strcmp(buf, "SINGLE") == 0 || strcmp(buf, "SINGLE_COLOR") == 0) {
    out = LED::SINGLE_COLOR;
    return true;
  }
  if (strcmp(buf, "MIDPOINT") == 0 || strcmp(buf, "MIDPOINT_SPLIT") == 0) {
    out = LED::MIDPOINT_SPLIT;
    return true;
  }
  if (strcmp(buf, "EDGE_CENTER") == 0 || strcmp(buf, "MIDDLE") == 0 || strcmp(buf, "EDGE") == 0) {
    out = LED::EDGE_CENTER;
    return true;
  }
  if (isdigit((unsigned char)buf[0]) || buf[0] == '-') {
    int idx = atoi(buf);
    if (idx >= static_cast<int>(LED::LINEAR) && idx <= static_cast<int>(LED::EDGE_CENTER)) {
      out = static_cast<LED::GradientMode>(idx);
      return true;
    }
  }
  return false;
}

inline bool ParseInterpolationModeToken(const char* s, LED::InterpolationMode& out) {
  if (!s) return false;
  while (*s == ' ' || *s == '\t') ++s;
  if (!*s) return false;
  char buf[16] = {0};
  size_t i = 0;
  while (*s && *s != ' ' && *s != '\t' && i < sizeof(buf) - 1) {
    buf[i++] = toupper((unsigned char)*s++);
  }
  buf[i] = '\0';

  if (strcmp(buf, "LINEAR") == 0) {
    out = LED::InterpolationMode::Linear;
    return true;
  }
  if (strcmp(buf, "SMOOTH") == 0 || strcmp(buf, "SCURVE") == 0) {
    out = LED::InterpolationMode::Smooth;
    return true;
  }
  return false;
}

inline void SanitizeEdgeCenterConfig(LED::Config& cfg) {
  cfg.gradientMiddleEdgeSize = constrain(cfg.gradientMiddleEdgeSize, 0.0f, 0.5f);
  cfg.gradientMiddleCenterSize = constrain(cfg.gradientMiddleCenterSize, 0.0f, 1.0f);
  const float maxCenter = 1.0f - 2.0f * cfg.gradientMiddleEdgeSize;
  if (cfg.gradientMiddleCenterSize > maxCenter) {
    cfg.gradientMiddleCenterSize = (maxCenter > 0.0f) ? maxCenter : 0.0f;
  }
  if (cfg.gradientMiddleCenterSize < 0.0f) {
    cfg.gradientMiddleCenterSize = 0.0f;
  }
}



// --- Color name parsing helper -----------------------------------------
/**
 * @brief Map textual color names to LED::Pixel (RGBW).
 * @param name   input string (any case)
 * @param out    pixel to fill
 * @return true if name recognized
 */
inline bool ParseColorName(const char* name, LED::Pixel_byte& out) {
  if (!name || !*name) return false;
  // normalize: skip leading spaces
  while (*name == ' ' || *name == '\t') ++name;

  // Case-insensitive comparisons
  if (strncasecmp(name, "RED", 3) == 0) {
    out = { 255, 0, 0, 0 };
    return true;
  }
  if (strncasecmp(name, "GREEN", 5) == 0) {
    out = { 0, 255, 0, 0 };
    return true;
  }
  if (strncasecmp(name, "BLUE", 4) == 0) {
    out = { 0, 0, 255, 0 };
    return true;
  }
  if (strncasecmp(name, "WHITE", 5) == 0) {
    out = { 0, 0, 0, 255 };
    return true;  // full white on W channel for RGBW strips
  }
  if (strncasecmp(name, "FULLWHITE_RGB", 5) == 0) {
    out = { 255, 255, 255, 0 };
    return true;  // full white on W channel for RGBW strips
  }
  if (strncasecmp(name, "BLACK", 5) == 0 || strncasecmp(name, "OFF", 3) == 0) {
    out = { 0, 0, 0, 0 };
    return true;
  }
  if (strncasecmp(name, "YELLOW", 6) == 0) {
    out = { 255, 255, 0, 0 };
    return true;
  }
  if (strncasecmp(name, "CYAN", 4) == 0) {
    out = { 0, 255, 255, 0 };
    return true;
  }
  if (strncasecmp(name, "MAGENTA", 7) == 0 || strncasecmp(name, "FUCHSIA", 7) == 0) {
    out = { 255, 0, 255, 0 };
    return true;
  }
  if (strncasecmp(name, "ORANGE", 6) == 0) {
    out = { 255, 128, 0, 0 };
    return true;
  }
  if (strncasecmp(name, "PURPLE", 6) == 0) {
    out = { 128, 0, 128, 0 };
    return true;
  }
  if (strncasecmp(name, "PINK", 4) == 0) {
    out = { 255, 192, 203, 0 };
    return true;
  }
  if (strncasecmp(name, "WARMWHITE_RGB", 9) == 0) {
    out = { 255, 147, 41, 0 };
    return true;  // warm approximate
  }
  if (strncasecmp(name, "COOLWHITE_RGB", 9) == 0) {
    out = { 201, 226, 255, 0 };
    return true;  // cool approximate
  }
  // allow hex style: "#RRGGBB" or "0xRRGGBB"
  if (name[0] == '#' || (name[0] == '0' && (name[1] == 'x' || name[1] == 'X'))) {
    unsigned int hex = 0;
    if (name[0] == '#') {
      sscanf(name + 1, "%x", &hex);
    } else {
      sscanf(name, "%x", &hex);
    }
    // hex expected as RRGGBB
    uint8_t r = (hex >> 16) & 0xFF;
    uint8_t g = (hex >> 8) & 0xFF;
    uint8_t b = (hex)&0xFF;
    out = { r, g, b, 0 };
    return true;
  }
  return false;
}


/**
 * @brief Helper: trim leading spaces (in-place) and return pointer to first non-space.
 */
inline const char* TrimLeading(const char* s) {
  while (*s == ' ' || *s == '\t') ++s;
  return s;
}



/**
 * @brief Parse four integers from a whitespace or comma separated string.
 *        Returns true on success.
 */
inline bool ParseFourUints(const char* s, int& a, int& b, int& c, int& d) {
  // allow both "r g b w" and "r,g,b,w"
  // use sscanf which is available in Arduino core
  int matched = sscanf(s, " %d %d %d %d", &a, &b, &c, &d);
  if (matched == 4) return true;
  // try comma-separated
  matched = sscanf(s, " %d , %d , %d , %d", &a, &b, &c, &d);
  return (matched == 4);
}




inline bool DebugSerialEnabled() {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  return true;
#else
  return false;
#endif
}

inline void PrintCommandEcho(const char* line) {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  Serial.println(line);
#else
  (void)line;
#endif
}

inline void PrintResponseLine(const __FlashStringHelper* msg) {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  Serial.print(F("> "));
  Serial.println(msg);
#else
  (void)msg;
#endif
}

inline void PrintResponseLine(const char* msg) {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  Serial.print(F("> "));
  Serial.println(msg);
#else
  (void)msg;
#endif
}

inline void PrintResponseLine(const String& msg) {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  Serial.print(F("> "));
  Serial.println(msg);
#else
  (void)msg;
#endif
}

inline void PrintResponseBlankLine() {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  Serial.println();
#endif
}

inline void PrintResponseLineFmt(const char* fmt, ...) {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  if (!fmt) {
    Serial.println();
    return;
  }
  constexpr size_t BUF_SZ = 192;
  char buf[BUF_SZ];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, BUF_SZ, fmt, ap);
  va_end(ap);
  buf[BUF_SZ - 1] = '\0';
  Serial.print(F("> "));
  Serial.println(buf);
#else
  (void)fmt;
#endif
}

inline void PrintResponseLineFloat(const __FlashStringHelper* prefix, float value, uint8_t digits) {
#if defined(DEBUG_SERIAL) && (DEBUG_SERIAL == 1)
  Serial.print(F("> "));
  Serial.print(prefix);
  Serial.println(value, digits);
#else
  (void)prefix;
  (void)value;
  (void)digits;
#endif
}


}  // namespace CONSOLE
