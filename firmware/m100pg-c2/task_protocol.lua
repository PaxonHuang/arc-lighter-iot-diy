--[[
task_protocol.lua
银尔达 IOT 物模型协议解析模块(纯函数,便于测试)
协议详见 docs/superpowers/specs/2026-07-26-camp-heater-iot-stress-test-design.md §4.3
]]

local M = {}

-- 解析云端下发的 set_relay 命令
-- 输入: JSON 字符串
-- 输出: table {ch, state, pulse_ms} 或 nil(无效)
function M.parse_set_relay(json_str)
    -- 在真实 DTU 中使用 json.decode
    -- 这里以伪代码表示;集成时调用合宙 json API
    local j = json.decode(json_str)
    if not j or j.cmd ~= "set_relay" then return nil end
    local p = j.param
    if not p or not p.ch or not p.state then return nil end
    return {
        ch = p.ch,
        state = p.state,                    -- "on"/"off"/"pulse"
        pulse_ms = p.pulse_ms or 1000
    }
end

-- 构造设备应答 JSON
function M.build_ack(cmd, did, rst, param)
    local jb = {cmd = cmd, did = did, rst = rst, param = param}
    return json.encode(jb)
end

-- 构造遥测上报 JSON
function M.build_tele(did, param)
    local jb = {
        cmd = "dup",
        did = did,
        ts = os.time() * 1000,
        param = param
    }
    return json.encode(jb)
end

-- 校验参数合法性
function M.is_valid_relay_state(state)
    return state == "on" or state == "off" or state == "pulse"
end

return M
