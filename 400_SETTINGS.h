//////////////////////////////////
//        PERSISTANCE           //
//////////////////////////////////
#pragma once
/**
 * 300_SETTINGS.h
 *
 * Header-only settings persistence for ESP32 (Preferences).
 * - Generic ConfigId enum for known configs (currently LED only)
 * - Save/Load whole POD Config structs
 * - Uses changeCounter + lastModifiedMs fields inside each Config to detect changes
 *
 * Requirements:
 *  - Define CONFIG_VERSION before including this header.
 *  - Include this header after headers that fully define:
 *      LED::Config and LED::GetConfig()
 *
 * Usage:
 *  SETTINGS::Init();
 *  SETTINGS::LoadConfig(SETTINGS::ConfigId::LED);
 *  // When you modify a config:
 *  cfg.lastModifiedMs = millis();
 *  ++cfg.changeCounter;
 *  // Regularly:
 *  SETTINGS::Update();
 */

#include <Arduino.h>
#include <Preferences.h>

#ifndef CONFIG_VERSION
#error "CONFIG_VERSION must be defined before including 300_SETTINGS.h, e.g. #define CONFIG_VERSION \"V01.01.04\""
#endif

namespace SETTINGS {

/* -------------------- configuration constants ------------------------- */

static constexpr const char* kPrefsNamespace = "appcfg";
static constexpr const char* kKeyLed = "led_cfg";

static constexpr uint32_t kBlobMagic = 0xC00F1342u;
static constexpr size_t kSketchVersionLen = (sizeof(CONFIG_VERSION) - 1);
static_assert(kSketchVersionLen > 0 && kSketchVersionLen <= 16, "CONFIG_VERSION too long; keep <=16 chars");

static uint32_t g_autoSaveDelayMs = 15000u;  // debounce interval

// static buffer for blob operations (avoid runtime malloc)
static constexpr size_t kMaxBlobBuffer = 512;
static uint8_t g_blobBuffer[kMaxBlobBuffer] = { 0 };

/* -------------------- ConfigId --------------------------------------- */

enum class ConfigId : uint8_t {
  LED = 0,
  COUNT
};

inline const char* KeyFor(ConfigId id) {
  switch (id) {
    case ConfigId::LED: return kKeyLed;
    default: return nullptr;
  }
}

/* -------------------- internal tracking ------------------------------- */

// last saved changeCounter per ConfigId (persisted changeCounter -> if differ => needs save)
static uint32_t g_lastSavedCounter[static_cast<size_t>(ConfigId::COUNT)] = { 0 };

// optional lastSavedMs mirror (not required but useful for debugging)
//static uint32_t g_lastSavedMs[static_cast<size_t>(ConfigId::COUNT)] = { 0 };

/* -------------------- helpers ---------------------------------------- */

inline uint16_t simpleChecksum(const uint8_t* data, size_t len) {
  uint32_t s = 0;
  for (size_t i = 0; i < len; ++i) s += data[i];
  return static_cast<uint16_t>(s & 0xFFFFu);
}

/**
 * Blob layout:
 *  [magic: u32][sketchVersion: kSketchVersionLen bytes][payload bytes][checksum: u16]
 */

template<typename T>
inline bool LoadStructPref(const char* key, T& outObj) {
  Preferences pref;
  pref.begin(kPrefsNamespace, true); // read-only
  size_t total = pref.getBytesLength(key);
  if (total == 0) {
    pref.end();
    if (DEBUG_SERIAL) Serial.println(F("LoadStructPref: no data for key"));
    return false;
  }

  // minimal: configVersion + checksum (payload may be zero)
  const size_t minHeader = kSketchVersionLen + sizeof(uint16_t);
  if (total < minHeader) {
    pref.end();
    if (DEBUG_SERIAL) Serial.println(F("LoadStructPref: stored blob too small"));
    return false;
  }
  if (total > kMaxBlobBuffer) {
    pref.end();
    if (DEBUG_SERIAL) {
      Serial.print(F("LoadStructPref: stored blob exceeds kMaxBlobBuffer ("));
      Serial.print(total);
      Serial.print(F(" > "));
      Serial.print(kMaxBlobBuffer);
      Serial.println(F(")"));
    }
    return false;
  }

  size_t read = pref.getBytes(key, g_blobBuffer, total);
  pref.end();
  if (read != total) {
    if (DEBUG_SERIAL) {
      Serial.print(F("LoadStructPref: pref.getBytes returned "));
      Serial.print(read);
      Serial.print(F(" but expected "));
      Serial.println(total);
    }
    return false;
  }

  uint8_t* cur = g_blobBuffer;

  // Compare stored CONFIG_VERSION string (kSketchVersionLen bytes) with current CONFIG_VERSION
  if (memcmp(cur, CONFIG_VERSION, kSketchVersionLen) != 0) {
    if (DEBUG_SERIAL) {
      Serial.print(F("LoadStructPref: config-version string mismatch (stored != current): stored='"));
      // print stored string safely (may not be null-terminated)
      char tmp[kSketchVersionLen + 1];
      memcpy(tmp, cur, kSketchVersionLen);
      tmp[kSketchVersionLen] = '\0';
      Serial.print(tmp);
      Serial.print(F("' expected='"));
      Serial.print(CONFIG_VERSION);
      Serial.println(F("')"));
    }
    return false;
  }
  cur += kSketchVersionLen;

  const size_t payloadLen = total - (kSketchVersionLen + sizeof(uint16_t));
  if (payloadLen != sizeof(T)) {
    if (DEBUG_SERIAL) {
      Serial.print(F("LoadStructPref: payload size mismatch (stored="));
      Serial.print(payloadLen);
      Serial.print(F(" expected="));
      Serial.print(sizeof(T));
      Serial.println(F(")"));
    }
    return false;
  }

  // copy payload into output object
  memcpy(&outObj, cur, payloadLen);
  cur += payloadLen;

  // read stored checksum
  uint16_t storedCsum = 0;
  memcpy(&storedCsum, cur, sizeof(uint16_t));

  // compute checksum of the payload we just loaded
  uint16_t computed = simpleChecksum(reinterpret_cast<const uint8_t*>(&outObj), payloadLen);
  if (storedCsum != computed) {
    if (DEBUG_SERIAL) {
      Serial.print(F("LoadStructPref: checksum mismatch (stored=0x"));
      Serial.print(storedCsum, HEX);
      Serial.print(F(" computed=0x"));
      Serial.print(computed, HEX);
      Serial.println(F(")"));
    }
    return false;
  }

  return true;
}

template<typename T>
inline bool SaveStructPref(const char* key, const T& obj) {
  const size_t payloadLen = sizeof(T);
  const size_t total = kSketchVersionLen + payloadLen + sizeof(uint16_t);
  if (total > kMaxBlobBuffer) {
    if (DEBUG_SERIAL) {
      Serial.print(F("SaveStructPref: blob too large ("));
      Serial.print(total);
      Serial.print(F(" > "));
      Serial.print(kMaxBlobBuffer);
      Serial.println(F(")"));
    }
    return false;
  }

  // prepare buffer: [version_string (kSketchVersionLen)] [payload] [csum]
  uint8_t* cur = g_blobBuffer;

  // zero pad version field, then copy CONFIG_VERSION (which is a string literal)
  memset(cur, 0, kSketchVersionLen);
  size_t copyLen = strlen(CONFIG_VERSION);
  if (copyLen > kSketchVersionLen) copyLen = kSketchVersionLen;
  memcpy(cur, CONFIG_VERSION, copyLen);
  cur += kSketchVersionLen;

  // payload
  memcpy(cur, reinterpret_cast<const uint8_t*>(&obj), payloadLen);
  cur += payloadLen;

  // checksum
  uint16_t csum = simpleChecksum(reinterpret_cast<const uint8_t*>(&obj), payloadLen);
  memcpy(cur, &csum, sizeof(uint16_t));

  Preferences pref;
  pref.begin(kPrefsNamespace, false); // read/write
  bool ok = pref.putBytes(key, g_blobBuffer, total);
  pref.end();

  if (!ok && DEBUG_SERIAL) {
    Serial.println(F("SaveStructPref: pref.putBytes failed"));
  } else if (ok && DEBUG_SERIAL) {
    Serial.print(F("SETTINGS: saved key="));
    Serial.print(key);
    Serial.print(F(" size="));
    Serial.println(total);
    Serial.println();
  }

  return ok;
}


/* -------------------- Public API ------------------------------------- */

/**
 * Init (no-op for Preferences, kept for API symmetry).
 */
inline void Init() {
  // no-op
}

/**
 * Save by ConfigId (reads singleton and writes).
 * Requires that corresponding GetConfig() and Config type exist.
 */
inline bool SaveConfig(ConfigId id) {
  switch (id) {
    case ConfigId::LED:
      return SaveStructPref(kKeyLed, LED::GetConfig());
    default:
      return false;
  }
}

/**
 * Load by ConfigId (loads into singleton). Returns true if load succeeded.
 */
inline bool LoadConfig(ConfigId id) {
  switch (id) {
    case ConfigId::LED:
      {
        LED::Config tmp;
        if (!LoadStructPref(kKeyLed, tmp)) return false;
        LED::GetConfig() = tmp;
        g_lastSavedCounter[static_cast<size_t>(id)] = tmp.changeCounter;
        //g_lastSavedMs[static_cast<size_t>(id)] = tmp.lastModifiedMs;
        return true;
      }
    default:
      return false;
  }
}

/**
 * Save all known configs.
 */
inline bool SaveAll() {
  bool ok = true;
  for (uint8_t i = 0; i < static_cast<uint8_t>(ConfigId::COUNT); ++i) {
    auto id = static_cast<ConfigId>(i);
    if (!SaveConfig(id)) {
      ok = false;
      continue;
    }
    if (id == ConfigId::LED) {
      g_lastSavedCounter[i] = LED::GetConfig().changeCounter;
    }
  }
  return ok;
}

/**
 * Load all known configs.
 */
inline bool LoadAll() {
  bool ok = true;
  for (uint8_t i = 0; i < static_cast<uint8_t>(ConfigId::COUNT); ++i) {
    if (!LoadConfig(static_cast<ConfigId>(i))) ok = false;
  }
  return ok;
}

/**
 * Erase stored configs.
 */
inline void EraseAll() {
  Preferences pref;
  pref.begin(kPrefsNamespace, false);
  pref.remove(kKeyLed);
  pref.end();
}

/* -------------------- Update Loop ------------------------------------ */

/**
 * Set autosave interval (ms).
 */
inline void SetAutoSaveIntervalMs(uint32_t ms) {
  g_autoSaveDelayMs = ms;
}

/**
 * Update() : check each Config's changeCounter and lastModifiedMs and save
 * when:
 *   - changeCounter != lastSavedCounter  (i.e. config changed since last save)
 *   - AND (now - lastModifiedMs) >= debounce
 *
 * Notes about millis()/wrap:
 *   Subtraction is performed unsigned (uint32_t). If lastModifiedMs was from a
 *   previous run and thus greater than current millis() after a reboot, the
 *   unsigned subtraction wraps and yields a large value -> condition (>= debounce)
 *   will be true and save will occur. This avoids missed saves due to clock reset.
 *
 * Returns true if any save occurred.
 */
inline bool Update() {
  bool savedAny = false;
  const uint32_t now = millis();

  // LED
  {
    const size_t idx = static_cast<size_t>(ConfigId::LED);
    auto& cfg = LED::GetConfig();
    if (cfg.changeCounter != g_lastSavedCounter[idx]) {
      uint32_t elapsed = now - cfg.lastModifiedMs;
      if (elapsed >= g_autoSaveDelayMs) {
        if (SaveConfig(ConfigId::LED)) {
          g_lastSavedCounter[idx] = cfg.changeCounter;
          //g_lastSavedMs[idx] = cfg.lastModifiedMs;
          savedAny = true;
          if (DEBUG_SERIAL) {
            Serial.printf("> Saved LED Config! changeCounter: %d\n", cfg.changeCounter);
          }
        } else {
          if (DEBUG_SERIAL) {
            Serial.print("Unable to save LED Config!\n\n");
          }
        }
      }
    }
  }

  return savedAny;
}


/**
 * Initialize settings subsystem and attempt to load persisted configs.
 * If DEBUG_SERIAL is defined, prints load status and basic info to Serial.
 *
 * Call this early in setup(), after Serial.begin() (or this function will
 * start Serial if not already started).
 */
inline void InitAndLoadReport() {
  // Ensure Preferences subsystem (no-op) and internal state initialised.
  Init();

  if (DEBUG_SERIAL) {

    if (!Serial) {
      Serial.begin(115200);
      delay(100);
    }
    Serial.println(F("SETTINGS: InitAndLoadReport() starting..."));
  }


  // --- Load LED config ---
  {
    const ConfigId id = ConfigId::LED;
    bool ok = LoadConfig(id);

    if (DEBUG_SERIAL) {
      Serial.print(F("SETTINGS: LED config "));
      Serial.println(ok ? F("loaded from prefs") : F("not found; using defaults"));
      auto& c = LED::GetConfig();
      Serial.print(F("  size(bytes): "));
      Serial.println(sizeof(c));
      Serial.print(F("  changeCounter: "));
      Serial.println(c.changeCounter);
      Serial.print(F("  lastModifiedMs: "));
      Serial.println(c.lastModifiedMs);
    }
  }

  if (DEBUG_SERIAL)  Serial.println(F("SETTINGS: InitAndLoadReport() done.\n"));
}




/**
 * Force immediate save of both singletons.
 */
inline bool SaveNow() {
  bool ok = SaveConfig(ConfigId::LED);
  if (ok) {
    g_lastSavedCounter[static_cast<size_t>(ConfigId::LED)] = LED::GetConfig().changeCounter;
  }
  return ok;
}

}  // namespace SETTINGS
