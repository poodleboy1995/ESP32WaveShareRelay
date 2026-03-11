#ifndef HELPERS_H
#define HELPERS_H

#include "config.h"

// If your relay logic is inverted on your board, flip these:
static inline void relayWrite(int ch, bool on) {
  digitalWrite(RELAY_PINS[ch], on ? HIGH : LOW);
}

static inline bool relayRead(int ch) {
  return digitalRead(RELAY_PINS[ch]) == HIGH;
}

static int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static unsigned long clampUL(unsigned long v, unsigned long lo, unsigned long hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static const char* lightModeLabel() {
  switch (lightMode) {
    case MANUAL: return "MANUAL";
    case VEG:    return "VEG (18/6)";
    case FLOWER: return "FLOWER (12/12)";
    case CUSTOM: return "CUSTOM";
    default:     return "UNKNOWN";
  }
}

static const char* lightAutoBadge() {
  switch (lightMode) {
    case VEG:    return "AUTO • VEG";
    case FLOWER: return "AUTO • FLOWER";
    case CUSTOM: return "AUTO • CUSTOM";
    default:     return "MANUAL";
  }
}

static void setLightManual(bool on) {
  lightMode = MANUAL;
  lightOnDuration = 0;
  lightOffDuration = 0;
  lightPhaseTimer = 0;
  lightPhaseOn = on;
  relayWrite(CH_LIGHT, on);
}

static void applyLightMode(LightMode mode) {
  lightMode = mode;
  if (mode == VEG) {
    lightOnDuration = 18 * HOUR;
    lightOffDuration = 6 * HOUR;
  } else if (mode == FLOWER) {
    lightOnDuration = 12 * HOUR;
    lightOffDuration = 12 * HOUR;
  } else if (mode == CUSTOM) {
    lightOnDuration = customOnDuration;
    lightOffDuration = customOffDuration;
  } else {
    return;
  }

  lightPhaseOn = true;
  lightPhaseTimer = 0;
  relayWrite(CH_LIGHT, true);
}

static void startTimedOutput(int ch, TimerControl &t, unsigned long durationMs) {
  relayWrite(ch, true);
  t.state = true;
  t.duration = durationMs; // 0 means "no timer, stays on"
  t.elapsed = 0;
}

static void stopTimedOutput(int ch, TimerControl &t) {
  relayWrite(ch, false);
  t.state = false;
  t.duration = 0;
  t.elapsed = 0;
}

static void tickTimer(int ch, TimerControl &t, unsigned long dtMs) {
  if (t.duration == 0) return;
  t.elapsed += dtMs;
  if (t.elapsed >= t.duration) {
    stopTimedOutput(ch, t);
  }
}

static long remainingSeconds(const TimerControl &t) {
  if (!(t.state && t.duration > 0)) return 0;
  long rem = (long)((t.duration - t.elapsed) / 1000);
  if (rem < 0) rem = 0;
  return rem;
}

static long lightRemainingSeconds() {
  if (lightMode == MANUAL) return 0;
  unsigned long phaseTotal = lightPhaseOn ? lightOnDuration : lightOffDuration;
  if (phaseTotal == 0) return 0;
  if (lightPhaseTimer >= phaseTotal) return 0;
  return (long)((phaseTotal - lightPhaseTimer) / 1000);
}

// --------- Water level sensor helpers ----------
static bool isLowLevelRaw() {
  int v = digitalRead(PUMP_LEVEL_PIN);
  if (LOW_LEVEL_IS_LOW) return (v == LOW);
  return (v == HIGH);
}

// Returns true only if low-level has been stable for LEVEL_DEBOUNCE_MS
static bool isLowLevelStable(unsigned long nowMs) {
  if (isLowLevelRaw()) {
    if (lowLevelSeenAtMs == 0) lowLevelSeenAtMs = nowMs;
    if (nowMs - lowLevelSeenAtMs >= LEVEL_DEBOUNCE_MS) return true;
  } else {
    lowLevelSeenAtMs = 0;
  }
  return false;
}

static void armPumpAutoStop(unsigned long nowMs) {
  pumpAutoStopArmed = true;
  pumpArmedAtMs = nowMs;
  lowLevelSeenAtMs = 0;
}

static void disarmPumpAutoStop() {
  pumpAutoStopArmed = false;
  pumpArmedAtMs = 0;
  lowLevelSeenAtMs = 0;
}

// --------- Water latch helpers ----------
static void startWaterLatchedOn() {
  startTimedOutput(CH_WATER, waterTimer, 0); // 0 = stays on
  waterLatchedOn = true;
}

static void stopWaterLatchedOff() {
  stopTimedOutput(CH_WATER, waterTimer);
  waterLatchedOn = false;
}

static void stopPumpBecauseLowLevel() {
  stopTimedOutput(CH_PUMP, pumpTimer);
  disarmPumpAutoStop();

  // Low level event triggers WATER ON (latched)
  startWaterLatchedOn();
}

static bool isWaterManualOffActive() {
  return digitalRead(WATER_MANUAL_OFF_PIN) == LOW;
}

static bool isWaterManualOnActive() {
  return digitalRead(WATER_MANUAL_ON_PIN) == LOW;
}

// --------- Query parsing helpers ----------
static String getQueryParam(const String &request, const String &key) {
  int qPos = request.indexOf('?');
  if (qPos < 0) return "";

  String token = key + "=";
  int start = request.indexOf(token, qPos);
  if (start < 0) return "";

  start += token.length();
  int endAmp = request.indexOf('&', start);
  int endSpace = request.indexOf(' ', start);

  int end = -1;
  if (endAmp >= 0 && endSpace >= 0) end = min(endAmp, endSpace);
  else if (endAmp >= 0) end = endAmp;
  else end = endSpace;

  if (end < 0) return "";
  return request.substring(start, end);
}

static void sendRedirect(WiFiClient &client, const char* location = "/") {
  client.println("HTTP/1.1 302 Found");
  client.print("Location: ");
  client.println(location);
  client.println("Connection: close");
  client.println();
}

static void sendPlain(WiFiClient &client, const String &body) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain; charset=UTF-8");
  client.println("Connection: close");
  client.println();
  client.print(body);
}

static void sendBadRequest(WiFiClient &client, const String &msg) {
  client.println("HTTP/1.1 400 Bad Request");
  client.println("Content-Type: text/plain; charset=UTF-8");
  client.println("Connection: close");
  client.println();
  client.print(msg);
}

static bool setTimedDeviceByName(const String &dev, unsigned long seconds) {
  unsigned long durationMs = seconds * 1000UL;

  if (dev == "water") {
    startTimedOutput(CH_WATER, waterTimer, durationMs);
    waterLatchedOn = false;
    return true;
  }
  if (dev == "pump") {
    startTimedOutput(CH_PUMP, pumpTimer, durationMs);
    armPumpAutoStop(millis());
    return true;
  }
  if (dev == "spray") {
    startTimedOutput(CH_SPRAY, sprayTimer, durationMs);
    return true;
  }
  if (dev == "aux1") {
    startTimedOutput(CH_AUX1, aux1Timer, durationMs);
    return true;
  }
  if (dev == "aux2") {
    startTimedOutput(CH_AUX2, aux2Timer, durationMs);
    return true;
  }
  return false;
}

#endif