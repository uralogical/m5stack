#include <M5Atom.h>

const uint32_t OFF = 0x000000;
const uint32_t PURPLE = 0x8000FF;
const uint32_t WHITE = 0xFFFFFF;

uint32_t alien[25] = {
  OFF,    PURPLE, PURPLE, PURPLE, OFF,
  PURPLE, WHITE,  PURPLE,    WHITE,  PURPLE,
  PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
  PURPLE, PURPLE, PURPLE, PURPLE, PURPLE,
  PURPLE, OFF,    PURPLE, OFF,    PURPLE
};

void drawAlien() {
  for (int i = 0; i < 25; i++) {
    M5.dis.drawpix(i, alien[i]);
  }
}

void setup() {
  M5.begin(true, false, true);
  drawAlien();
}

void loop() {
  M5.update();

  if (M5.Btn.wasPressed()) {
    drawAlien();
  }

  if (M5.Btn.pressedFor(1000)) {
    M5.dis.clear();
  }

  delay(20);
}