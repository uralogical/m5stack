#include <M5Atom.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "patterns.h"

// ===== MQTT =====
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

int selectedIndex = 0;

// ===== MQTT Topic =====
String topic() {
  return String(IO_USERNAME) + "/feeds/" + OUT_FEED;
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

void playSecretAnimation() {
  uint32_t colors[] = {
    RED,
    ORANGE,
    YELLOW,
    GREEN,
    CYAN,
    BLUE,
    PURPLE,
    PINK
  };

  for (int i = 0; i < 20; i++) {
    uint32_t color = colors[i % 8];

    for (int p = 0; p < 25; p++) {
      if (presets[selectedIndex].pixels[p] != OFF) {
        M5.dis.drawpix(p, color);
      } else {
        M5.dis.drawpix(p, OFF);
      }
    }

    delay(80);
  }

  M5.dis.clear();
}

// ===== Wi-Fi / MQTT =====
void connectWiFi() {
  M5.dis.fillpix(BLUE);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  flashColor(GREEN, 1);
}

void connectMqtt() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);

  while (!mqtt.connected()) {
    M5.dis.fillpix(WHITE);

    String clientId = "atom-sender-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), IO_USERNAME, IO_KEY)) {
      flashColor(GREEN, 1);
    } else {
      flashColor(RED, 2);
      delay(1000);
    }
  }
}

// ===== 送信 =====
bool sendPreset(int index) {
  String payload = String("{\"type\":\"preset\",\"value\":\"") +
                   presets[index].id +
                   "\",\"ttlSec\":1800}";

  return mqtt.publish(topic().c_str(), payload.c_str());
}

bool sendSecret() {
  String payload = "{\"type\":\"easter_egg\",\"value\":\"secret_alien\",\"ttlSec\":60}";
  return mqtt.publish(topic().c_str(), payload.c_str());
}

bool isAlienSelected() {
  return String(presets[selectedIndex].id) == "alien";
}

// ===== setup / loop =====
void setup() {
  M5.begin(true, false, true);
  randomSeed(millis());

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

  static unsigned long pressedAt = 0;
  static bool isPressing = false;

  // 押し始め
  if (M5.Btn.wasPressed()) {
    pressedAt = millis();
    isPressing = true;
  }

  // 離したときに、短押し/長押し/隠しコマンドを判定
  if (M5.Btn.wasReleased() && isPressing) {
    unsigned long pressDuration = millis() - pressedAt;

    // 3秒以上: イースターエッグ
    if (pressDuration >= 3000) {
      bool ok = false;

      if (isAlienSelected()) {
        playSecretAnimation();
        ok = sendSecret();
      } else {
        playSendAnimation(selectedIndex);
        ok = sendPreset(selectedIndex);
      }

      if (ok) {
        flashColor(GREEN, 2);
      } else {
        flashColor(RED, 3);
      }

      drawPreset(selectedIndex);
    }

    // 1秒以上3秒未満: 通常送信
    else if (pressDuration >= 1000) {
      int sendingIndex = selectedIndex;

      playSendAnimation(sendingIndex);

      bool ok = sendPreset(sendingIndex);

      if (ok) {
        flashColor(GREEN, 2);
      } else {
        flashColor(RED, 3);
      }

      drawPreset(selectedIndex);
    }

    // 1秒未満: 次のプリセットへ
    else {
      selectedIndex++;

      if (selectedIndex >= PRESET_COUNT) {
        selectedIndex = 0;
      }

      drawPreset(selectedIndex);
    }

    isPressing = false;
  }

  delay(20);
}