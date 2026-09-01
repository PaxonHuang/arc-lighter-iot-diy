// firmware/arduino/arc_remote/src/main.cpp
// YED 4G IoT 电弧点火机 - Arduino 协控固件 v2.0 (PlatformIO)
//
// 职责:
//   - 继电器驱动(D7):接收 DTU 下行命令吸合/释放 — M100PG-C2 无 doout 资源,
//     继电器控制权在本板(见 docs/wiring-guide.md v2)
//   - 本地点动按键 → 通过串口通知 DTU 上报切换事件
//   - 状态 LED 同步 + 按键蜂鸣提示
//
// 串口协议(9600,8,N,1):
//   - DTU → Arduino: "relay:1\n" / "relay:0\n"
//   - Arduino → DTU: {"evt":"btn_toggle"} / {"type":"boot",...}
// 调试口: SoftwareSerial D5,9600 — 烧录或板载 USB 被 D0/D1 占用时用外接 USB-TTL 观察

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "config.h"

static SoftwareSerial dbg(-1, DEBUG_TX_PIN);  // 仅 TX

volatile bool btnPressed = false;
bool relayState = false;
unsigned long lastBtnMs = 0;

void btnISR() { btnPressed = true; }

void dbgPrint(const __FlashStringHelper *s) {
#if DEBUG_LOG
    dbg.println(s);
#endif
}

void beepShort() {
    tone(BUZZER_PIN, 2000, BUZZER_SHORT_MS);
}

void applyRelay(bool on) {
    relayState = on;
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
    digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}

void setup() {
    Serial.begin(UART_BAUD);
#if DEBUG_LOG
    dbg.begin(DEBUG_BAUD);
#endif
    pinMode(LOCAL_BTN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LOCAL_BTN_PIN), btnISR, FALLING);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);      // 上电确保电弧回路断开
    digitalWrite(STATUS_LED_PIN, LOW);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(DTU_RST_PIN, OUTPUT);      // 预留 DTU 复位线,空闲保持高
    digitalWrite(DTU_RST_PIN, HIGH);

    dbgPrint(F("{\"type\":\"boot\",\"fwv\":\"arduino-2.0.0\"}"));
    delay(200);
    Serial.println(F("{\"type\":\"ready\"}"));
    dbgPrint(F("{\"type\":\"ready\"}"));
}

void loop() {
    // 处理本地按键(带 50ms 消抖)
    if (btnPressed) {
        btnPressed = false;
        unsigned long now = millis();
        if (now - lastBtnMs > DEBOUNCE_MS) {
            lastBtnMs = now;
            Serial.println(F("{\"evt\":\"btn_toggle\"}"));
            dbgPrint(F("evt=btn_toggle"));
            beepShort();
        }
    }

    // 接收 DTU 下行继电器命令
    if (Serial.available()) {
        String s = Serial.readStringUntil('\n');
        s.trim();
        if (s == "relay:1") {
            applyRelay(true);
            dbgPrint(F("relay=ON"));
            beepShort();
        } else if (s == "relay:0") {
            applyRelay(false);
            dbgPrint(F("relay=OFF"));
        } else if (s.length() > 0) {
#if DEBUG_LOG
            dbg.print(F("rx? ")); dbg.println(s);
#endif
        }
    }

    delay(50);
}
