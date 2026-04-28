#pragma once

// ===== 色 =====
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

// ===== プリセット定義 =====
struct Preset {
  const char* id;
  const char* label;
  uint32_t pixels[25];
};

// ラ = ラーメン
uint32_t patternRa[25] = {
  OFF, RED, RED, RED, OFF,
  OFF, OFF, OFF, OFF, OFF,
  RED, RED, RED, RED, RED,
  OFF, OFF, OFF, OFF, RED,
  OFF, OFF, RED, RED, OFF
};

// サ = 酒
uint32_t patternSa[25] = {
  OFF,    ORANGE, OFF,    ORANGE, OFF,
  ORANGE, ORANGE, ORANGE, ORANGE, ORANGE,
  OFF,    ORANGE, OFF,    ORANGE, OFF,
  OFF,    OFF,    OFF,    ORANGE, OFF,
  OFF,    OFF,    ORANGE, OFF,    OFF
};

// シ = 死
uint32_t patternShi[25] = {
  BLUE, OFF,  OFF,  OFF,  OFF,
  OFF,  BLUE, OFF,  OFF,  OFF,
  BLUE, OFF,  OFF,  OFF,  BLUE,
  OFF,  OFF,  OFF,  BLUE, OFF,
  OFF,  BLUE, BLUE, OFF,  OFF
};

// 👾 = ゲーム/遊ぶ
uint32_t patternAlien[25] = {
  OFF,    PURPLE, PURPLE, PURPLE, OFF,
  PURPLE, WHITE,  OFF,    WHITE,  PURPLE,
  PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
  PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
  PURPLE, OFF,    PURPLE, OFF,    PURPLE
};

// 🎶 = カラオケ/音楽
uint32_t patternMusic[25] = {
  OFF,    OFF,    YELLOW, YELLOW, YELLOW,
  OFF,    OFF,    YELLOW, OFF,    YELLOW,
  OFF,    OFF,    YELLOW, OFF,    YELLOW,
  OFF,    YELLOW, YELLOW, OFF,    YELLOW,
  YELLOW, YELLOW, OFF,    YELLOW, YELLOW
};

// ☠ = ドクロ。今は未使用。追加したくなったらpresetsに入れる
uint32_t patternSkull[25] = {
  WHITE, WHITE, WHITE, WHITE, WHITE,
  WHITE, OFF,   WHITE, OFF,   WHITE,
  WHITE, WHITE, WHITE, WHITE, WHITE,
  OFF,   WHITE, WHITE, WHITE, OFF,
  OFF,   WHITE, OFF,   WHITE, OFF
};

Preset presets[] = {
  { "ramen",    "ラ",  {
    OFF, RED, RED, RED, OFF,
    OFF, OFF, OFF, OFF, OFF,
    RED, RED, RED, RED, RED,
    OFF, OFF, OFF, OFF, RED,
    OFF, OFF, RED, RED, OFF
  }},
  { "sake",    "サ",  {
    OFF,    ORANGE, OFF,    ORANGE, OFF,
    ORANGE, ORANGE, ORANGE, ORANGE, ORANGE,
    OFF,    ORANGE, OFF,    ORANGE, OFF,
    OFF,    OFF,    OFF,    ORANGE, OFF,
    OFF,    OFF,    ORANGE, OFF,    OFF
  }},
  { "dead",   "シ",  {
    BLUE, OFF,  OFF,  OFF,  OFF,
    OFF,  BLUE, OFF,  OFF,  OFF,
    BLUE, OFF,  OFF,  OFF,  BLUE,
    OFF,  OFF,  OFF,  BLUE, OFF,
    OFF,  BLUE, BLUE, OFF,  OFF
  }},
  { "alien", "👾",  {
    OFF,    PURPLE, PURPLE, PURPLE, OFF,
    PURPLE, WHITE,  OFF,    WHITE,  PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, OFF,    PURPLE, OFF,    PURPLE
  }},
  { "yeah", "🎶",  {
    OFF,    OFF,    YELLOW, YELLOW, YELLOW,
    OFF,    OFF,    YELLOW, OFF,    YELLOW,
    OFF,    OFF,    YELLOW, OFF,    YELLOW,
    OFF,    YELLOW, YELLOW, OFF,    YELLOW,
    YELLOW, YELLOW, OFF,    YELLOW, YELLOW
  }}
};

const int PRESET_COUNT = sizeof(presets) / sizeof(presets[0]);