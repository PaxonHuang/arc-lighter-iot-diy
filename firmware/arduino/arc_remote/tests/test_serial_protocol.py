"""
测试 Arduino 串口接收 DTU 命令的协议解析逻辑
真实实现在 arc_remote.ino::loop()
"""
import pytest

def parse_dtu_command(line: str):
    """协议解析等价代码"""
    line = line.strip()
    if line == "relay:1":
        return ("relay", True)
    if line == "relay:0":
        return ("relay", False)
    if line.startswith('{"evt":'):
        return ("event", line)
    return ("unknown", None)


def test_relay_on_command():
    assert parse_dtu_command("relay:1\n") == ("relay", True)


def test_relay_off_command():
    assert parse_dtu_command("relay:0\n") == ("relay", False)


def test_event_command():
    result = parse_dtu_command('{"evt":"btn_toggle"}\n')
    assert result[0] == "event"
    assert "btn_toggle" in result[1]


def test_unknown_command():
    assert parse_dtu_command("garbage\n") == ("unknown", None)


def test_empty_line():
    assert parse_dtu_command("\n") == ("unknown", None)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
