# CLAUDE.md — 项目上下文与文档导航（M100PG-C2 电弧点火机）

> 本文件是进入本项目的第一入口，用于约束后续会话/Agent 的注意力，避免读到已作废内容而产生幻觉。

## 1. 当前权威文档（改代码前先读这些）
| 主题 | 文件 | 状态 |
|---|---|---|
| 接线 / 硬件 / 供电轨 | `docs/wiring-guide.md` (**v3**) | ✅ 权威 |
| 平台配置与联调（test/DTU/IOT） | `docs/yinerda-platform-ops-guide.md` | ✅ 权威 |
| IOT 三要素 / MQTT topic | `cloud/iot-platform.md` | ✅ 权威（含设备凭据，仓库须私有） |
| 器件清单 BOM | `hardware/bom.csv` | ✅ 权威 |
| 点火模块原理图 | `img/lighter-high-Voltage-Transformer.jpg` | ✅ 商供原件 |
| 固件 | `firmware/arduino/arc_remote/`（PlatformIO）、`firmware/m100pg-c2/task.lua` | ✅ 最新 |

## 2. 已作废 / 仅历史参考（勿据此实现）
- `docs/superpowers/specs/*`、`docs/superpowers/plans/*`：2026-07-26 初版设计与计划。**其中以下结论已被实测推翻**：
  - "doout 驱动继电器" → M100PG-C2 **无 DO 资源**，继电器改由 **UNO D7** 驱动，DTU 经 UART 下发 `relay:1/relay:0`。
  - "XL6009→D880 单管自激、24V 母线" → 点火模块实为 **DC12V 输入的 ZVS 推挽自激振荡升压**整板。
  - **蜂鸣器功能已全部移除**（固件不再使用 D8）。

## 3. 关键硬件事实（务必牢记，避免走回头路）
- **M100PG-C2 引脚**：仅 VIN/GND/TXD/RXD/DTR/RST/RDY/1PPS，**没有 DO/继电器输出脚**。数字量控制一律经 Arduino。
- **继电器控制权在 UNO D7**（高电平触发），上电默认 `LOW`（电弧回路断开）。
- **点火模块 ZVS 无反馈闭环**：通电即满功率起振 → **单次点火必须限时（建议 ≤3~5s）**，应在 `task.lua` 增加 relay:1 超时自动关断；断电后对高压电容充分放电。
- **供电分两轨**：5V（UNO+继电器 VCC）与 12V（点火模块动力电，经继电器 COM/NO 切换）。不可把 18650(3.7~4.2V) 直供点火模块。
- **串口协议(9600,8,N,1)**：DTU→UNO `"relay:1\n"/"relay:0\n"`；UNO→DTU `{"evt":"btn_toggle"}` / `{"type":"boot"...}`。调试日志走 D5 SoftwareSerial，板载 USB(D0/D1) 留给 DTU 串口。

## 4. 官方厂商文档子模块 `Air780-YED-M100PG-C2-doc/`
- 这是一个 **git submodule**（远端 `github.com/PaxonHuang/Air780-YED-M100PG-C2-doc`，与官方仓库 1:1）。默认**不会被索引/glob**，只在需要查官方 API/案例时按需打开对应文件（见 ops-guide §引用清单：MQTT 远程控制实例、物模型 MQTT 协议、任务/数据模板规范）。
- ⚠️ **`Air780-YED-M100PG-C2-doc/graphify-out/` 是知识图谱生成缓存（53 个 JSON），非厂商文档，请勿当作事实来源阅读。**
- 与本项目无关、无需主动阅读的官方分支：GPS/定位案例、HTTP/WebSocket/UDP 配置、Modbus 采集、低功耗等——用到再看，不必预读。

## 5. 下一步开发路线（阶段 A）
**目标：不依赖 IOT 平台，先打通 MQTT 链路。** 待办要点：
- 核对 IMEI 第 10 位不一致问题（`wiring-guide.md §1.3` / `iot-platform.md` 标注的 `0↔6`），否则通道鉴权可能失败。
- DTU 网络通道 1 指向 broker（先用公共测试 broker 或本地 mosquitto，绕开 IOT 平台），验证 `PronetGetNetSta==1`。
- task.lua 上行/下行 JSON 收发经 raw MQTT 客户端可观测；加入 relay 超时自动关断。
