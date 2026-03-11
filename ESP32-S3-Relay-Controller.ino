#include <WiFi.h>
#include "config.h"
#include "helpers.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 6; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  pinMode(PUMP_LEVEL_PIN, INPUT_PULLUP);
  pinMode(WATER_OFF_PIN, INPUT_PULLUP);
  pinMode(WATER_MANUAL_OFF_PIN, INPUT_PULLUP);
  pinMode(WATER_MANUAL_ON_PIN, INPUT_PULLUP);

  delay(500);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  unsigned long now = millis();

  if (now - previousMillis >= interval) {
    previousMillis = now;

    if (lightMode != MANUAL) {
      lightPhaseTimer += interval;
      unsigned long phaseTotal = lightPhaseOn ? lightOnDuration : lightOffDuration;
      if (phaseTotal > 0 && lightPhaseTimer >= phaseTotal) {
        lightPhaseOn = !lightPhaseOn;
        lightPhaseTimer = 0;
        relayWrite(CH_LIGHT, lightPhaseOn);
      }
    }

    tickTimer(CH_WATER, waterTimer, interval);
    tickTimer(CH_PUMP,  pumpTimer,  interval);
    tickTimer(CH_SPRAY, sprayTimer, interval);
    tickTimer(CH_AUX1,  aux1Timer,  interval);
    tickTimer(CH_AUX2,  aux2Timer,  interval);
  }

  if (pumpAutoStopArmed && relayRead(CH_PUMP)) {
    if (isLowLevelStable(now)) {
      Serial.println("Low level reached -> stopping pump + latching WATER ON");
      stopPumpBecauseLowLevel();
    }

    if (pumpArmedAtMs > 0 && (now - pumpArmedAtMs) >= PUMP_MAX_RUNTIME_MS) {
      Serial.println("Pump max runtime reached -> stopping pump (failsafe)");
      stopPumpBecauseLowLevel();
    }
  } else {
    lowLevelSeenAtMs = 0;
  }

  if (waterLatchedOn) {
    if (digitalRead(WATER_OFF_PIN) == LOW) {
      stopWaterLatchedOff();
    }
  }

  if (isWaterManualOffActive()) {
    relayWrite(CH_WATER, false);
  } else if (isWaterManualOnActive()) {
    relayWrite(CH_WATER, true);
  } else {
    relayWrite(CH_WATER, waterTimer.state);
  }

  handleWebServer();
}