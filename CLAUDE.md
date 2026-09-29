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

## 5. 下一步开发路线（阶段 A → B）
**当前状态：阶段 A 已完成 ✅，阶段 B 全链路已完成 ✅（B-1~B-5 全过，双向 JSON 链路贯通）。下一步 = 接 UNO 测 D7 物理继电器 → 阶段 C（IOT 平台）。**

**阶段 A 完成项**（详见 `docs/handoff-2026-09-06-phase-A-done.md`）：
- ✅ 设备 IMEI 已定位：`864865083079369`，与 `cloud/iot-platform.md` 存储的 `...679369` 第10位不一致——平台记录错误（设备权威），阶段 C 前必须先在 iot.yinerda.com 删除旧设备、用正确 IMEI 重建。
- ✅ DTU 通道 1 已用 `config,set,mqtt,1,uart,120,43.139.170.206:1002,...` 配通 test.yinerda.com 公共测试 broker（10 分钟无交互过期，重刷重配）。
- ✅ 重启后 netstatus=1 / ssta=4；串口 `config,get,imei/csq/vbatt` 全部正常应答；浏览器订阅 yed/arc/up 看到设备消息。
- ✅ task.lua 已加 4s relay:1 超时（ZVS 安全），commit `f101184`。

**阶段 B 进展（截至 2026-09-29 23:56）——✅ 全链路完成**：
- ✅ B-1：RAM 配置 save → 已固化（含 LBS 60s）
- ✅ B-2：save 后重启 + MQTT 链路复测 ok,1
- ✅ B-3：dtu.yinerda.com「任务」tab 已粘贴 task.lua **v4**（v1~v3 裸代码缺 function 包装，调度器不执行——见 handoff §7.6）+ 点保存参数 + 设备自动重启拉任务
- ✅ B-4：Luatools（4G-USB，勾「4G模块USB打印」）看到 `iotArcTask ===== START =====`；根因=log 默认关闭 + USB 口选错 + 裸代码缺 function
- ✅ B-5：test 工具发 `set_relay`（did=t1, sw1=1）→ 收 `set_relay_bck`（did=t1, rst=0, csq/vbat 真实）+ 定位上报 `121.3877673_028.3658970`；MQTT 彻底干净（关 test 页签让设备独占 clientid 后 `connected` 不再 event 5）
- ⏸ **B-4 阻塞**：Luatools 看不到 `iotArcTask ===== START =====` 日志
  - **根因**：M100PG-C2 用 **CH340 转串口**（不是 Air780 原生 USB CDC），Luatools 必须勾 ☑「**通用串口打印**」（**不是**「4G模块USB打印」）
  - 波特率依次试 **9600 → 115200 → 921600**
  - 完整修复指南见 handoff 文档 §B-4
- ⏭ B-5（test 工具发 set_relay JSON）待 B-4 通过

**绝对不要刷 LuatOS/AT/iRTU 固件**——M100PG-C2 已预装 YED_DTU3 固件（带 Lua 解释器跑 task.lua），刷其他固件会破坏 DTU 的 MQTT 透传 + config 命令 + task.lua 框架。

**阶段 C（暂缓）**：用正确 IMEI 在 iot.yinerda.com 删除重建设备 → 重新拿三要素 → 回填 `cloud/iot-platform.md`。
