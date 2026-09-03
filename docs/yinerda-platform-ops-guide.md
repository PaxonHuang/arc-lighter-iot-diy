# 银尔达平台配置与联调操作指南（M100PG-C2 电弧点火机）

> 2026-08-30 更新。基于 `Air780-YED-M100PG-C2-doc/` 官方文档（DTU固件实例讲解、任务和数据模板规范/调试方法、物模型MQTT协议、IOT平台介绍、常供电定位器案例）整理，结合本项目当前进展（远程开关 10 循环稳定性已通过，见 `docs/experiment-log.md`）。

> **2026-08-31 实测修订（接线指南 v2）**：`config,set,doout,1,1` 返回 `error,2`——M100PG-C2 无 DO 硬件资源，**继电器改由 Arduino D7 驱动**，task.lua 已移除 PerSetDo；MQTT 下发 config 命令手打字面 `\r\n` 会造成 `error,1`（格式错），换行交给测试工具的"回车换行"选项。详见 `docs/wiring-guide.md` §1；IOT 平台三要素与 topic 见 `cloud/iot-platform.md`。PlatformIO 迁移已完成：`firmware/arduino/arc_remote/` 现为 PIO 工程（`pio run -t upload`），调试日志走 D5 SoftwareSerial，板载 USB(D0/D1) 留给 DTU 串口。

> **2026-09-02 修订（接线指南 v3）**：① **蜂鸣器功能全部移除**（固件不再使用 D8）。② 高压部分按商供原理图 + BOM 重写为 **DC12V 输入的 ZVS 推挽自激振荡升压**模块（旧"XL6009→D880 单管、24V 母线"作废）；ZVS 无反馈闭环 → **单次点火须限时（≤3~5s）、软件做超时自动关断、断电后对高压电容放电**。当前权威文档：`docs/wiring-guide.md`(v3)、`cloud/iot-platform.md`、`hardware/bom.csv`。`docs/superpowers/` 初版计划中的 doout/XL6009/蜂鸣器等描述已作废，仅作历史参考。

## 0. 三个官方站点/工具的角色（先分清，避免走错门）

| 站点/工具 | 作用 | 是否收费 |
|---|---|---|
| https://dtu.yinerda.com | **DTU 配置平台**：给设备下发网络通道参数（服务器地址、topic、串口参数）、任务 Lua 代码、数据模板 | 免费 |
| http://test.yinerda.com | **测试服务器**：内置 "MQTT测试工具"，一键获取测试用 服务器地址/端口 + 三要素；还提供 TCP/UDP 测试工具。10 分钟无交互自动回收资源，刷新重取 | 免费（仅测试） |
| https://iot.yinerda.com | **IOT 业务平台**：产品/数据点/设备管理、Web 控制界面、小程序 UI、数据存储与轨迹 | 收费 |
| YEDTestTools（`E:\EEprojects\Bomb\YEDTestTools_v1.1.260210`） | 串口调试工具；`Project/*.ysp` 是官方命令序列工程，可直接导入一键发送 | 免费 |
| Luatools_v3（`E:\EEprojects\Bomb\Luatools_v3`） | 观察设备日志、校验任务 Lua 语法、固件升级（必备调试工具） | 免费 |

**关键认知**（来自《银尔达IOT平台介绍》）：IOT 平台与 DTU 配置本质无关。DTU 只是按协议往任意服务器上报/接收数据；IOT 平台是消费方。所以即使不开通 IOT 平台，用 test.yinerda.com 的 MQTT 测试工具 + 自定义任务也能完成全部功能验证。

## 1. 官方案例取精去糟（对本项目的结论）

### 取精（已吸收/应遵守）
1. **认证与心跳参数是硬性约定**：Air780 系列 MQTT 心跳固定 240s、协议版本只能 3.1.1、QOS 只支持 0、clean session=1。配置时字段不能有空格。
2. **参数更新确认方法**：保存参数后断电重启等 20–30s；单台设备看分组里"未更新设备数量=0"，多台看"分组参数版本 == 设备参数版本"。
3. **任务调试铁律**（《任务和数据模板调试方法》）：代码先在本地编辑器写好，**一次性**粘贴到浏览器；测试期用一台设备单独分组，通过后再批量；任务必须 `while true + sys.wait(≥100ms)`，否则占死 CPU 导致设备崩溃重启。
4. **配置文件上限 50KB**；Air780 用 Lua 5.3，Air724 用 5.1，API 不兼容（本项目 Air780EPM，参考案例时只看 Air780 系）。
5. **物模型消息 ID（did）机制**：每条消息带 did，应答必须原样回带 did；服务器主动下发最大重发 3 次、间隔 5s，全超时判离线并断开。设备离线判定：MQTT 断开，或连接中 5 分钟无任何交互。
6. **config 快速验证命令**（远程控制开关需在基本参数中手动打开）：
   - `config,set,doout,1,1\r\n` 开继电器1；`config,set,doout,1,0\r\n` 关
   - `config,get,imei` / `config,get,netstatus,1` / `config,set,reboot`
   - 多 topic 时，下发控制命令只能用**第一个订阅 topic**，应答走第一个发布 topic。
7. **API 参考**：官方案例确认 Air780 上可用 `mobile.csq()`、`PerSetDo()`、`PronetGetNetSta/PronetSetSendCh/PronetGetRecChAndDel`、`UartSetSendCh/UartGetRecChAndDel`、`PerGetVbattV`。本任务用到的 API 与《常供电定位器案例》一致，结构可放心沿用。API 细节可在 `YEDTestTools_v1.1.260210\LuaEnv\core\`（csuart.lua、cstcpc.lua、sys.lua 等）里查签名。
8. **.ysp 工程即命令模板**：`Project\1-Air系列DTU透传固件工程\104/204-MQTT...ysp` 是可导入 YEDTestTools 的发送序列（含 `config,get,imei`、topic 设置等），联调时导入即用，不必手敲。

### 去糟（对我们不适用的部分）
1. 官方 demo 大量面向 Air724 / RS485 / Modbus / 多通道（nid=1,2）场景，本项目单通道单串口，无需引入。
2. 《MQTT远程控制DTU资源实例》的 config 直控方式绕过了任务逻辑，无法做点火计数、电压遥测和 Arduino 同步——只作为链路通断验证，不作为最终方案。
3. 《常供电定位器案例》的 GPS/围栏部分与本项目无关，只借鉴其任务骨架（初始化停止默认收发 → while 循环 → 收下行 → 收串口 → 周期上报）。
4. 官方物模型标准命令是 `dreg/sset/sget/dup/ssetbck/sgetbck`；本项目任务里用的是自定义 `set_relay/set_relay_bck/dup/event`。因为 IOT 平台数据点是我们自建的，自定义命令可行；但若将来想直接复用平台内置 UI 联动（sw1 数据点直接下发），需把下行解析改成标准 `sset`（`{"cmd":"sset","did":"..","param":{"sw1":1}}`）并回 `ssetbck`。当前阶段保持自定义协议即可，切换成本低。

### 本仓库现有代码的小问题（顺手记录）
- `firmware/m100pg-c2/task_protocol.lua`、`task_telemetry.lua` 是"纯函数便于测试"的副本，真正部署只粘贴 `task.lua`（文件头已注明"已整合所有逻辑"）。注意两份文件里 `parse_set_relay` 用的字段（ch/state/pulse_ms）与 task.lua 实际协议（param.sw1）**不一致**，测试用例维护时以 task.lua 为准，建议后续统一或删除旧模块。
- task.lua 遥测周期 5s 偏密（配 IOT 平台收费/流量无谓消耗）；演示期无妨，长期运行建议 60s 并在 5 分钟内至少有一次交互以维持在线判定。

## 2. 分阶段操作步骤

> **串口直配三条铁律（2026-09-03 实测教训，务必遵守）**：
> 1. **改完参数最后必须 `config,set,save`**——手册《基本命令》§9："设置参数后，最后一条命令是保存，必须保存后前面的命令才生效"。save 会触发设备自动重启。漏掉 save = 白配（断电即丢）。
> 2. **`paramsrc` 别设成 1**。手册 §13：paramsrc=1 → "设备将不再去服务器请求数据"（纯串口模式），这会**锁死 dtu.yinerda.com 的 web 下发**（含任务 Lua、网络通道参数）。联调期用 `config,set,paramsrc,0`（串口+web 都可）或干脆不动它（出厂默认 2=web），改完 save。
> 3. **阶段 A 走 MQTT 就用 `config,set,mqtt,...`，不是 `config,set,tcp,...`**。tcp 与 mqtt 是两种协议命令；MQTT 测试需要 clientId/user/pass/订阅topic/发布topic 等字段（见手册《网络协议命令》§6 示例）。只配 tcp 指向裸 broker 端口不会完成 MQTT 握手。
> 4. 附：`config,get,ttluart` 返回 error,1 属正常——ttluart 是**串口通道名**（用于 set tcp/mqtt 的第二参数），本身不是可 get 的配置项；error,1 = "本设备不支持该命令"。M100PG-C2 主串口在 set 命令里填 `uart`。

### 阶段 A：不依赖 IOT 平台，先打通 MQTT 链路（推荐下一步从这里开始）
1. 浏览器打开 http://test.yinerda.com → "MQTT测试工具" → "打开"，记下：服务器地址/域名、端口、ClientID、Username、Password（三要素仅测试用，10 分钟无交互失效，重刷新重取）。
2. 浏览器打开 https://dtu.yinerda.com，登录 → 设备管理确认设备已添加、已建分组并分配（首次使用看《WEB配置入门教程》）。
3. **基本参数**页：勾选/打开"远程控制命令"→ 保存（默认关闭，不开则 config 命令无效）。
4. **网络通道参数**页：协议选 MQTT；填测试服务器地址、端口、三要素；订阅/发布 topic 各填一个（如 `yed/arc/down`、`yed/arc/up`）；心跳保持默认（Air780 固定 240s）；协议版本 3.1.1。保存后**断电重启**，等 20–30s。
5. 在分组页确认"未更新设备数量 = 0"（或设备参数版本==分组参数版本）。
6. 回 test.yinerda.com 的 MQTT 测试工具：设备连上后能看到注册包和连接信息；在订阅 topic 下发 `config,get,imei\r\n`（工具先勾选"回车换行"），应答回到发布 topic → 链路 OK。
7. 再发 `config,set,doout,1,1\r\n` → 听继电器吸合；`config,set,doout,1,0\r\n` → 释放。这验证了 DTU 本地 GPIO 通道（接继电器/电弧模块的一路）。

### 阶段 B：部署任务（边缘逻辑 + 自定义协议）
1. 本地编辑器打开 `firmware/m100pg-c2/task.lua`，按需调整遥测周期后**整文件**粘贴到 DTU 配置平台的分组任务代码框（function/end 首尾格式保持，前后不留空行空格）。
2. 保存参数 → 断电重启 → 确认参数版本已更新。
3. 用 Luatools 观察日志：设备 USB 接电脑（Win10 免驱），Luatools 打开日志，应看到 `iotArcTask ===== START =====`、mqtt rx / uart rx 打印。若启动即重启，多半是 Lua 语法错误——可先用 Luatools 项目管理新建工程 + Air780E CORE 校验语法（《调试方法》§3 的方法）。
4. 用 MQTT 测试工具下发 `{"cmd":"set_relay","did":"t1","param":{"sw1":1}}` 到订阅 topic：
   - 继电器吸合、`set_relay_bck`（did=t1, rst=0）出现在发布 topic；
   - 每 5s 收到一条 `dup` 遥测（sw1/vbat/csq/arc_count）。
   注意：测试工具的 topic 下发是"该工具订阅了设备的发布 topic"才能收到应答；工具里把设备订阅 topic 和发布 topic 都填上。

### 阶段 C：上 IOT 平台（要正式 UI/数据存储时再做）
1. https://iot.yinerda.com 登录 → 产品管理 → 创建产品（协议 MQTT 3.1.1）→ 按 `cloud/datapoints.md` 建 5 个数据点（sw1/vbat/csq/arc_count/btn_event）。
2. 设备管理 → 添加设备 → 记录正式三要素。
3. DTU 配置平台把通道参数换成 IOT 平台地址与正式三要素，保存重启更新。
4. IOT 平台设备列表应显示在线，数据点随 5s 遥测刷新；Web 端切换 sw1 → 下发 `sset`（若仍用自定义协议，需在平台侧配置下发格式或保持阶段 B 的 MQTT 工具验证方式）。
5. 可选：用平台 UI 编辑器做开关/电压显示界面（参考《常供电定位器案例》的小程序 UI 流程）。

### 阶段 D：回归验收
按 `docs/ACCEPTANCE.md` 勾选；重点补做：断网 30s 自动重连、5 分钟静默离线判定、低电压（vbat<3.3V）上报告警路径。

## 3. Arduino 侧：要不要迁移 PlatformIO？—— 建议：是，低成本高收益

现状是 Arduino IDE + UNO。迁移 PlatformIO 的理由：
- 你已能用 `pio` CLI（6.1.19），固件可纳入本仓库与 pytest 串口测试（`test/test_arduino_serial.py`）同一工作流，`pio run -t upload` 一条命令完成编译烧录；
- 依赖声明化（platformio.ini 进 git），换电脑可复现；
- 未来若换 STM32，PlatformIO 是现成的跨平台通道，Arduino IDE 则要另起炉灶。

迁移步骤（约 15 分钟）：
1. 在 `firmware/arduino/arc_remote/` 下建 `platformio.ini`：
   ```ini
   [env:uno]
   platform = atmelavr
   board = uno
   framework = arduino
   monitor_speed = 115200
   upload_port = COM5        ; 按实际改
   ```
2. 把现有 `.ino` 移入 `src/main.cpp`（或保留 `src/*.ino` 亦可），顶部如缺 `#include <Arduino.h>` 补上。
3. `pio run`（编译）→ `pio run -t upload`（烧录）→ `pio device monitor`（看串口）。
4. 验证与 Arduino IDE 烧录行为一致后，Arduino IDE 即可退役。原 `.ino` 目录保留一份或打 tag 存档。

## 4. Demo 成功后要不要换 STM32F103C8T6？—— 建议：没有必要

理由：
1. **架构上 MCU 已被"分工"**：联网、协议解析、遥测在 M100PG-C2 的 Lua 任务里完成；**继电器驱动（D7）+ 本地点动按键 + 状态 LED**由 UNO 协控（已无蜂鸣器）。STM32 解决不了任何当前瓶颈。
2. **F103C8T6 的短板反而添麻烦**：无 USB（需 ST-Link/串口烧录）、3.3V 电平（还要处理与 5V 外设的电平匹配）、生态迁移+重接线+重写固件，对"户外点火器"功能零增益。
3. **什么时候才值得换 MCU**：需要高精度时序（点火脉宽 ms 级控制）、多路传感器融合、低功耗深睡（UNO 做不到，F103 也一般，真要省电应优先用 DTU 自身的超低功耗休眠模式：46µA，见 M100PG-C2.md §2.3）或抗干扰隔离设计。届时也建议选带 USB-CDC 的型号（如 STM32F072/G071/CH32V203）而非古老的 F103C8T6。
4. 更划算的演进路径：① 把 UNO 的协控逻辑也搬进 Lua 任务（官方《IOT关于任务教程的说明》"DTU/RTU+平台"方案），MCU 可整体去掉；② 需要本地安全互锁时再加一颗小 MCU，且只做"允许点火"与"禁止点火"的硬线逻辑。

## 5. 参考索引
- 本地官方案例：`Air780-YED-M100PG-C2-doc/DTU固件实例讲解/MQTT远程控制DTU资源实例.md`（三要素获取、参数配置全流程截图）
- 物模型协议：`Air780-YED-M100PG-C2-doc/通讯协议和数据格式/1、IOT物模型-MQTT通讯协议.md`
- 任务规范/调试：`Air780-YED-M100PG-C2-doc/Web配置工具/DTU固件任务和数据模板使用/`
- 任务 API 源码参考：`E:\EEprojects\Bomb\YEDTestTools_v1.1.260210\LuaEnv\core\`
- 官方命令序列：`E:\EEprojects\Bomb\YEDTestTools_v1.1.260210\Project\1-Air系列DTU透传固件工程\*.ysp`
- 项目 SPEC：`docs/superpowers/specs/2026-07-26-camp-heater-iot-stress-test-design.md`
