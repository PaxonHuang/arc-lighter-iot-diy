# YED-CAMP-HEATER-IOT-STRESS / 高压电弧点火机 DIY

基于银尔达 M100PG-C2 (Air780EPM) 的 4G IoT 远程控制电弧点火装置。

## 项目文档（当前权威）
- **接线/硬件**: `docs/wiring-guide.md`（**v3**：继电器由 UNO D7 驱动；DTU 无 doout 资源；点火模块为 DC12V ZVS 推挽升压；已移除蜂鸣器）
- **平台操作**: `docs/yinerda-platform-ops-guide.md`；IOT 三要素: `cloud/iot-platform.md`
- 器件 BOM: `hardware/bom.csv`；原理图: `img/lighter-high-Voltage-Transformer.jpg`（商供点火模块）

> ⚠️ 历史文档：`docs/superpowers/`（初版 SPEC/计划）中的 doout 驱动、XL6009→D880 单管、蜂鸣器等描述**均已作废**，仅作历史脉络参考，勿据此实现。

## 代码结构
- `firmware/arduino/` — Arduino UNO 协控固件(C++，PlatformIO 工程)
- `firmware/m100pg-c2/` — M100PG-C2 DTU Lua 任务
- `cloud/` — 银尔达 IOT 平台配置
- `hardware/` — BOM；`img/` — 原理图
- `test/` — 端到端测试脚本

## 快速开始
以 `docs/wiring-guide.md`(v3) 的装配步骤为准；平台配置见 `docs/yinerda-platform-ops-guide.md`。





ZSNNMSL