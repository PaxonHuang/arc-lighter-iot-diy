"""
test_load.py
连续 10 次远程开关,验证系统稳定性。
"""
import paho.mqtt.client as mqtt
import json
import time
import sys

# 银尔达 IOT 平台(项目专用)
BROKER = "iot.yinerda.com"
PORT = 1883
IMEI = sys.argv[1] if len(sys.argv) > 1 else "your_imei_here"
USERNAME = sys.argv[2] if len(sys.argv) > 2 else "your_username"
PASSWORD = sys.argv[3] if len(sys.argv) > 3 else "your_password"

CMD_TOPIC = f"/{IMEI}/cmd"
TELE_TOPIC = f"/{IMEI}/tele"

received_count = 0
ack_count = 0


def on_message(client, userdata, msg):
    global received_count, ack_count
    received_count += 1
    if "set_relay_bck" in msg.payload.decode('utf-8', errors='replace'):
        ack_count += 1


def main():
    client = mqtt.Client(client_id="load_test")
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_message = on_message
    client.connect(BROKER, PORT, 60)
    client.subscribe(TELE_TOPIC)
    client.loop_start()
    time.sleep(2)

    success = 0
    for i in range(10):
        state = (i % 2)
        cmd = {
            "cmd": "set_relay",
            "did": f"load{i:03d}",
            "param": {"ch": 1, "sw1": state}
        }
        client.publish(CMD_TOPIC, json.dumps(cmd))
        print(f"[{i+1}/10] {'ON' if state else 'OFF'}")
        time.sleep(3)

    time.sleep(5)
    client.loop_stop()
    client.disconnect()

    print(f"\nResults:")
    print(f"  Tele received: {received_count}")
    print(f"  ACKs received: {ack_count}")
    assert ack_count >= 8, f"Too few ACKs: {ack_count}/10"
    print(f"✅ Load test passed")


if __name__ == "__main__":
    main()