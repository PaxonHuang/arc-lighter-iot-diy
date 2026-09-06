--[[
task.lua - M100PG-C2 主任务
复制此代码到银尔达 DTU 配置平台 (https://dtu.yinerda.com) 的任务代码编辑框

依赖: 无(本文件已整合所有逻辑,实际部署只需复制本文件)
v2 (2026-08-31): 实测 M100PG-C2 无 doout 硬件资源(config,doout,error,2),
  移除 PerSetDo;继电器由 Arduino D7 驱动,本任务仅经 UART 下发 relay 命令
v3 (2026-09-06): 加入 relay:1 超时自动关断(RELAY_TIMEOUT_MS=4000)
  点火模块 ZVS 无反馈闭环,单次通电>5s 会过热烧管/磁芯,
  强制 4s 后自动 relay:0 并上报 {type="relay_timeout"} 事件
]]

local taskname = "iotArcTask"
log.info(taskname, "===== START =====")

-- 初始化
PronetStopProRecCh(1)
UartStopProRecCh(1)

local nid, uid = 1, 1
local RELAY_TIMEOUT_MS = 4000   -- relay:1 最长保持 4s(安全阈值)
local relay_state = 0
local relay_on_ms = 0           -- 0=当前未吸合;非 0=吸合时刻(os.time()*1000)
local arc_count = 0
local last_tele_ms = 0

-- did 自增
local did_counter = 0
local function gen_did()
    did_counter = did_counter + 1
    return tostring(os.time()) .. string.format("%04d", did_counter % 10000)
end

-- 发送 JSON(经 MQTT 通道)
local function send_json(s)
    if s and 1 == PronetGetNetSta(nid) then
        PronetSetSendCh(nid, {1, s})
    end
end

-- 同步状态到 Arduino
local function sync_arduino(state)
    if state == 1 then
        UartSetSendCh(uid, "relay:1\n")
    else
        UartSetSendCh(uid, "relay:0\n")
    end
end

-- 周期遥测上报
local function upload_tele()
    local param = {
        sw1 = relay_state,
        vbat = PerGetVbattV(),
        csq = mobile.csq(),
        arc_count = arc_count
    }
    local s = json.encode({cmd = "dup", did = gen_did(), param = param})
    send_json(s)
end

-- 主循环
while true do
    -- 0. 安全闸: relay:1 超时自动关断
    if relay_state == 1 and relay_on_ms > 0
       and (os.time() * 1000 - relay_on_ms) > RELAY_TIMEOUT_MS then
        log.warn(taskname, "relay timeout auto-off")
        relay_state = 0
        relay_on_ms = 0
        sync_arduino(0)
        local ev = json.encode({
            cmd = "event",
            did = gen_did(),
            param = {type = "relay_timeout", sw1 = 0}
        })
        send_json(ev)
    end

    -- 1. 处理 MQTT 下行
    local netr = PronetGetRecChAndDel(nid)
    if netr then
        log.info(taskname, "mqtt rx", netr)
        local j = json.decode(netr)
        if j and j.cmd == "set_relay" and j.param and j.param.sw1 ~= nil then
            if j.param.sw1 == 1 then
                relay_state = 1
                relay_on_ms = os.time() * 1000
                arc_count = arc_count + 1
                sync_arduino(1)
            else
                relay_state = 0
                relay_on_ms = 0
                sync_arduino(0)
            end
            -- 应答
            local ack = json.encode({
                cmd = "set_relay_bck",
                did = j.did,
                rst = 0,
                param = {sw1 = relay_state, csq = mobile.csq(), vbat = PerGetVbattV()}
            })
            send_json(ack)
        end
    end

    -- 2. 处理 Arduino 上行事件
    local uartr = UartGetRecChAndDel(uid)
    if uartr then
        log.info(taskname, "uart rx", uartr)
        local j = json.decode(uartr)
        if j and j.evt == "btn_toggle" then
            relay_state = (relay_state == 1) and 0 or 1
            if relay_state == 1 then
                arc_count = arc_count + 1
                relay_on_ms = os.time() * 1000
            else
                relay_on_ms = 0
            end
            sync_arduino(relay_state)
            -- 上报事件
            local ev = json.encode({
                cmd = "event",
                did = gen_did(),
                param = {type = "local_btn", state = relay_state}
            })
            send_json(ev)
        end
    end

    -- 3. 周期遥测 (5s)
    if os.time() * 1000 - last_tele_ms > 5000 then
        last_tele_ms = os.time() * 1000
        upload_tele()
    end

    sys.wait(100)
end
