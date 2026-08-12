#include "web_ui.h"

#include <Arduino.h>
#include <Bluepad32.h>
#include <WebServer.h>
#include <WiFi.h>

#include "board_config.h"
#include "led_controller.h"
#include "light_effects.h"

#ifdef AUDIO_TX_GPIO
#include "audio_player.h"
#endif

#ifdef I2C_SDA_GPIO
#include "eye_motion.h"
#endif

namespace {

// Change these if you want a different network name/password. WPA2
// requires at least 8 characters.
constexpr const char* kApSsid = "k2so-esp32";
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

    html += "</div>";

#ifdef AUDIO_TX_GPIO
    // Tracks are numbered rather than named: the firmware knows how many
    // /mp3/NNNN.mp3 files exist but nothing about what is in them, since the
    // module never reports back. TRACKS.txt on the card is the key.
    html += R"HTML(<h2>Audio</h2>
<p style="color:#888;font-size:0.85rem;margin-top:-0.5rem">Track numbers match /mp3/NNNN.mp3 on the card; see TRACKS.txt.</p>
<div class="grid" id="tracks">)HTML";
    uint16_t tracks = AudioPlayer::trackCount();
    for (uint16_t t = 1; t <= tracks; t++) {
        html += "<button onclick=\"playTrack(";
        html += String(t);
        html += ")\">";
        html += String(t);
        html += "</button>";
    }
    html += R"HTML(</div>
<h2>Volume</h2>
<input type="range" id="volume" min="0" max="30" value=")HTML";
    html += String(AudioPlayer::volume());
    html += R"HTML(" onchange="setVolume(this.value)">
)HTML";
#endif

#ifdef I2C_SDA_GPIO
    // Calibration, not a control you use day to day. Changes take effect
    // immediately but are not persisted -- once a value looks right it needs
    // copying into EYE_*_TRIM_DEG in the board config to survive a reboot.
    html += R"HTML(<h2>Eye Centring Trim</h2>
<p style="color:#888;font-size:0.85rem;margin-top:-0.5rem">Degrees. Live, but not saved &mdash; copy the final value into the board config.</p>
<label style="font-size:0.85rem;color:#aaa">Pan <span id="panVal"></span></label>
<input type="range" id="panTrim" min="-45" max="45" step="1" value=")HTML";
    html += String(static_cast<int>(EyeMotion::panTrim()));
    html += R"HTML(" oninput="setTrim('pan',this.value)">
<label style="font-size:0.85rem;color:#aaa">Tilt <span id="tiltVal"></span></label>
<input type="range" id="tiltTrim" min="-45" max="45" step="1" value=")HTML";
    html += String(static_cast<int>(EyeMotion::tiltTrim()));
    html += R"HTML(" oninput="setTrim('tilt',this.value)">
)HTML";
#endif

    html += R"HTML(<h2>Accent Color</h2>
<p style="color:#888;font-size:0.85rem;margin-top:-0.5rem">Applies to whichever pattern is running (except the rainbow ones).</p>
<input type="color" id="colorPicker" value="#ffa028" onchange="setColor(this.value)">
<h2>Brightness</h2>
<input type="range" id="brightness" min="0" max="255" value="200" onchange="setBrightness(this.value)">
<script>
function selectEffect(i){fetch('/api/effect?index='+i).then(refreshStatus);}
function setColor(hex){fetch('/api/color?hex='+hex.substring(1)).then(refreshStatus);}
function setBrightness(v){fetch('/api/brightness?value='+v).then(refreshStatus);}
function playTrack(t){fetch('/api/play?track='+t).then(refreshStatus);}
function setVolume(v){fetch('/api/volume?value='+v).then(refreshStatus);}
function setTrim(axis,v){
  // Labelled from the slider directly rather than waiting for the status
  // poll, so dragging feels immediate while calibrating.
  var l=document.getElementById(axis+'Val'); if(l){l.textContent=v+'°';}
  fetch('/api/eyetrim?'+axis+'='+v);
}
function refreshStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    var s = 'Current: ' + d.effect + ' — Brightness: ' + d.brightness;
    if (d.tracks > 0) {
      s += ' — Volume: ' + d.volume;
      s += d.track > 0 ? ' — Track ' + d.track : ' — no track played yet';
    }
    document.getElementById('status').textContent = s;
    // Reflect changes made on the gamepad, so the two controls agree rather
    // than each showing its own last action.
    var vol = document.getElementById('volume');
    if (vol && document.activeElement !== vol) { vol.value = d.volume; }
  });
}
['pan','tilt'].forEach(function(a){
  var s=document.getElementById(a+'Trim'), l=document.getElementById(a+'Val');
  if(s&&l){l.textContent=s.value+'°';}
});
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

#ifdef AUDIO_TX_GPIO
void handlePlay() {
    if (!sServer.hasArg("track")) {
        sServer.send(400, "text/plain", "missing track");
        return;
    }
    int track = sServer.arg("track").toInt();
    if (track < 1 || track > static_cast<int>(AudioPlayer::trackCount())) {
        sServer.send(400, "text/plain", "track out of range");
        return;
    }
    AudioPlayer::play(static_cast<uint16_t>(track));
    sServer.send(200, "text/plain", "OK");
}

void handleSetVolume() {
    if (!sServer.hasArg("value")) {
        sServer.send(400, "text/plain", "missing value");
        return;
    }
    int value = sServer.arg("value").toInt();
    if (value < 0) value = 0;
    if (value > 30) value = 30;
    AudioPlayer::setVolume(static_cast<uint8_t>(value));
    sServer.send(200, "text/plain", "OK");
}
#endif

#ifdef I2C_SDA_GPIO
void handleEyeTrim() {
    if (sServer.hasArg("pan")) {
        EyeMotion::setPanTrim(sServer.arg("pan").toFloat());
    }
    if (sServer.hasArg("tilt")) {
        EyeMotion::setTiltTrim(sServer.arg("tilt").toFloat());
    }
    // Echo what was actually applied, since the setters clamp.
    String json = "{\"pan\":";
    json += String(EyeMotion::panTrim(), 1);
    json += ",\"tilt\":";
    json += String(EyeMotion::tiltTrim(), 1);
    json += "}";
    sServer.send(200, "application/json", json);
}
#endif

void handleStatus() {
    String json = "{\"effect\":\"";
    json += LightEffects::currentName();
    json += "\",\"brightness\":";
    json += String(LedController::brightness());
#ifdef AUDIO_TX_GPIO
    json += ",\"volume\":";
    json += String(AudioPlayer::volume());
    json += ",\"track\":";
    json += String(AudioPlayer::currentTrack());
    json += ",\"tracks\":";
    json += String(AudioPlayer::trackCount());
#else
    // Always present so the page's JavaScript needs no board-specific
    // branching — it just sees zero tracks and hides the audio readout.
    json += ",\"volume\":0,\"track\":0,\"tracks\":0";
#endif
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
#ifdef AUDIO_TX_GPIO
    sServer.on("/api/play", handlePlay);
    sServer.on("/api/volume", handleSetVolume);
#endif
#ifdef I2C_SDA_GPIO
    sServer.on("/api/eyetrim", handleEyeTrim);
#endif

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
    if (!WiFi.softAP(kApSsid, kApPassword)) {
        // Leaving sRunning false matters beyond tidiness: the caller reports
        // success or failure to the user from isRunning(), so setting it
        // optimistically means a failed AP is indistinguishable from a
        // working one until you go looking for the network.
        WiFi.mode(WIFI_OFF);
        Console.println("Web UI: soft AP failed to start — radio back off");
        return;
    }
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
