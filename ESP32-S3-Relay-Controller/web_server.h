#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "helpers.h"

static void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = "";
  unsigned long reqStart = millis();

  while (client.connected() && (millis() - reqStart < 1000)) {
    while (client.available()) {
      char c = client.read();
      request += c;

      // Wait until the full HTTP request header is received
      if (request.endsWith("\r\n\r\n")) {

        // ---- Timed device endpoint ----
        // /settimer?dev=water&sec=90
        if (request.indexOf("GET /settimer?") >= 0) {
          String dev = getQueryParam(request, "dev");
          String secStr = getQueryParam(request, "sec");

          unsigned long sec = (unsigned long) secStr.toInt();
          sec = clampUL(sec, 1UL, 86400UL); // up to 24 hours

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

        // ---- Light controls ----
        if (request.indexOf("GET /light/on") >= 0)     { setLightManual(true); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/off") >= 0)    { setLightManual(false); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/veg") >= 0)    { applyLightMode(VEG); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/flower") >= 0) { applyLightMode(FLOWER); sendRedirect(client); client.stop(); return; }
        if (request.indexOf("GET /light/custom") >= 0) { applyLightMode(CUSTOM); sendRedirect(client); client.stop(); return; }

        // Set custom ON hours via: /light/seton?h=#
        if (request.indexOf("GET /light/seton") >= 0) {
          int hPos = request.indexOf("h=");
          if (hPos >= 0) {
            int endPos = request.indexOf(' ', hPos);
            String hStr = request.substring(hPos + 2, endPos);
            int onHours = clampInt(hStr.toInt(), 1, 24);
            customOnDuration = (unsigned long)onHours * HOUR;
            int offHours = 24 - onHours;
            if (offHours < 0) offHours = 0;
            customOffDuration = (unsigned long)offHours * HOUR;

            if (lightMode == CUSTOM) {
              lightOnDuration = customOnDuration;
              lightOffDuration = customOffDuration;
              unsigned long phaseTotal = lightPhaseOn ? lightOnDuration : lightOffDuration;
              if (phaseTotal > 0 && lightPhaseTimer > phaseTotal) lightPhaseTimer = phaseTotal;
            }
          }
          sendRedirect(client);
          client.stop();
          return;
        }

        // ---- Water controls ----
        if (request.indexOf("GET /water/on") >= 0) {
          startTimedOutput(CH_WATER, waterTimer, 0);
          waterLatchedOn = false;
          sendRedirect(client);
          client.stop();
          return;
        }
        if (request.indexOf("GET /water/off") >= 0)  {
          stopWaterLatchedOff();
          sendRedirect(client);
          client.stop();
          return;
        }

        // ---- Pump controls ----
        if (request.indexOf("GET /pump/on") >= 0)   {
          startTimedOutput(CH_PUMP, pumpTimer, 0);
          armPumpAutoStop(millis());
          sendRedirect(client);
          client.stop();
          return;
        }
        if (request.indexOf("GET /pump/off") >= 0)  {
          stopTimedOutput(CH_PUMP, pumpTimer);
          disarmPumpAutoStop();
          sendRedirect(client);
          client.stop();
          return;
        }

        // ---- Spray controls ----
        if (request.indexOf("GET /spray/on") >= 0)   {
          startTimedOutput(CH_SPRAY, sprayTimer, 0);
          sendRedirect(client);
          client.stop();
          return;
        }
        if (request.indexOf("GET /spray/off") >= 0)  {
          stopTimedOutput(CH_SPRAY, sprayTimer);
          sendRedirect(client);
          client.stop();
          return;
        }

        // ---- AUX 1 / AUX 2 controls ----
        if (request.indexOf("GET /aux1/on") >= 0)   {
          startTimedOutput(CH_AUX1, aux1Timer, 0);
          sendRedirect(client);
          client.stop();
          return;
        }
        if (request.indexOf("GET /aux1/off") >= 0)  {
          stopTimedOutput(CH_AUX1, aux1Timer);
          sendRedirect(client);
          client.stop();
          return;
        }

        if (request.indexOf("GET /aux2/on") >= 0)   {
          startTimedOutput(CH_AUX2, aux2Timer, 0);
          sendRedirect(client);
          client.stop();
          return;
        }
        if (request.indexOf("GET /aux2/off") >= 0)  {
          stopTimedOutput(CH_AUX2, aux2Timer);
          sendRedirect(client);
          client.stop();
          return;
        }

        // ---- STATUS endpoint ----
        if (request.indexOf("GET /status") >= 0) {
          long lightRem = lightRemainingSeconds();
          long waterRem = remainingSeconds(waterTimer);
          long pumpRem  = remainingSeconds(pumpTimer);
          long sprayRem = remainingSeconds(sprayTimer);
          long aux1Rem  = remainingSeconds(aux1Timer);
          long aux2Rem  = remainingSeconds(aux2Timer);

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain; charset=UTF-8");
          client.println("Connection: close");
          client.println();
          client.print(lightRem); client.print(",");
          client.print(waterRem); client.print(",");
          client.print(pumpRem);  client.print(",");
          client.print(sprayRem); client.print(",");
          client.print(aux1Rem);  client.print(",");
          client.print(aux2Rem);

          client.stop();
          return;
        }

        // ---- STATE endpoint ----
        if (request.indexOf("GET /state") >= 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain; charset=UTF-8");
          client.println("Connection: close");
          client.println();
          client.print(relayRead(CH_LIGHT) ? 1 : 0); client.print(",");
          client.print(relayRead(CH_WATER) ? 1 : 0); client.print(",");
          client.print(relayRead(CH_PUMP)  ? 1 : 0); client.print(",");
          client.print(relayRead(CH_SPRAY) ? 1 : 0); client.print(",");
          client.print(relayRead(CH_AUX1)  ? 1 : 0); client.print(",");
          client.print(relayRead(CH_AUX2)  ? 1 : 0);

          client.stop();
          return;
        }

        // ---- LIGHT META endpoint ----
        if (request.indexOf("GET /lightmeta") >= 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain; charset=UTF-8");
          client.println("Connection: close");
          client.println();
          client.print(lightModeLabel());
          client.print(",");
          if (lightMode == MANUAL) client.print("MANUAL");
          else client.print(lightPhaseOn ? "ON" : "OFF");

          client.stop();
          return;
        }

        // ---- WATER META endpoint ----
        if (request.indexOf("GET /watermeta") >= 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain; charset=UTF-8");
          client.println("Connection: close");
          client.println();
          client.print(isWaterManualOffActive() ? "MANUAL_OFF_ACTIVE" : "MANUAL_OFF_CLEAR");
          client.print(",");
          client.print(isWaterManualOnActive() ? "MANUAL_ON_ACTIVE" : "MANUAL_ON_CLEAR");
          client.print(",");
          client.print(waterLatchedOn ? "LATCHED" : "NOT_LATCHED");

          client.stop();
          return;
        }

        // ---- SENSOR endpoint ----
        if (request.indexOf("GET /sensor") >= 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain; charset=UTF-8");
          client.println("Connection: close");
          client.println();
          client.print("raw=");
          client.print(digitalRead(PUMP_LEVEL_PIN));
          client.print(", low=");
          client.print(isLowLevelRaw() ? "1" : "0");
          client.print(", armed=");
          client.print(pumpAutoStopArmed ? "1" : "0");
          client.print(", waterLatched=");
          client.print(waterLatchedOn ? "1" : "0");
          client.print(", waterOffPin=");
          client.print(digitalRead(WATER_OFF_PIN));
          client.print(", waterManualOffPin=");
          client.print(digitalRead(WATER_MANUAL_OFF_PIN));
          client.print(", waterManualOnPin=");
          client.print(digitalRead(WATER_MANUAL_ON_PIN));

          client.stop();
          return;
        }

        // ---- MAIN PAGE ----
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html; charset=UTF-8");
        client.println("Connection: close");
        client.println();

        int currentOnHours = (int)(customOnDuration / HOUR);
        if (currentOnHours < 1) currentOnHours = 1;
        if (currentOnHours > 24) currentOnHours = 24;

        client.println(
          "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
          "<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
          "<title>ESP32-S3-Relay-6CH Panel</title><style>"
        );

        client.println("body{font-family:'Courier New',monospace;background:#1f1f1f;color:#f0f0f0;margin:0;padding:12px;text-align:center;}");
        client.println(".top{max-width:1100px;margin:0 auto 12px auto;display:flex;gap:10px;flex-wrap:wrap;justify-content:space-between;align-items:center;}");
        client.println(".badge{background:#151515;border:1px solid #333;padding:10px 12px;border-radius:10px;box-shadow:0 4px 6px rgba(0,0,0,0.25);}");
        client.println(".badge h2{margin:0;font-size:12px;color:#bbb;font-weight:900;letter-spacing:1px;}");
        client.println(".badge .big{font-size:22px;font-weight:900;margin-top:2px;}");
        client.println(".grid{max-width:1100px;margin:0 auto;display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:12px;}");
        client.println(".card{background:#2c2c2c;padding:15px;border-radius:12px;box-shadow:0 6px 10px rgba(0,0,0,0.35);border:1px solid #3a3a3a;}");
        client.println(".device-title{font-size:20px;font-weight:900;margin-bottom:8px;letter-spacing:1px;display:flex;justify-content:space-between;align-items:center;}");
        client.println(".chip{font-size:11px;font-weight:900;padding:4px 8px;border-radius:999px;border:1px solid #444;background:#141414;color:#ddd;letter-spacing:0.7px;}");
        client.println(".chip.auto{border-color:#3b5bdb;color:#dbe4ff;}");
        client.println(".chip.manual{border-color:#666;color:#ddd;}");
        client.println(".chip.alert{border-color:#c92a2a;color:#ffd8d8;background:#2b1111;}");
        client.println(".chip.onchip{border-color:#2f9e44;color:#d3f9d8;background:#102312;}");
        client.println(".row{display:flex;justify-content:space-between;align-items:center;gap:10px;margin-bottom:10px;}");
        client.println(".left{display:flex;align-items:center;gap:8px;font-weight:900;}");
        client.println(".led{width:14px;height:14px;border-radius:50%;display:inline-block;}");
        client.println(".led.on{background:#35d04b;box-shadow:0 0 10px rgba(53,208,75,0.55);}");
        client.println(".led.off{background:#ff3b30;box-shadow:0 0 10px rgba(255,59,48,0.35);}");
        client.println(".meta{font-size:12px;color:#cfcfcf;opacity:0.95;}");
        client.println(".mono{font-variant-numeric:tabular-nums;}");
        client.println(".btnRow{margin-top:10px;display:flex;flex-wrap:wrap;justify-content:center;gap:8px;}");
        client.println("button{padding:10px 18px;font-size:15px;border:none;border-radius:10px;cursor:pointer;font-weight:900;letter-spacing:0.5px;}");
        client.println(".onBtn{background:#2f9e44;color:white;}");
        client.println(".offBtn{background:#c92a2a;color:white;}");
        client.println(".modeBtn{background:#3b5bdb;color:white;}");
        client.println(".timerWrap{margin-top:12px;padding:10px;background:#191919;border:1px solid #3a3a3a;border-radius:10px;text-align:left;}");
        client.println(".timerWrap h3{margin:0 0 8px 0;font-size:14px;}");
        client.println(".small{font-size:12px;color:#cfcfcf;}");
        client.println(".inputRow{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-top:8px;}");
        client.println("input[type=range]{width:100%;}");
        client.println("input[type=number]{background:#101010;color:#fff;border:1px solid #444;border-radius:8px;padding:8px;width:78px;font-family:inherit;font-size:14px;}");
        client.println(".applyBtn{background:#3b5bdb;color:#fff;}");
        client.println("</style><script>");

        client.println("function pad2(n){return String(n).padStart(2,'0');}");
        client.println("function formatTime(sec){sec=parseInt(sec);if(isNaN(sec)||sec<=0)return '00:00:00';"
                       "let h=Math.floor(sec/3600),m=Math.floor((sec%3600)/60),s=sec%60;"
                       "return pad2(h)+':'+pad2(m)+':'+pad2(s);}");

        client.println("function updateClock(){const now=new Date();"
                       "document.getElementById('clock').textContent=now.toLocaleTimeString([], {hour:'2-digit',minute:'2-digit',second:'2-digit'});}");

        client.println("function setLedAndText(timerId, on){"
                       "const led=document.getElementById('led_'+timerId);"
                       "const txt=document.getElementById('txt_'+timerId);"
                       "if(!led||!txt)return;"
                       "led.className='led '+(on?'on':'off');"
                       "txt.textContent=on?'ON':'OFF';}");

        client.println("function updateState(){fetch('/state').then(r=>r.text()).then(t=>{let d=t.split(',');"
                       "setLedAndText('tLight', d[0]=='1');"
                       "setLedAndText('tWater', d[1]=='1');"
                       "setLedAndText('tPump', d[2]=='1');"
                       "setLedAndText('tSpray', d[3]=='1');"
                       "setLedAndText('tAux1', d[4]=='1');"
                       "setLedAndText('tAux2', d[5]=='1');"
                       "});}");

        client.println("function updateLightMeta(){fetch('/lightmeta').then(r=>r.text()).then(t=>{"
                       "let d=t.split(',');"
                       "let mode=d[0]||'MANUAL';"
                       "let phase=d[1]||'MANUAL';"
                       "const chip=document.getElementById('lightChip');"
                       "if(!chip)return;"
                       "if(mode==='MANUAL'){chip.textContent='MANUAL'; chip.className='chip manual';}"
                       "else{chip.textContent='AUTO • '+mode+' • '+phase; chip.className='chip auto';}"
                       "});}");

        client.println("function updateWaterMeta(){fetch('/watermeta').then(r=>r.text()).then(t=>{"
                       "let d=t.split(',');"
                       "let manualOff=d[0]||'MANUAL_OFF_CLEAR';"
                       "let manualOn=d[1]||'MANUAL_ON_CLEAR';"
                       "let latch=d[2]||'NOT_LATCHED';"
                       "const offChip=document.getElementById('waterManualChip');"
                       "const offTop=document.getElementById('waterManualTop');"
                       "const onChip=document.getElementById('waterManualOnChip');"
                       "const onTop=document.getElementById('waterManualOnTop');"
                       "const latchChip=document.getElementById('waterLatchChip');"
                       "if(offChip){"
                       " if(manualOff==='MANUAL_OFF_ACTIVE'){offChip.textContent='MANUAL OFF: ACTIVE'; offChip.className='chip alert';}"
                       " else{offChip.textContent='MANUAL OFF: CLEAR'; offChip.className='chip manual';}"
                       "}"
                       "if(offTop){"
                       " if(manualOff==='MANUAL_OFF_ACTIVE'){offTop.textContent='OFF ACTIVE';}"
                       " else{offTop.textContent='CLEAR';}"
                       "}"
                       "if(onChip){"
                       " if(manualOn==='MANUAL_ON_ACTIVE'){onChip.textContent='MANUAL ON: ACTIVE'; onChip.className='chip onchip';}"
                       " else{onChip.textContent='MANUAL ON: CLEAR'; onChip.className='chip manual';}"
                       "}"
                       "if(onTop){"
                       " if(manualOn==='MANUAL_ON_ACTIVE'){onTop.textContent='ON ACTIVE';}"
                       " else{onTop.textContent='CLEAR';}"
                       "}"
                       "if(latchChip){"
                       " if(latch==='LATCHED'){latchChip.textContent='LATCHED'; latchChip.className='chip auto';}"
                       " else{latchChip.textContent='NOT LATCHED'; latchChip.className='chip manual';}"
                       "}"
                       "});}");

        client.println("function updateTimers(){fetch('/status').then(r=>r.text()).then(t=>{let d=t.split(',');"
                       "document.getElementById('tLight').textContent=formatTime(d[0]);"
                       "document.getElementById('tWater').textContent=formatTime(d[1]);"
                       "document.getElementById('tPump').textContent=formatTime(d[2]);"
                       "document.getElementById('tSpray').textContent=formatTime(d[3]);"
                       "document.getElementById('tAux1').textContent=formatTime(d[4]);"
                       "document.getElementById('tAux2').textContent=formatTime(d[5]);"
                       "});}");

        client.println("function tick(){updateClock();updateTimers();updateState();updateLightMeta();updateWaterMeta();}"
                       "setInterval(tick,1000);window.onload=tick;");

        client.println("function setOnHours(v){document.getElementById('onHoursVal').textContent=v;"
                       "document.getElementById('offHoursVal').textContent=(24-parseInt(v));}");

        client.println("function applyOnHours(){const v=document.getElementById('onHours').value;"
                       "fetch('/light/seton?h='+v).then(()=>{});}");


        client.println("function updatePresetLabel(dev,v){"
                       "const el=document.getElementById(dev+'_preset_label');"
                       "if(el){el.textContent=v+' min';}"
                       "}");

        client.println("function applyPresetTimer(dev){"
                       "const slider=document.getElementById(dev+'_preset');"
                       "if(!slider)return;"
                       "const min=parseInt(slider.value)||1;"
                       "const sec=min*60;"
                       "fetch('/settimer?dev='+dev+'&sec='+sec).then(()=>{});"
                       "}");

        client.println("function applyManualTimer(dev){"
                       "const minEl=document.getElementById(dev+'_min');"
                       "const secEl=document.getElementById(dev+'_sec');"
                       "let min=parseInt(minEl.value)||0;"
                       "let sec=parseInt(secEl.value)||0;"
                       "if(min<0)min=0;"
                       "if(sec<0)sec=0;"
                       "if(sec>59)sec=59;"
                       "secEl.value=sec;"
                       "let total=(min*60)+sec;"
                       "if(total<=0){alert('Enter a time greater than 0 seconds');return;}"
                       "fetch('/settimer?dev='+dev+'&sec='+total).then(()=>{});"
                       "}");

        client.println("</script></head><body>");

        client.println("<div class='top'>");
        client.println("<div class='badge'><h2>LOCAL TIME</h2><div class='big mono' id='clock'>--:--:--</div></div>");
        client.println("<div class='badge'><h2>LIGHT MODE</h2><div class='big mono'>");
        client.println(lightModeLabel());
        client.println("</div></div>");
        client.println("<div class='badge'><h2>WATER MANUAL OFF</h2><div class='big mono' id='waterManualTop'>");
        client.println(isWaterManualOffActive() ? "OFF ACTIVE" : "CLEAR");
        client.println("</div></div>");
        client.println("<div class='badge'><h2>WATER MANUAL ON</h2><div class='big mono' id='waterManualOnTop'>");
        client.println(isWaterManualOnActive() ? "ON ACTIVE" : "CLEAR");
        client.println("</div></div>");
        client.println("</div>");

        client.println("<h1>ESP32-S3-Relay-6CH Control Panel</h1>");
        client.println("<div class='grid'>");

        auto printTimedControlBlock = [&](const char* dev) {
          client.println("<div class='timerWrap'>");
          client.println("<h3>Preset Timer</h3>");
          client.println(String("<div class='small mono'>Selected: <span id='") + dev + "_preset_label'>1 min</span></div>");
          client.println(String("<input id='") + dev + "_preset' type='range' min='1' max='20' step='1' value='1' oninput=\"updatePresetLabel('" + String(dev) + "', this.value)\">");
          client.println("<div class='btnRow' style='justify-content:flex-end;'>");
          client.println(String("<button class='applyBtn' onclick=\"applyPresetTimer('") + dev + "')\">APPLY PRESET</button>");
          client.println("</div>");

          client.println("<h3 style='margin-top:12px;'>Manual Time Entry</h3>");
          client.println("<div class='small'>Set any time using minutes and seconds</div>");
          client.println("<div class='inputRow'>");
          client.println(String("<input id='") + dev + "_min' type='number' min='0' value='0' placeholder='min'>");
          client.println(String("<input id='") + dev + "_sec' type='number' min='0' max='59' value='30' placeholder='sec'>");
          client.println(String("<button class='applyBtn' onclick=\"applyManualTimer('") + dev + "')\">APPLY CUSTOM</button>");
          client.println("</div>");
          client.println("</div>");
        };

        auto printCard = [&](const char* title, const char* emoji, const char* basePath, const char* devName, bool isOn, const char* timerId, bool showModes, bool isWaterCard) {
          client.println("<div class='card'>");

          if (showModes) {
            client.println(String("<div class='device-title'><span>") + emoji + " " + title + "</span><span class='chip auto' id='lightChip'>" + lightAutoBadge() + "</span></div>");
          } else if (isWaterCard) {
            client.println(String("<div class='device-title'><span>") + emoji + " " + title + "</span><span class='chip manual' id='waterManualChip'>MANUAL OFF: --</span></div>");
          } else {
            client.println(String("<div class='device-title'><span>") + emoji + " " + title + "</span></div>");
          }

          if (isWaterCard) {
            client.println("<div class='row'>");
            client.println("<div class='meta'><span class='chip manual' id='waterManualOnChip'>MANUAL ON: --</span></div>");
            client.println("<div class='meta'><span class='chip manual' id='waterLatchChip'>NOT LATCHED</span></div>");
            client.println("</div>");
          }

          client.println("<div class='row'>");
          client.println(String("<div class='left'><div class='led ") + (isOn ? "on" : "off") + "' id='led_" + timerId + "'></div><span id='txt_" + timerId + "'>" + (isOn ? "ON" : "OFF") + "</span></div>");
          client.println(String("<div class='meta mono'>Countdown: <span id='") + timerId + "'>00:00:00</span></div>");
          client.println("</div>");

          client.println("<div class='btnRow'>");
          client.println(String("<a href='") + basePath + "/on'><button class='onBtn'>ON</button></a>");
          client.println(String("<a href='") + basePath + "/off'><button class='offBtn'>OFF</button></a>");

          if (showModes) {
            client.println("<a href='/light/veg'><button class='modeBtn'>VEG</button></a>");
            client.println("<a href='/light/flower'><button class='modeBtn'>FLOWER</button></a>");
            client.println("<a href='/light/custom'><button class='modeBtn'>CUSTOM</button></a>");
          }
          client.println("</div>");

          if (showModes) {
            client.println("<div class='timerWrap'>");
            client.println("<div class='small'><b>Custom Schedule</b> (24-hour cycle)</div>");
            client.println(String("<div class='small mono'>ON: <span id='onHoursVal'>") + currentOnHours + "</span>h | OFF: <span id='offHoursVal'>" + (24 - currentOnHours) + "</span>h</div>");
            client.println(String("<input id='onHours' type='range' min='1' max='24' value='") + currentOnHours + "' oninput='setOnHours(this.value)'>");
            client.println("<div class='btnRow' style='justify-content:flex-end;'>");
            client.println("<button class='modeBtn' onclick='applyOnHours()'>APPLY</button>");
            client.println("</div></div>");
          } else if (devName != nullptr) {
            printTimedControlBlock(devName);
          }

          client.println("</div>");
        };

        printCard("LIGHT", "💡", "/light", nullptr, relayRead(CH_LIGHT), "tLight", true, false);
        printCard("WATER", "💧", "/water", "water", relayRead(CH_WATER), "tWater", false, true);
        printCard("PUMP", "⚡", "/pump", "pump", relayRead(CH_PUMP), "tPump", false, false);
        printCard("SPRAY", "🌊", "/spray", "spray", relayRead(CH_SPRAY), "tSpray", false, false);
        printCard("AUX 1", "🧰", "/aux1", "aux1", relayRead(CH_AUX1), "tAux1", false, false);
        printCard("AUX 2", "🧲", "/aux2", "aux2", relayRead(CH_AUX2), "tAux2", false, false);

        client.println("</div></body></html>");

        client.stop();
        return;
      }
    }
  }

  client.stop();
}

#endif