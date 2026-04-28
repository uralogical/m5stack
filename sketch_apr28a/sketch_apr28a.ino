#include <M5Atom.h>

void setup() {
  M5.begin(true, false, true);
}

void loop() {
  M5.update();

  if (M5.Btn.wasPressed()) {
    M5.dis.fillpix(0xffff00);  // 黄色
  }

  if (M5.Btn.pressedFor(1000)) {
    M5.dis.clear();  // 1秒長押しで消灯
  }

  delay(20);
}