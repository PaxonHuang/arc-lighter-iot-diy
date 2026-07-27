// firmware/arduino/arc_remote/config.h
// 集中所有硬件引脚与可调参数,便于维护与移植
#ifndef CONFIG_H
#define CONFIG_H

// === 串口配置 ===
#define UART_BAUD 9600

// === Arduino 引脚映射 ===
#define LOCAL_BTN_PIN    2   // 本地点动按键(中断触发)
#define STATUS_LED_PIN   4   // 状态 LED(继电器吸合时常亮)
#define BUZZER_PIN       8   // 有源蜂鸣器

// === 协议字节 ===
#define PROTO_HEADER     0xA5
#define PROTO_CMD_RELAY  0x01
#define PROTO_RELAY_ON   0x01
#define PROTO_RELAY_OFF  0x00

// === 防抖 ===
#define DEBOUNCE_MS      50

// === 蜂鸣器时长 ===
#define BUZZER_SHORT_MS  100

#endif
