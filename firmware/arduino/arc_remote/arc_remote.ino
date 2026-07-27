// firmware/arduino/arc_remote/arc_remote.ino
// YED 4G IoT 电弧点火机 - Arduino 协控固件 v1.0
//
// 职责:
//   - 本地点动按键 → 通过串口通知 DTU 切换继电器
//   - 接收 DTU 下行命令 → 同步状态 LED
//   - 按键时蜂鸣器提示
//
// 引脚定义见 config.h
// 串口协议: 9600,8,N,1
//   - DTU → Arduino: "relay:1\n" / "relay:0\n"
//   - Arduino → DTU: "{\"evt\":\"btn_toggle\"}\n"

#include "config.h"

volatile bool btnPressed = false;
bool relayState = false;
unsigned long lastBtnMs = 0;

void btnISR() { btnPressed = true; }

void beepShort() {
    tone(BUZZER_PIN, 2000, BUZZER_SHORT_MS);
}

void syncLed() {
    digitalWrite(STATUS_LED_PIN, relayState ? HIGH : LOW);
}

void setup() {
    Serial.begin(UART_BAUD);
    pinMode(LOCAL_BTN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LOCAL_BTN_PIN), btnISR, FALLING);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    pinMode(BUZZER_PIN, OUTPUT);

    Serial.println(F("{\"type\":\"boot\",\"fwv\":\"arduino-1.0.0\"}"));
    delay(200);
    Serial.println(F("{\"type\":\"ready\"}"));
}

void loop() {
    // 处理本地按键(带 50ms 消抖)
    if (btnPressed) {
        btnPressed = false;
        unsigned long now = millis();
        if (now - lastBtnMs > DEBOUNCE_MS) {
            lastBtnMs = now;
            Serial.println(F("{\"evt\":\"btn_toggle\"}"));
            beepShort();
        }
    }

    // 接收 DTU 下行同步
    if (Serial.available()) {
        String s = Serial.readStringUntil('\n');
        s.trim();
        if (s == "relay:1") {
            relayState = true;
            syncLed();
        } else if (s == "relay:0") {
            relayState = false;
            syncLed();
        }
    }

    delay(50);
}
