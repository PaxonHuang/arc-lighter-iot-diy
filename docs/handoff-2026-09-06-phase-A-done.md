# Handoff · 阶段 A 完成（2026-09-06）

> 本文件是 **CLAUDE.md §5** 的补充——记录 2026-09-06 阶段 A 验收后的设备实际状态、已识别的坑、阶段 B 的精确下一步。新会话接手时**先读 CLAUDE.md，再读本文件**。

## 1. 设备当前实际状态（flash 已固化 + RAM 临时）

| 项 | 当前值 | 来源 |
|---|---|---|
| 设备 IMEI | `864865083079369`（**权威**） | 串口 `config,get,imei` 两次一致 |
| paramsrc | `2`（web 模式，阶段 B 可走 dtu.yinerda.com 部署） | 串口 A0 查询 |
| 通道 1 协议 | **mqtt**（不是 tcp——昨天 trap） | `config,set,mqtt,1,uart,120,...` |
| MQTT server | `43.139.170.206:1002`（test.yinerda.com 公共测试 broker） | A1 工具给的 |
| ClientID | `7c037f41c7703c7c74624a8bf4b469cc` | 同上 |
| Username | 同 ClientID | 同上（测试服务器怪癖，正常） |
| Password | `88888888` | 同上 |
| MQTT 心跳 | 120s | A2 配的 |
| 协议版本 | 3.1.1（字段第 10 位 = 1） | A2 配的 |
| 清除会话 | 1（测试期用） | A2 配的 |
| QOS | 订阅/发布 = 0/0 | A2 配的 |
| 设备订阅 topic | `yed/arc/down`（接收命令） | A2 自定义 |
| 设备发布 topic | `yed/arc/up`（发出应答/遥测） | A2 自定义 |
| LBS 定位 | 60s 间隔（**RAM 里**，save 后才进 flash） | 昨天的 `config,set,location,1,1,60,0,0,1` |
| netstatus,1 | `1`（已连） | A3 验收 |
| ssta | `4`（至少一路通道连接服务器成功） | A3 验收 |
| paramver | `12`（save 时 11→12） | A3 验收 |

**重要**：测试 broker 三要素 **10 分钟无交互就过期**。新会话开始时若发现浏览器端三要素已失效，**只需刷新 test.yinerda.com MQTT工具拿新三要素 + 用相同命令格式重配通道 1**（命令结构不变，只换 7 个字段值）。

## 2. 已识别 + 已修复的坑（避免重复踩）

| # | 坑 | 处置 | 文档位置 |
|---|---|---|---|
| 1 | IMEI 不一致——平台 IMEI 第 10 位错（0→6），设备 IMEI 正确 | 在 iot.yinerda.com 用正确 IMEI 重建设备（**阶段 C 做**）；阶段 A 用 test broker 不受影响 | `wiring-guide.md §1.3`、`iot-platform.md §待核对` |
| 2 | 串口配置改完不 save → 重启即丢 | **每次改完最后必发 `config,set,save`**（设备 2s 后自动重启） | `ops-guide.md §阶段A` 铁律1 |
| 3 | `paramsrc=1` 锁死 web 下发（含 task.lua 部署） | 保持默认 `2`（web）或阶段 B 前改 `0` | `ops-guide.md §阶段A` 铁律2 |
| 4 | 阶段 A MQTT 配通道 1 必须用 `config,set,mqtt,...` 不是 `tcp` | TCP 不会完成 MQTT 握手 | `ops-guide.md §阶段A` 铁律3 |
| 5 | `config,get,ttluart` 返回 error,1 是正常的——`ttluart` 是串口通道名（set 命令参数），不是可 get 的命令 | 不处理 | `ops-guide.md §阶段A` 铁律4 |
| 6 | ZVS 点火模块无反馈闭环 → relay:1 必须限时 | task.lua 已加 4s 自动关断 + relay_timeout 事件 | `task.lua v3`、`CLAUDE.md §3` |

## 3. 阶段 B · 精确下一步（5 步，按序）

新会话接手**严格按这个顺序**做，每步做完贴串口/日志/截图。

### B-1：固化 RAM 配置（30s）
```
config,set,save
```
期望 `config,save,ok` + 设备 2s 后重启。等 30s 让设备重新连 broker。

### B-2：确认链路仍通
```
config,get,netstatus,1
```
期望 `config,netstatus,ok,1`。**这条不通过就停**，先排查（多半是测试 broker 三要素过期，参考 §1 末尾说明）。

### B-3：dtu.yinerda.com 部署 task.lua（5min）
1. 浏览器打开 https://dtu.yinerda.com → 登录 → 设备管理 → 找 IMEI `864865083079369` 的设备 → 进分组
2. 任务代码编辑框 → 全选粘贴 `firmware/m100pg-c2/task.lua` 整文件 → 保存
3. **不要手动改 task.lua**——Lua 语法错会导致设备无限重启
4. 设备自动重启拉新任务 → 等 30s

### B-4：Luatools 验证 task.lua 跑起来（2min）
- USB 接 DTU（不是 UNO）→ 打开 Luatools_v3（路径 `E:\EEprojects\Bomb\Luatools_v3`）
- 期望日志看到 `iotArcTask ===== START =====`
- 看不到 = Lua 语法没过编译 = 设备进入重启循环 → 把 Luatools 完整日志截图

### B-5：浏览器 test 工具发 JSON 验证（5min）
前提：UNO 还没接没关系，先验证 task.lua 的 JSON 解析。
1. 浏览器切回 test.yinerda.com（若过期就刷新重配通道 1）
2. 工具订阅 `yed/arc/up`，发送topic 填 `yed/arc/down`
3. 内容框粘贴：
   ```
   {"cmd":"set_relay","did":"t1","param":{"sw1":1}}
   ```
   勾「回车换行」→ 发送
4. 1~3s 内订阅区应出现：
   ```
   {"cmd":"set_relay_bck","did":"t1","rst":0,"param":{"sw1":1,"csq":...,"vbat":3728}}
   ```
   did 必须原样回 `t1`
5. 4s 后订阅区应再出现：
   ```
   {"cmd":"event","did":"...","param":{"type":"relay_timeout","sw1":0}}
   ```
   这是 v3 新加的 4s 超时触发事件。

**5 步全过 → 阶段 B 链路 OK**。之后接 UNO 测 D7 物理控制。

## 4. 关键文件 / Git 状态

最新 3 个 commit（按时间）：
- `6c9fa85` docs: 解决 IMEI 阻塞项（wiring-guide §1.3 + iot-platform §待核对 + ops-guide §阶段A 铁律）
- `835e471` docs(ops-guide): 阶段 A 详细操作指南（A0~A6 字段级详解 + 浏览器↔串口协同）
- `f101184` firmware(task.lua): relay:1 超时自动关断（ZVS 安全）

权威文件清单（不要读错版本）：
- `docs/wiring-guide.md` (v3) — 硬件接线
- `docs/yinerda-platform-ops-guide.md` (含 §阶段A 铁律 + §阶段A · 详细操作指南 + §阶段B/C/D)
- `cloud/iot-platform.md` — IOT 三要素（**当前 ClientID 第10位有误，阶段 C 前必须重建**）
- `firmware/m100pg-c2/task.lua` (v3) — 已含 4s 超时
- `firmware/arduino/arc_remote/` — Arduino 固件（PlatformIO）

## 5. 新会话开场白（建议粘贴）

> "我在接手 M100PG-C2 电弧点火机项目（仓库 arc-lighter-iot-diy）。请先读 CLAUDE.md 和 docs/handoff-2026-09-06-phase-A-done.md——阶段 A 已完成（mqtt 链路通、test broker 已配、task.lua 已加超时），下一步是阶段 B：先 `config,set,save` 固化 RAM 里的 LBS 配置，再走 dtu.yinerda.com 部署 task.lua，最后用 test 工具发 set_relay JSON 验证双向链路。请告诉我你会怎么开始。"