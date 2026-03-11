#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "helpers.h"

void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = "";
  unsigned long reqStart = millis();

  while (client.connected() && (millis() - reqStart < 1000)) {
    while (client.available()) {
      char c = client.read();
      request += c;

      if (request.endsWith("\r\n\r\n")) {

        if (request.indexOf("GET /settimer?") >= 0) {
          String dev = getQueryParam(request, "dev");
          String secStr = getQueryParam(request, "sec");

          unsigned long sec = (unsigned long) secStr.toInt();
          sec = clampUL(sec, 1UL, 86400UL);

          if (dev.length() == 0 || secStr.length() == 0) {
            sendBadRequest(client, "Missing dev or sec");
            client.stop();
            return;
          }

          if (!setTimedDeviceByName(dev, sec)) {
            sendBadRequest(client, "Invalid device");
            client.stop();
            return;
          }

          sendPlain(client, "OK");
          client.stop();
          return;
        }

        if (request.indexOf("GET /light/on") >= 0)     { setLightManual(true); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/off") >= 0)    { setLightManual(false); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/veg") >= 0)    { applyLightMode(VEG); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/flower") >= 0) { applyLightMode(FLOWER); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/custom") >= 0) { applyLightMode(CUSTOM); sendRedirect(client); client.stop(); return; }

        if (request.indexOf("GET /water/on") >= 0) {
          startTimedOutput(CH_WATER, waterTimer, 0);
          waterLatchedOn = false;
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /water/off") >= 0) {
          stopWaterLatchedOff();
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /pump/on") >= 0) {
          startTimedOutput(CH_PUMP, pumpTimer, 0);
          armPumpAutoStop(millis());
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /pump/off") >= 0) {
          stopTimedOutput(CH_PUMP, pumpTimer);
          disarmPumpAutoStop();
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /spray/on") >= 0) {
          startTimedOutput(CH_SPRAY, sprayTimer, 0);
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /spray/off") >= 0) {
          stopTimedOutput(CH_SPRAY, sprayTimer);
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /aux1/on") >= 0) {
          startTimedOutput(CH_AUX1, aux1Timer, 0);
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /aux1/off") >= 0) {
          stopTimedOutput(CH_AUX1, aux1Timer);
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /aux2/on") >= 0) {
          startTimedOutput(CH_AUX2, aux2Timer, 0);
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /aux2/off") >= 0) {
          stopTimedOutput(CH_AUX2, aux2Timer);
          sendRedirect(client);
          client.stop();
          return;
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html; charset=UTF-8");
        client.println("Connection: close");
        client.println();
        client.println("<html><body><h1>Web UI moved here</h1></body></html>");

        client.stop();
        return;
      }
    }
  }

  client.stop();
}

#endif