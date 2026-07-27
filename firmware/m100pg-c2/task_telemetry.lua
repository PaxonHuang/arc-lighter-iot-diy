--[[
task_telemetry.lua
周期遥测与上报模块
]]

local M = {}

-- 生成唯一 did(基于时间戳 + 自增计数)
local did_counter = 0
function M.gen_did()
    did_counter = did_counter + 1
    return tostring(os.time()) .. string.format("%04d", did_counter % 10000)
end

-- 构造完整遥测数据点
function M.build_payload(relay_state, arc_count)
    return {
        sw1 = relay_state,                 -- 电弧开关(0/1)
        vbat = PerGetVbattV(),             -- 电池电压 mV
        csq = mobile.csq(),                -- 信号强度
        arc_count = arc_count,             -- 点火次数累计
        imei = mobile.imei()               -- 设备 IMEI
    }
end

-- 主动上报(银尔达物模型 dup 命令)
function M.upload_tele(nid, relay_state, arc_count)
    local param = M.build_payload(relay_state, arc_count)
    local s = json.encode({cmd = "dup", did = M.gen_did(), param = param})
    if s and 1 == PronetGetNetSta(nid) then
        -- 银尔达物模型下,数据点自动同步,这里简化为直接发送
        PronetSetSendCh(nid, {1, s})
        return true
    end
    return false
end

return M
