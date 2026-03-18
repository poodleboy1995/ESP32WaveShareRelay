#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>

// ---------------- Waveshare ESP32-S3-Relay-6CH relay GPIO mapping ----------------
// CH1=GPIO1, CH2=GPIO2, CH3=GPIO41, CH4=GPIO42, CH5=GPIO45, CH6=GPIO46
// GPIO21 is BUZZER on this board (do not use for relays).
static const int RELAY_PINS[6] = { 1, 2, 41, 42, 45, 46 };

// Assign roles to channels
static const int CH_LIGHT = 0; // CH1
static const int CH_WATER = 1; // CH2
static const int CH_PUMP  = 2; // CH3
static const int CH_SPRAY = 3; // CH4
static const int CH_AUX1  = 4; // CH5
static const int CH_AUX2  = 5; // CH6

// ---------------- SENSOR CONFIG ----------------
static const int PUMP_LEVEL_PIN = 4;
static const bool LOW_LEVEL_IS_LOW = true;
static const unsigned long LEVEL_DEBOUNCE_MS = 800;
static const unsigned long PUMP_MAX_RUNTIME_MS = 20UL * 60UL * 1000UL; // 20 minutes

// ---------------- WATER LATCH CONTROL ----------------
static const int WATER_OFF_PIN = 5;   // dry contact to GND, INPUT_PULLUP
bool waterLatchedOn = false;

// ---------------- WATER MANUAL OFF OVERRIDE ----------------
static const int WATER_MANUAL_OFF_PIN = 40; // dry contact to GND, INPUT_PULLUP

// ---------------- WATER MANUAL ON OVERRIDE ----------------
static const int WATER_MANUAL_ON_PIN = 39; // dry contact to GND, INPUT_PULLUP

// WiFi credentials
const char* ssid = "Insert Here Test";
const char* password = "Insert Here";

// Web server
WiFiServer server(80);

// ---------------- Timing ----------------
unsigned long previousMillis = 0;
const unsigned long interval = 1000; // 1 second
const unsigned long HOUR = 3600000UL;
const unsigned long MINUTE = 60000UL;

// ---------------- Light auto schedule ----------------
enum LightMode { MANUAL, VEG, FLOWER, CUSTOM };
LightMode lightMode = MANUAL;

unsigned long lightOnDuration = 0;
unsigned long lightOffDuration = 0;

bool lightPhaseOn = false;       // true => ON phase, false => OFF phase
unsigned long lightPhaseTimer = 0;

// Custom schedule (default 18/6)
unsigned long customOnDuration = 18 * HOUR;
unsigned long customOffDuration = 6 * HOUR;

// ---------------- One-shot timers for channels ----------------
struct TimerControl {
  bool state = false;            // desired state bookkeeping
  unsigned long duration = 0;    // ms (0 = no timer)
  unsigned long elapsed = 0;     // ms
};

TimerControl waterTimer;
TimerControl pumpTimer;
TimerControl sprayTimer;
TimerControl aux1Timer;
TimerControl aux2Timer;

// ---------------- Pump auto-stop state ----------------
bool pumpAutoStopArmed = false;
unsigned long pumpArmedAtMs = 0;
unsigned long lowLevelSeenAtMs = 0;

#endif