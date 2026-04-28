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
  { "yeah", "🎶", {
    OFF,    OFF,    YELLOW, YELLOW, YELLOW,
    OFF,    OFF,    YELLOW, OFF,    YELLOW,
    OFF,    OFF,    YELLOW, OFF,    YELLOW,
    OFF,    YELLOW, YELLOW, OFF,    YELLOW,
    YELLOW, YELLOW, OFF,    YELLOW, YELLOW
  }},
  { "secret_alien", "SECRET", {
    OFF,    PURPLE, PURPLE, PURPLE, OFF,
    PURPLE, WHITE,  OFF,    WHITE,  PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
    PURPLE, OFF,    PURPLE, OFF,    PURPLE
  }}
};

const int PRESET_COUNT = sizeof(presets) / sizeof(presets[0]);