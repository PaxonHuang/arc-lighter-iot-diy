# YED-CAMP-HEATER-IOT-STRESS / 高压电弧点火机 DIY

基于银尔达 M100PG-C2 (Air780EPM) 的 4G IoT 远程控制电弧点火装置。

## 项目文档
- SPEC: `docs/superpowers/specs/2026-07-26-camp-heater-iot-stress-test-design.md`
- 实施计划: `docs/superpowers/plans/2026-07-26-arc-lighter-iot-diy.md`

## 代码结构
- `firmware/arduino/` — Arduino UNO 协控固件(C++)
- `firmware/m100pg-c2/` — M100PG-C2 DTU Lua 任务
- `cloud/` — 银尔达 IOT 平台配置
- `hardware/` — 硬件接线、BOM、原理图
- `test/` — 端到端测试脚本
- 接线指南: `docs/wiring-guide.md`（v2：继电器由 UNO D7 驱动，DTU 无 doout 资源）
- 平台操作: `docs/yinerda-platform-ops-guide.md`；IOT 三要素: `cloud/iot-platform.md`

## 快速开始
参见 SPEC 文档 §6 操作步骤。





ZSNNMSL
