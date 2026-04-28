#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <HTTPUpdate.h>
#include <Preferences.h>

#include "config.h"
#include "patterns.h"

const char* FW_VERSION = "1.0.0";
const char* OTA_FEED = "translight-ota";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Preferences prefs;

char deviceId[32] = "";

bool isShowingReceived = false;
bool blinkVisible = true;
unsigned long lastBlinkAt = 0;
const unsigned long BLINK_INTERVAL_MS = 800;

int selectedIndex = 0;
int currentReceivedIndex = -1;

unsigned long receivedAt = 0;
unsigned long displayTtlMs = 30UL * 60UL * 1000UL;

bool isSideA() {
  return String(deviceId) == "masa";
}

String outFeed() {
  return isSideA() ? "translight-m-to-h" : "translight-h-to-m";
}

String inFeed() {
  return isSideA() ? "translight-h-to-m" : "translight-m-to-h";
}

String outTopic() {
  return String(IO_USERNAME) + "/feeds/" + outFeed();
}

String inTopic() {
  return String(IO_USERNAME) + "/feeds/" + inFeed();
}

// ===== 表示 =====
void drawPreset(int index) {
  for (int i = 0; i < 25; i++) {
    M5.dis.drawpix(i, presets[index].pixels[i]);
  }
}

void drawPresetShifted(int index, int yOffset) {
  M5.dis.clear();

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      int srcY = y - yOffset;

      if (srcY < 0 || srcY >= 5) {
        continue;
      }

      int srcIndex = srcY * 5 + x;
      int dstIndex = y * 5 + x;

      uint32_t color = presets[index].pixels[srcIndex];

      if (color != OFF) {
        M5.dis.drawpix(dstIndex, color);
      }
    }
  }
}

void flashColor(uint32_t color, int count) {
  for (int i = 0; i < count; i++) {
    M5.dis.fillpix(color);
    delay(120);
    M5.dis.clear();
    delay(80);
  }
}

void playSendAnimation(int index) {
  for (int offset = 0; offset >= -5; offset--) {
    drawPresetShifted(index, offset);
    delay(120);
  }
}

void playReceiveAnimation(int index) {
  for (int offset = -5; offset <= 0; offset++) {
    drawPresetShifted(index, offset);
    delay(120);
  }
}

void playSecretAnimation(int index) {
  uint32_t colors[] = {
    RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK
  };

  for (int step = 0; step < 24; step++) {
    uint32_t color = colors[step % 8];

    for (int i = 0; i < 25; i++) {
      if (presets[index].pixels[i] != OFF) {
        M5.dis.drawpix(i, color);
      } else {
        M5.dis.drawpix(i, OFF);
      }
    }

    delay(80);
  }

  drawPreset(index);
}

// ===== payload =====
int findPresetIndex(String value) {
  value.trim();

  for (int i = 0; i < PRESET_COUNT; i++) {
    if (value == presets[i].id) {
      return i;
    }
  }

  return -1;
}

String extractValue(String payload) {
  payload.trim();

  if (!payload.startsWith("{")) {
    return payload;
  }

  int keyIndex = payload.indexOf("\"value\"");
  if (keyIndex < 0) {
    return "";
  }

  int colonIndex = payload.indexOf(":", keyIndex);
  if (colonIndex < 0) {
    return "";
  }

  int firstQuote = payload.indexOf("\"", colonIndex + 1);
  if (firstQuote < 0) {
    return "";
  }

  int secondQuote = payload.indexOf("\"", firstQuote + 1);
  if (secondQuote < 0) {
    return "";
  }

  return payload.substring(firstQuote + 1, secondQuote);
}

unsigned long extractTtlMs(String payload) {
  int keyIndex = payload.indexOf("\"ttlSec\"");
  if (keyIndex < 0) {
    return 30UL * 60UL * 1000UL;
  }

  int colonIndex = payload.indexOf(":", keyIndex);
  if (colonIndex < 0) {
    return 30UL * 60UL * 1000UL;
  }

  int endIndex = payload.indexOf("}", colonIndex);
  if (endIndex < 0) {
    endIndex = payload.length();
  }

  String ttlText = payload.substring(colonIndex + 1, endIndex);
  ttlText.trim();

  int ttlSec = ttlText.toInt();

  if (ttlSec <= 0) {
    ttlSec = 1800;
  }

  return (unsigned long)ttlSec * 1000UL;
}

bool sendPresetByIndex(int index) {
  String payload = String("{\"type\":\"preset\",\"value\":\"") +
                   presets[index].id +
                   "\",\"ttlSec\":1800}";

  return mqtt.publish(outTopic().c_str(), payload.c_str());
}

bool sendSecretAlien() {
  String payload = "{\"type\":\"easter_egg\",\"value\":\"secret_alien\",\"ttlSec\":60}";
  return mqtt.publish(outTopic().c_str(), payload.c_str());
}

// ===== OTA =====
String otaTopic() {
  return String(IO_USERNAME) + "/feeds/" + OTA_FEED;
}

void performOta(String url) {
  url.trim();
  if (url.length() == 0) return;

  M5.dis.fillpix(CYAN);

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPUpdate httpUpdate;
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  t_httpUpdate_return ret = httpUpdate.update(secureClient, url);

  switch (ret) {
    case HTTP_UPDATE_OK:
      flashColor(GREEN, 5);
      ESP.restart();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      flashColor(YELLOW, 3);
      break;
    default:
      flashColor(RED, 5);
      break;
  }

  drawPreset(selectedIndex);
}

// ===== MQTT受信 =====
void onMessage(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == otaTopic()) {
    String url = extractValue(message);
    if (url.length() == 0) url = message;
    performOta(url);
    return;
  }

  String value = extractValue(message);
  int presetIndex = findPresetIndex(value);

  if (presetIndex < 0) {
    flashColor(RED, 2);
    drawPreset(selectedIndex);
    return;
  }

  currentReceivedIndex = presetIndex;
  receivedAt = millis();
  displayTtlMs = extractTtlMs(message);

  isShowingReceived = true;
  blinkVisible = true;
  lastBlinkAt = millis();

  if (value == "secret_alien") {
    playSecretAnimation(presetIndex);
  } else {
    playReceiveAnimation(presetIndex);
  }

  drawPreset(presetIndex);
}

// ===== Wi-Fi / MQTT =====
void loadDeviceId() {
  prefs.begin("translight", true);
  String id = prefs.getString("deviceId", "");
  prefs.end();

  id.toCharArray(deviceId, sizeof(deviceId));
}

void saveDeviceId(const char* id) {
  strncpy(deviceId, id, sizeof(deviceId) - 1);

  prefs.begin("translight", false);
  prefs.putString("deviceId", id);
  prefs.end();
}

void drawName(const char* name) {
  M5.dis.clear();
  if (String(name) == "masa") {
    uint32_t px[25] = {
      GREEN, OFF,   OFF,   OFF,   GREEN,
      GREEN, GREEN, OFF,   GREEN, GREEN,
      GREEN, OFF,   GREEN, OFF,   GREEN,
      GREEN, OFF,   OFF,   OFF,   GREEN,
      GREEN, OFF,   OFF,   OFF,   GREEN
    };
    for (int i = 0; i < 25; i++) M5.dis.drawpix(i, px[i]);
  } else {
    uint32_t px[25] = {
      BLUE, OFF,  OFF,  OFF,  BLUE,
      BLUE, OFF,  OFF,  OFF,  BLUE,
      BLUE, BLUE, BLUE, BLUE, BLUE,
      BLUE, OFF,  OFF,  OFF,  BLUE,
      BLUE, OFF,  OFF,  OFF,  BLUE
    };
    for (int i = 0; i < 25; i++) M5.dis.drawpix(i, px[i]);
  }
}

void selectDevice() {
  loadDeviceId();
  if (strlen(deviceId) > 0) return;

  const char* names[] = { "masa", "haru" };
  int current = 0;

  drawName(names[current]);

  while (true) {
    M5.update();

    if (M5.Btn.wasPressed()) {
      unsigned long pressedAt = millis();

      while (!M5.Btn.wasReleased()) {
        M5.update();
        delay(10);
      }

      unsigned long duration = millis() - pressedAt;

      if (duration < 1000) {
        current = 1 - current;
        drawName(names[current]);
      } else {
        saveDeviceId(names[current]);
        flashColor(GREEN, 3);
        return;
      }
    }

    delay(20);
  }
}

void drawWifiIcon() {
  const uint32_t W = BLUE;
  const uint32_t _ = OFF;
  uint32_t icon[25] = {
    _, W, W, W, _,
    W, _, _, _, W,
    _, _, W, _, _,
    _, W, _, W, _,
    _, _, W, _, _
  };
  for (int i = 0; i < 25; i++) M5.dis.drawpix(i, icon[i]);
}

void connectWiFi() {
  drawWifiIcon();

  WiFiManager wm;

  M5.update();
  if (M5.Btn.isPressed()) {
    M5.dis.fillpix(PURPLE);
    delay(1000);
    wm.resetSettings();
    prefs.begin("translight", false);
    prefs.clear();
    prefs.end();
    flashColor(PURPLE, 2);
    ESP.restart();
  }

  String apName = String("Translight-") + deviceId;
  bool ok = wm.autoConnect(apName.c_str());

  if (ok) {
    flashColor(GREEN, 1);
  } else {
    flashColor(RED, 3);
    ESP.restart();
  }
}

void connectMqtt() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(onMessage);

  while (!mqtt.connected()) {
    M5.dis.fillpix(WHITE);

    String clientId = "atom-friend-light-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), IO_USERNAME, IO_KEY)) {
      mqtt.subscribe(inTopic().c_str());
      mqtt.subscribe(otaTopic().c_str());
      flashColor(GREEN, 1);
    } else {
      flashColor(RED, 2);
      delay(1000);
    }
  }
}

// ===== ボタン =====
void selectNextPreset() {
  selectedIndex++;

  if (selectedIndex >= PRESET_COUNT - 1) {
    selectedIndex = 0;
  }

  currentReceivedIndex = -1;
  isShowingReceived = false;
  blinkVisible = true;

  drawPreset(selectedIndex);
}

void sendSelectedPreset() {
  int sendingIndex = selectedIndex;

  playSendAnimation(sendingIndex);

  bool ok = sendPresetByIndex(sendingIndex);

  if (ok) {
    flashColor(GREEN, 2);
  } else {
    flashColor(RED, 3);
  }

  drawPreset(selectedIndex);
}

void sendEasterEggIfPossible() {
  bool isAlien = String(presets[selectedIndex].id) == "alien";

  bool ok = false;

  if (isAlien) {
    playSecretAnimation(selectedIndex);
    ok = sendSecretAlien();
  } else {
    playSendAnimation(selectedIndex);
    ok = sendPresetByIndex(selectedIndex);
  }

  if (ok) {
    flashColor(GREEN, 2);
  } else {
    flashColor(RED, 3);
  }

  drawPreset(selectedIndex);
}

void handleButton() {
  static unsigned long pressedAt = 0;
  static bool isPressing = false;

  if (M5.Btn.wasPressed()) {
    pressedAt = millis();
    isPressing = true;
  }

  if (M5.Btn.wasReleased() && isPressing) {
    unsigned long duration = millis() - pressedAt;

    if (duration < 1000) {
      selectNextPreset();
    } else if (duration < 3000) {
      sendSelectedPreset();
    } else {
      sendEasterEggIfPossible();
    }

    isPressing = false;
  }
}

void setup() {
  M5.begin(true, false, true);
  randomSeed(millis());

  selectDevice();
  connectWiFi();
  connectMqtt();

  drawPreset(selectedIndex);
}

void loop() {
  M5.update();

  if (!mqtt.connected()) {
    connectMqtt();
  }

  mqtt.loop();

  handleButton();

  if (isShowingReceived && currentReceivedIndex >= 0) {
    if (millis() - lastBlinkAt >= BLINK_INTERVAL_MS) {
      blinkVisible = !blinkVisible;
      lastBlinkAt = millis();

      if (blinkVisible) {
        drawPreset(currentReceivedIndex);
      } else {
        M5.dis.clear();
      }
    }
  }

  if (currentReceivedIndex >= 0 && millis() - receivedAt > displayTtlMs) {
    currentReceivedIndex = -1;
    isShowingReceived = false;
    blinkVisible = true;
    drawPreset(selectedIndex);
  }

  delay(20);
}