#pragma once

const uint32_t OFF    = 0x000000;
const uint32_t RED    = 0xFF0000;
const uint32_t ORANGE = 0xFF7F00;
const uint32_t YELLOW = 0xFFFF00;
const uint32_t GREEN  = 0x00FF00;
const uint32_t CYAN   = 0x00FFFF;
const uint32_t BLUE   = 0x0000FF;
const uint32_t PURPLE = 0x8000FF;
const uint32_t PINK   = 0xFF00FF;
const uint32_t WHITE  = 0xFFFFFF;

struct Preset {
  const char* id;
  const char* label;
  uint32_t pixels[25];
};

Preset presets[] = {
  { "ramen", "ラ", {
    OFF, RED, RED, RED, OFF,
    OFF, OFF, OFF, OFF, OFF,
    RED, RED, RED, RED, RED,
    OFF, OFF, OFF, OFF, RED,
    OFF, OFF, RED, RED, OFF
  }},
  { "sake", "サ", {
    OFF,    ORANGE, OFF,    ORANGE, OFF,
    ORANGE, ORANGE, ORANGE, ORANGE, ORANGE,
    OFF,    ORANGE, OFF,    ORANGE, OFF,
    OFF,    OFF,    OFF,    ORANGE, OFF,
    OFF,    OFF,    ORANGE, OFF,    OFF
  }},
  { "dead", "シ", {
    BLUE, OFF,  OFF,  OFF,  OFF,
    OFF,  BLUE, OFF,  OFF,  OFF,
    BLUE, OFF,  OFF,  OFF,  BLUE,
    OFF,  OFF,  OFF,  BLUE, OFF,
    OFF,  BLUE, BLUE, OFF,  OFF
  }},
  { "alien", "👾", {
    OFF,    PURPLE, PURPLE, PURPLE, OFF,
    PURPLE, WHITE,  OFF,    WHITE,  PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, OFF,    PURPLE, OFF,    PURPLE
  }},
};

const int VISIBLE_PRESET_COUNT = sizeof(presets) / sizeof(presets[0]);

Preset hiddenPresets[] = {
  { "secret_alien", "SECRET", {
    OFF,    PURPLE, PURPLE, PURPLE, OFF,
    PURPLE, WHITE,  OFF,    WHITE,  PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, OFF,    PURPLE, OFF,    PURPLE
  }},
  { "beer", "🍺", {
    YELLOW, YELLOW, YELLOW, YELLOW, OFF,
    YELLOW, OFF,    OFF,    YELLOW, WHITE,
    YELLOW, OFF,    OFF,    YELLOW, WHITE,
    YELLOW, OFF,    OFF,    YELLOW, OFF,
    YELLOW, YELLOW, YELLOW, YELLOW, OFF
  }},
  { "donburi", "🍜", {
    WHITE, OFF,   WHITE, OFF,   WHITE,
    OFF,   WHITE, OFF,   WHITE, OFF,
    RED,   RED,   RED,   RED,   RED,
    OFF,   RED,   RED,   RED,   OFF,
    OFF,   OFF,   RED,   OFF,   OFF
  }},
  { "skull", "💀", {
    OFF,   WHITE, WHITE, WHITE, OFF,
    WHITE, OFF,   WHITE, OFF,   WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE,
    OFF,   WHITE, OFF,   WHITE, OFF,
    OFF,   OFF,   WHITE, OFF,   OFF
  }}
};

const int HIDDEN_PRESET_COUNT = sizeof(hiddenPresets) / sizeof(hiddenPresets[0]);

// ===== スクロール用フォント (3x5) =====
const uint8_t font[][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
  {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
  {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
  {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
  {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
  {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
  {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
  {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
  {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
  {0b000, 0b000, 0b000, 0b000, 0b001}, // . (index 10)
};

int fontIndex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c == '.') return 10;
  return -1;
}