"""
test_arduino_serial.py
通过 USB 串口与 Arduino 通信,验证按键和 DTU 同步功能。
需要 pyserial: pip install pyserial
"""
import serial
import time
import sys

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM3"
BAUD = 9600


def find_arduino():
    """自动寻找 Arduino 串口"""
    import serial.tools.list_ports
    for p in serial.tools.list_ports.comports():
        if "Arduino" in (p.description or "") or "CH340" in (p.description or ""):
            return p.device
    return PORT


def test_boot():
    """验证 Arduino 上电打印 boot JSON"""
    ser = serial.Serial(find_arduino(), BAUD, timeout=2)
    ser.dtr = False
    time.sleep(0.5)
    ser.dtr = True
    time.sleep(2)
    lines = []
    while ser.in_waiting:
        lines.append(ser.readline().decode().strip())
    ser.close()
    boot = [l for l in lines if '"type":"boot"' in l]
    ready = [l for l in lines if '"type":"ready"' in l]
    assert len(boot) == 1, f"Expected 1 boot line, got {len(boot)}: {lines}"
    assert len(ready) == 1, f"Expected 1 ready line, got {len(ready)}: {lines}"
    print("✅ Arduino boot OK")


def test_relay_sync_on():
    """通过串口发送 relay:1,期望 Arduino 拉高 LED"""
    ser = serial.Serial(find_arduino(), BAUD, timeout=1)
    time.sleep(0.5)
    ser.write(b"relay:1\n")
    time.sleep(0.5)
    # 无法直接读取 LED,但可观察 Arduino 是否回发其他数据
    ser.close()
    print("✅ relay:1 sent (LED should be on)")


if __name__ == "__main__":
    test_boot()
    test_relay_sync_on()