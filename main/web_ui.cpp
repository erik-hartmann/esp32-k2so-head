#include "web_ui.h"

#include <Arduino.h>
#include <Bluepad32.h>
#include <WebServer.h>
#include <WiFi.h>

#include "led_controller.h"
#include "light_effects.h"

namespace {

// Change these if you want a different network name/password. WPA2
// requires at least 8 characters.
constexpr const char* kApSsid = "ESP32-LEDs";
constexpr const char* kApPassword = "ledparty1";

WebServer sServer(80);
bool sRunning = false;

uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0;
}

String buildIndexHtml() {
    String html;
    html.reserve(3072);
    html += R"HTML(<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 LEDs</title><style>
body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:16px;}
h1{font-size:1.2rem;} h2{font-size:1rem;color:#aaa;margin-top:1.5rem;}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;}
button{padding:12px;border:none;border-radius:8px;background:#333;color:#fff;font-size:0.95rem;}
input[type=range]{width:100%;} input[type=color]{width:100%;height:44px;border:none;padding:0;}
#status{color:#8f8;min-height:1.2em;margin-bottom:0.5rem;}
</style></head><body>
<h1>ESP32 LED Control</h1>
<div id="status">Loading...</div>
<h2>Patterns</h2>
<div class="grid" id="patterns">)HTML";

    size_t count = LightEffects::effectCount();
    for (size_t i = 0; i < count; i++) {
        html += "<button onclick=\"selectEffect(";
        html += String(i);
        html += ")\">";
        html += LightEffects::effectName(static_cast<uint8_t>(i));
        html += "</button>";
    }

    html += R"HTML(</div>
<h2>Accent Color</h2>
<p style="color:#888;font-size:0.85rem;margin-top:-0.5rem">Applies to whichever pattern is running (except the rainbow ones).</p>
<input type="color" id="colorPicker" value="#ffa028" onchange="setColor(this.value)">
<h2>Brightness</h2>
<input type="range" id="brightness" min="0" max="255" value="200" onchange="setBrightness(this.value)">
<script>
function selectEffect(i){fetch('/api/effect?index='+i).then(refreshStatus);}
function setColor(hex){fetch('/api/color?hex='+hex.substring(1)).then(refreshStatus);}
function setBrightness(v){fetch('/api/brightness?value='+v).then(refreshStatus);}
function refreshStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('status').textContent = 'Current: ' + d.effect + ' — Brightness: ' + d.brightness;
  });
}
refreshStatus();
setInterval(refreshStatus, 3000);
</script>
</body></html>)HTML";
    return html;
}

void handleRoot() {
    sServer.send(200, "text/html", buildIndexHtml());
}

void handleSelectEffect() {
    if (!sServer.hasArg("index")) {
        sServer.send(400, "text/plain", "missing index");
        return;
    }
    int index = sServer.arg("index").toInt();
    if (index < 0 || index >= static_cast<int>(LightEffects::effectCount())) {
        sServer.send(400, "text/plain", "bad index");
        return;
    }
    LightEffects::selectEffect(static_cast<uint8_t>(index));
    sServer.send(200, "text/plain", "OK");
}

void handleSetColor() {
    String hex = sServer.arg("hex");
    if (hex.length() != 6) {
        sServer.send(400, "text/plain", "expected 6 hex digits, e.g. ff8800");
        return;
    }
    uint8_t r = static_cast<uint8_t>((hexNibble(hex[0]) << 4) | hexNibble(hex[1]));
    uint8_t g = static_cast<uint8_t>((hexNibble(hex[2]) << 4) | hexNibble(hex[3]));
    uint8_t b = static_cast<uint8_t>((hexNibble(hex[4]) << 4) | hexNibble(hex[5]));
    LightEffects::setAccentColor(r, g, b);
    sServer.send(200, "text/plain", "OK");
}

void handleSetBrightness() {
    if (!sServer.hasArg("value")) {
        sServer.send(400, "text/plain", "missing value");
        return;
    }
    int value = sServer.arg("value").toInt();
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    LedController::setBrightness(static_cast<uint8_t>(value));
    sServer.send(200, "text/plain", "OK");
}

void handleStatus() {
    String json = "{\"effect\":\"";
    json += LightEffects::currentName();
    json += "\",\"brightness\":";
    json += String(LedController::brightness());
    json += "}";
    sServer.send(200, "application/json", json);
}

}  // namespace

void WebUI::begin() {
    sServer.on("/", handleRoot);
    sServer.on("/api/effect", handleSelectEffect);
    sServer.on("/api/color", handleSetColor);
    sServer.on("/api/brightness", handleSetBrightness);
    sServer.on("/api/status", handleStatus);

    // Make sure the radio really is down rather than relying on it never
    // having been brought up.
    WiFi.mode(WIFI_OFF);
    Console.println("Web UI: routes registered, WiFi off (hold the board's admin button to toggle)");
}

void WebUI::start() {
    if (sRunning) {
        return;
    }
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApSsid, kApPassword);
    sServer.begin();
    sRunning = true;
    Console.printf("Web UI up: join WiFi \"%s\" (password \"%s\"), then browse to http://%s/\n", kApSsid,
                    kApPassword, WiFi.softAPIP().toString().c_str());
}

void WebUI::stop() {
    if (!sRunning) {
        return;
    }
    sServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    sRunning = false;
    Console.println("Web UI down: WiFi radio off");
}

bool WebUI::isRunning() {
    return sRunning;
}

void WebUI::handleClient() {
    if (sRunning) {
        sServer.handleClient();
    }
}
