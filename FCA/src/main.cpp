#include <Arduino.h>

#define RELAY_PIN_1 25
#define RELAY_PIN_2 26

void setup() {
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  
  // خاموش کردن اولیه رله‌ها
  digitalWrite(RELAY_PIN_1, HIGH);  // برای Low Level Trigger
  digitalWrite(RELAY_PIN_2, HIGH);
  
  randomSeed(analogRead(0));
}

void loop() {
  // زمان روشن: 60-90 ثانیه
  unsigned long onTime = random(60, 91) * 1000;
  
  // زمان خاموش: 5-7 دقیقه
  unsigned long offTime = random(5, 8) * 60 * 1000;
  
  // ========== روشن کردن هر دو رله ==========
  digitalWrite(RELAY_PIN_1, LOW);   // LOW = روشن (Low Level Trigger)
  digitalWrite(RELAY_PIN_2, LOW);
  delay(onTime);                    // 60-90 ثانیه روشن
  
  // ========== خاموش کردن هر دو رله ==========
  digitalWrite(RELAY_PIN_1, HIGH);  // HIGH = خاموش
  digitalWrite(RELAY_PIN_2, HIGH);
  delay(offTime);                   // 5-7 دقیقه خاموش
}
