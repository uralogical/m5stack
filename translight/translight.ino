#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <HTTPUpdate.h>
#include <Preferences.h>

#include "config.h"
#include "patterns.h"

const char* FW_VERSION = "1.1.5";
const char* OTA_FEED = "translight-ota";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Preferences prefs;

char deviceId[32] = "";

bool isShowingReceived = false;
const unsigned long BREATH_CYCLE_MS = 3000;

int selectedIndex = 0;
int currentReceivedIndex = -1;
int currentReceivedHiddenIndex = -1;

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
uint32_t scaleColor(uint32_t color, float brightness) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  return ((uint32_t)(r * brightness) << 16) |
         ((uint32_t)(g * brightness) << 8) |
         (uint32_t)(b * brightness);
}

void drawPixelsBreathing(const uint32_t* pixels, float brightness) {
  for (int i = 0; i < 25; i++) {
    M5.dis.drawpix(i, pixels[i] != OFF ? scaleColor(pixels[i], brightness) : OFF);
  }
}

void drawPixels(const uint32_t* pixels) {
  for (int i = 0; i < 25; i++) {
    M5.dis.drawpix(i, pixels[i]);
  }
}

void drawPreset(int index) {
  drawPixels(presets[index].pixels);
}

void drawPixelsShifted(const uint32_t* pixels, int yOffset) {
  M5.dis.clear();

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      int srcY = y - yOffset;

      if (srcY < 0 || srcY >= 5) {
        continue;
      }

      int srcIndex = srcY * 5 + x;
      int dstIndex = y * 5 + x;

      uint32_t color = pixels[srcIndex];

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

void animateSend(const uint32_t* pixels) {
  for (int offset = 0; offset >= -5; offset--) {
    drawPixelsShifted(pixels, offset);
    delay(120);
  }
}

void animateReceive(const uint32_t* pixels) {
  for (int offset = -5; offset <= 0; offset++) {
    drawPixelsShifted(pixels, offset);
    delay(120);
  }
}

void playSendAnimation(int index) {
  animateSend(presets[index].pixels);
}

void playReceiveAnimation(int index) {
  animateReceive(presets[index].pixels);
}

void playRainbowOnPixels(const uint32_t* pixels, int stepDelayMs = 80) {
  uint32_t rainbow[] = { RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK };
  for (int step = 0; step < 24; step++) {
    uint32_t color = rainbow[step % 8];
    for (int i = 0; i < 25; i++) {
      M5.dis.drawpix(i, pixels[i] != OFF ? color : OFF);
    }
    delay(stepDelayMs);
  }
}

void playMatchAnimation(int index) {
  uint32_t rainbow[] = { RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK };
  int step = 0;
  while (true) {
    M5.update();
    if (M5.Btn.wasPressed()) break;

    for (int i = 0; i < 25; i++) {
      if (presets[index].pixels[i] != OFF) {
        M5.dis.drawpix(i, rainbow[(step + i) % 8]);
      } else {
        M5.dis.drawpix(i, OFF);
      }
    }
    step++;
    delay(100);
  }
  drawPreset(selectedIndex);
}

void playSecretAnimation(int index) {
  playRainbowOnPixels(presets[index].pixels);
  drawPreset(index);
}

void playSecretAnimationHidden(int index) {
  playRainbowOnPixels(hiddenPresets[index].pixels);
  drawPixels(hiddenPresets[index].pixels);
}

// ===== payload =====
int findPresetIndex(String value) {
  value.trim();

  for (int i = 0; i < VISIBLE_PRESET_COUNT; i++) {
    if (value == presets[i].id) return i;
  }

  return -1;
}

int findHiddenIndex(String value) {
  value.trim();

  for (int i = 0; i < HIDDEN_PRESET_COUNT; i++) {
    if (value == hiddenPresets[i].id) return i;
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

bool sendMatch(int index) {
  String payload = String("match:") + presets[index].id;
  return mqtt.publish(outTopic().c_str(), payload.c_str());
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

void onOtaProgress(int current, int total) {
  if (total <= 0) return;
  int filled = (int)((long)current * 25 / total);
  for (int i = 0; i < 25; i++) {
    int row = 4 - i / 5;
    bool leftToRight = (4 - row) % 2 == 0;
    int x = leftToRight ? (i % 5) : (4 - i % 5);
    int pixel = row * 5 + x;
    M5.dis.drawpix(pixel, i < filled ? CYAN : OFF);
  }
}

void performOta(String url) {
  url.trim();
  if (url.length() == 0) return;

  Serial.println("[OTA] start: " + url);
  Serial.println("[OTA] free heap: " + String(ESP.getFreeHeap()));

  M5.dis.clear();

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPUpdate httpUpdate;
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.onProgress(onOtaProgress);

  t_httpUpdate_return ret = httpUpdate.update(secureClient, url);

  switch (ret) {
    case HTTP_UPDATE_OK:
      Serial.println("[OTA] success, restarting...");
      flashColor(GREEN, 5);
      ESP.restart();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] no updates");
      flashColor(YELLOW, 3);
      break;
    default:
      Serial.println("[OTA] failed, error: " + String(httpUpdate.getLastError()) + " " + httpUpdate.getLastErrorString());
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

  bool isMatch = message.startsWith("match:");
  String value;
  if (isMatch) {
    value = message.substring(6);
    value.trim();
  } else {
    value = extractValue(message);
  }

  int presetIndex = findPresetIndex(value);
  int hiddenIndex = findHiddenIndex(value);

  if (presetIndex < 0 && hiddenIndex < 0) {
    flashColor(RED, 2);
    drawPreset(selectedIndex);
    return;
  }

  if (isMatch && presetIndex >= 0) {
    playMatchAnimation(presetIndex);
    currentReceivedIndex = -1;
    isShowingReceived = false;
    drawPreset(selectedIndex);
    return;
  }

  receivedAt = millis();
  displayTtlMs = extractTtlMs(message);
  isShowingReceived = true;

  if (presetIndex >= 0) {
    currentReceivedIndex = presetIndex;
    currentReceivedHiddenIndex = -1;
    playReceiveAnimation(presetIndex);
    drawPreset(presetIndex);
  } else {
    currentReceivedIndex = -1;
    currentReceivedHiddenIndex = hiddenIndex;
    animateReceive(hiddenPresets[hiddenIndex].pixels);
    playRainbowOnPixels(hiddenPresets[hiddenIndex].pixels, 120);
    drawPixels(hiddenPresets[hiddenIndex].pixels);
  }
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
    M5.dis.clear();
    for (int row = 4; row >= 0; row--) {
      bool leftToRight = (4 - row) % 2 == 0;
      for (int step = 0; step < 5; step++) {
        int x = leftToRight ? step : 4 - step;
        M5.dis.drawpix(row * 5 + x, WHITE);
        delay(40);
      }
    }

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

  if (selectedIndex >= VISIBLE_PRESET_COUNT) {
    selectedIndex = 0;
  }

  isShowingReceived = false;

  drawPreset(selectedIndex);
}

void sendSelectedPreset() {
  int sendingIndex = selectedIndex;
  bool isMatch = currentReceivedIndex >= 0 && sendingIndex == currentReceivedIndex;

  playSendAnimation(sendingIndex);

  bool ok;
  if (isMatch) {
    ok = sendMatch(sendingIndex);
  } else {
    ok = sendPresetByIndex(sendingIndex);
  }

  if (ok && isMatch) {
    playMatchAnimation(sendingIndex);
    currentReceivedIndex = -1;
    isShowingReceived = false;
  } else if (ok) {
    flashColor(GREEN, 2);
  } else {
    flashColor(RED, 3);
  }

  drawPreset(selectedIndex);
}

bool sendHiddenPreset(const char* id) {
  String payload = String("{\"type\":\"preset\",\"value\":\"") + id + "\",\"ttlSec\":1800}";
  return mqtt.publish(outTopic().c_str(), payload.c_str());
}

void sendEasterEggIfPossible() {
  String currentId = String(presets[selectedIndex].id);
  int hiddenIndex = -1;
  bool ok = false;

  if (currentId == "ramen") {
    hiddenIndex = findHiddenIndex("donburi");
  } else if (currentId == "alien") {
    hiddenIndex = findHiddenIndex("secret_alien");
  } else if (currentId == "sake") {
    hiddenIndex = findHiddenIndex("beer");
  } else if (currentId == "dead") {
    hiddenIndex = findHiddenIndex("skull");
  }

  if (hiddenIndex >= 0) {
    playRainbowOnPixels(hiddenPresets[hiddenIndex].pixels);
    animateSend(hiddenPresets[hiddenIndex].pixels);
    ok = sendHiddenPreset(hiddenPresets[hiddenIndex].id);
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

// ===== バージョンスクロール =====
void scrollText(const char* text, uint32_t color) {
  int charCount = strlen(text);
  int totalWidth = 0;
  for (int i = 0; i < charCount; i++) {
    int fi = fontIndex(text[i]);
    if (fi < 0) continue;
    int w = (text[i] == '.') ? 1 : 3;
    if (totalWidth > 0) totalWidth += 1;
    totalWidth += w;
  }

  for (int scroll = 5; scroll >= -totalWidth; scroll--) {
    M5.dis.clear();
    int xPos = scroll;
    for (int c = 0; c < charCount; c++) {
      int fi = fontIndex(text[c]);
      if (fi < 0) continue;
      int charWidth = (text[c] == '.') ? 1 : 3;
      if (c > 0) xPos += 1;
      for (int row = 0; row < 5; row++) {
        for (int col = 0; col < charWidth; col++) {
          int px = xPos + col;
          if (px >= 0 && px < 5) {
            if (font[fi][row] & (1 << (charWidth - 1 - col))) {
              M5.dis.drawpix(row * 5 + px, color);
            }
          }
        }
      }
      xPos += charWidth;
    }
    delay(150);
  }
}

bool detectShake() {
  float ax, ay, az;
  M5.IMU.getAccelData(&ax, &ay, &az);
  float magnitude = sqrt(ax * ax + ay * ay + az * az);
  return magnitude > 7.0;
}

void handleShake() {
  static unsigned long lastShakeAt = 0;
  if (detectShake() && millis() - lastShakeAt > 2000) {
    lastShakeAt = millis();
    scrollText(FW_VERSION, CYAN);
    drawPreset(selectedIndex);
  }
}

int getHiddenIndexForCurrent() {
  String currentId = String(presets[selectedIndex].id);
  if (currentId == "ramen") return findHiddenIndex("donburi");
  if (currentId == "alien") return findHiddenIndex("secret_alien");
  if (currentId == "sake") return findHiddenIndex("beer");
  if (currentId == "dead") return findHiddenIndex("skull");
  return -1;
}

void handleButton() {
  static unsigned long pressedAt = 0;
  static bool isPressing = false;
  static bool hiddenShown = false;

  if (M5.Btn.wasPressed()) {
    pressedAt = millis();
    isPressing = true;
    hiddenShown = false;
  }

  if (isPressing && !hiddenShown && millis() - pressedAt >= 3000) {
    int hi = getHiddenIndexForCurrent();
    if (hi >= 0) {
      drawPixels(hiddenPresets[hi].pixels);
    }
    hiddenShown = true;
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
  M5.IMU.Init();
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
  handleShake();

  bool hasReceived = currentReceivedIndex >= 0 || currentReceivedHiddenIndex >= 0;

  if (isShowingReceived && hasReceived) {
    float phase = (millis() % BREATH_CYCLE_MS) / (float)BREATH_CYCLE_MS;
    float brightness = (1.0 + cos(phase * 2.0 * PI)) / 2.0;
    brightness = 0.15 + brightness * 0.85;

    bool isDonburi = currentReceivedHiddenIndex >= 0 &&
      String(hiddenPresets[currentReceivedHiddenIndex].id) == "donburi";

    bool isBeer = currentReceivedHiddenIndex >= 0 &&
      String(hiddenPresets[currentReceivedHiddenIndex].id) == "beer";

    if (isDonburi) {
      float steamPhase = (millis() % 1000) / 1000.0;
      bool steamFrame = steamPhase < 0.5;
      uint32_t steam1 = scaleColor(WHITE, steamFrame ? brightness : 0.0);
      uint32_t steam2 = scaleColor(WHITE, steamFrame ? 0.0 : brightness);
      uint32_t bowlColor = scaleColor(RED, brightness);
      uint32_t px[25] = {
        steam1,    steam2,    steam1,    steam2,    steam1,
        steam2,    steam1,    steam2,    steam1,    steam2,
        bowlColor, bowlColor, bowlColor, bowlColor, bowlColor,
        OFF,       bowlColor, bowlColor, bowlColor, OFF,
        OFF,       OFF,       bowlColor, OFF,       OFF
      };
      drawPixels(px);
    } else if (isBeer) {
      const unsigned long POUR_CYCLE_MS = 5000;
      unsigned long t = millis() % POUR_CYCLE_MS;
      int frame = t / 1000;

      uint32_t Y = scaleColor(YELLOW, brightness);
      uint32_t W = scaleColor(WHITE, brightness);

      if (frame == 0) {
        uint32_t px[25] = {
          Y, OFF, OFF, Y, OFF,
          Y, OFF, OFF, Y, W,
          Y, OFF, OFF, Y, W,
          Y, OFF, OFF, Y, OFF,
          Y, Y, Y, Y, OFF
        };
        drawPixels(px);
      } else if (frame == 1) {
        uint32_t px[25] = {
          Y, OFF, OFF, Y, OFF,
          Y, OFF, OFF, Y, W,
          Y, OFF, OFF, Y, W,
          Y, Y,   Y,   Y, OFF,
          Y, Y,   Y,   Y, OFF
        };
        drawPixels(px);
      } else if (frame == 2) {
        uint32_t px[25] = {
          Y, OFF, OFF, Y, OFF,
          Y, OFF, OFF, Y, W,
          Y, Y,   Y,   Y, W,
          Y, Y,   Y,   Y, OFF,
          Y, Y,   Y,   Y, OFF
        };
        drawPixels(px);
      } else if (frame == 3) {
        uint32_t px[25] = {
          Y, OFF, OFF, Y, OFF,
          Y, Y,   Y,   Y, W,
          Y, Y,   Y,   Y, W,
          Y, Y,   Y,   Y, OFF,
          Y, Y,   Y,   Y, OFF
        };
        drawPixels(px);
      } else {
        uint32_t px[25] = {
          Y, W,   W,   Y, OFF,
          Y, Y,   Y,   Y, W,
          Y, Y,   Y,   Y, W,
          Y, Y,   Y,   Y, OFF,
          Y, Y,   Y,   Y, OFF
        };
        drawPixels(px);
      }
    } else {
      const uint32_t* pixels = (currentReceivedIndex >= 0)
        ? presets[currentReceivedIndex].pixels
        : hiddenPresets[currentReceivedHiddenIndex].pixels;
      drawPixelsBreathing(pixels, brightness);
    }
  }

  if (hasReceived && millis() - receivedAt > displayTtlMs) {
    currentReceivedIndex = -1;
    currentReceivedHiddenIndex = -1;
    isShowingReceived = false;
    drawPreset(selectedIndex);
  }

  delay(20);
}
