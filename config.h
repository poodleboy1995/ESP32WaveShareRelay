#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>

// Relay mapping
static const int RELAY_PINS[6] = { 1, 2, 41, 42, 45, 46 };

// Channel roles
static const int CH_LIGHT = 0;
static const int CH_WATER = 1;
static const int CH_PUMP  = 2;
static const int CH_SPRAY = 3;
static const int CH_AUX1  = 4;
static const int CH_AUX2  = 5;

// Sensor config
static const int PUMP_LEVEL_PIN = 4;
static const bool LOW_LEVEL_IS_LOW = true;
static const unsigned long LEVEL_DEBOUNCE_MS = 800;
static const unsigned long PUMP_MAX_RUNTIME_MS = 20UL * 60UL * 1000UL;

// Water latch control
static const int WATER_OFF_PIN = 5;
extern bool waterLatchedOn;

// Water manual override
static const int WATER_MANUAL_OFF_PIN = 40;
static const int WATER_MANUAL_ON_PIN = 39;

// WiFi
const char* ssid = "Insert Here Test";
const char* password = "Insert Here";

// Web server
WiFiServer server(80);

// Timing
extern unsigned long previousMillis;
static const unsigned long interval = 1000;
static const unsigned long HOUR = 3600000UL;
static const unsigned long MINUTE = 60000UL;

// Light mode
enum LightMode { MANUAL, VEG, FLOWER, CUSTOM };
extern LightMode lightMode;
extern unsigned long lightOnDuration;
extern unsigned long lightOffDuration;
extern bool lightPhaseOn;
extern unsigned long lightPhaseTimer;
extern unsigned long customOnDuration;
extern unsigned long customOffDuration;

// Timer control
struct TimerControl {
  bool state = false;
  unsigned long duration = 0;
  unsigned long elapsed = 0;
};

extern TimerControl waterTimer;
extern TimerControl pumpTimer;
extern TimerControl sprayTimer;
extern TimerControl aux1Timer;
extern TimerControl aux2Timer;

// Pump state
extern bool pumpAutoStopArmed;
extern unsigned long pumpArmedAtMs;
extern unsigned long lowLevelSeenAtMs;

#endif