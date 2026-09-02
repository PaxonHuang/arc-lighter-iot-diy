// firmware/arduino/arc_remote/include/config.h
// 集中所有硬件引脚与可调参数,便于维护与移植
// v2 (2026-08-31): M100PG-C2 实测无 doout 资源,继电器改由 UNO D7 驱动
#ifndef CONFIG_H
#define CONFIG_H

// === 串口配置 ===
#define UART_BAUD 9600          // 与 DTU 的 TTL 串口(D0/D1)
#define DEBUG_BAUD 9600         // SoftwareSerial 调试口(D5)

// === Arduino 引脚映射 ===
#define LOCAL_BTN_PIN    2   // 本地点动按键(中断触发)
#define STATUS_LED_PIN   4   // 状态 LED(继电器吸合时常亮)
#define RELAY_PIN        7   // 继电器模块 IN(高电平触发) — 继电器控制权在本板
#define DEBUG_TX_PIN     5   // 调试日志输出口(接 USB-TTL 适配器 RX)
#define DTU_RST_PIN      6   // 预留:接 DTU RST 做异常恢复(本版未启用,保持高电平)

// === 调试口开关 ===
// 量产可置 0 省 ~1.5KB flash;联调期保持 1
#ifndef DEBUG_LOG
#define DEBUG_LOG 1
#endif

// === 防抖 ===
#define DEBOUNCE_MS      50

#endif
