// firmware/arduino/arc_remote/arc_remote.ino
// 协控固件: 本地点动按键 + 状态 LED + 蜂鸣器 + 串口桥接 DTU
#include "config.h"

// 全局状态
volatile bool btnPressed = false;
bool relayState = false;     // 镜像 DTU 下发的继电器状态
unsigned long lastBtnMs = 0;

// 按键中断服务程序
void btnISR() {
    btnPressed = true;
}

// 蜂鸣器短鸣
void beepShort() {
    tone(BUZZER_PIN, 2000, BUZZER_SHORT_MS);
}

// 同步状态到 LED
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

    Serial.println("{\"type\":\"boot\",\"fwv\":\"arduino-1.0.0\"}");
}

void loop() {
    // 1. 处理本地按键(带消抖)
    if (btnPressed) {
        btnPressed = false;
        unsigned long now = millis();
        if (now - lastBtnMs > DEBOUNCE_MS) {
            lastBtnMs = now;
            // 通过串口通知 DTU 切换继电器
            Serial.println("{\"evt\":\"btn_toggle\"}");
            beepShort();
        }
    }

    // 2. 接收 DTU 下行同步命令(简化协议: 字符串 "relay:1" / "relay:0")
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
