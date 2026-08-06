"""
test_yed_iot_remote.py
模拟云端命令,验证 M100PG-C2 是否正确响应。
需要 paho-mqtt: pip install paho-mqtt
"""
import paho.mqtt.client as mqtt
import json
import time
import sys

# 银尔达 IOT 平台(项目专用)
BROKER = sys.argv[1] if len(sys.argv) > 1 else "iot.yinerda.com"
PORT = 1883
IMEI = sys.argv[2] if len(sys.argv) > 2 else "your_imei_here"
USERNAME = sys.argv[3] if len(sys.argv) > 3 else "your_username"
PASSWORD = sys.argv[4] if len(sys.argv) > 4 else "your_password"
CLIENT_ID = sys.argv[5] if len(sys.argv) > 5 else "test_remote_py"

CMD_TOPIC = f"/{IMEI}/cmd"
TELE_TOPIC = f"/{IMEI}/tele"


def on_connect(client, userdata, flags, rc):
    print(f"Connected with rc={rc}")
    client.subscribe(TELE_TOPIC)


def on_message(client, userdata, msg):
    print(f"RX {msg.topic}: {msg.payload.decode('utf-8', errors='replace')[:200]}")


def test_remote_on():
    client = mqtt.Client(client_id=CLIENT_ID)
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    time.sleep(1)

    # 发送开命令
    cmd = {
        "cmd": "set_relay",
        "did": "test001",
        "param": {"ch": 1, "sw1": 1}
    }
    client.publish(CMD_TOPIC, json.dumps(cmd))
    print(f"TX {CMD_TOPIC}: {cmd}")
    time.sleep(5)  # 等待设备应答

    # 发送关命令
    cmd["param"]["sw1"] = 0
    client.publish(CMD_TOPIC, json.dumps(cmd))
    print(f"TX {CMD_TOPIC}: {cmd}")
    time.sleep(5)

    client.loop_stop()
    client.disconnect()
    print("✅ Remote on/off command sent")


if __name__ == "__main__":
    test_remote_on()