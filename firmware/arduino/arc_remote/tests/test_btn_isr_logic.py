"""
测试 Arduino 本地点动按键逻辑的 Python 模拟实现
真实代码在 src/main.cpp(PlatformIO),这里只验证消抖/事件上报逻辑等价性。
v3 (2026-09-02): 蜂鸣器功能已移除,改用 toggle_count 统计"通过消抖的按键事件"。
"""
import pytest


class FakeArduino:
    """模拟 Arduino 环境(简化版)"""
    def __init__(self):
        self.btn_pressed = False
        self.led_state = None
        self.toggle_count = 0          # 通过消抖后发出的 btn_toggle 事件次数
        self.serial_lines = []
        self.last_press_time_ms = -1000  # 起始远早于首次按下,确保首次按键通过消抖

    def btnISR(self):
        self.btn_pressed = True

    def millis(self):
        return self._current_ms

    def set_time(self, t):
        self._current_ms = t

    def digitalWrite(self, pin, state):
        if pin == 4:  # STATUS_LED_PIN
            self.led_state = state

    def println(self, s):
        self.serial_lines.append(s)

    def loop_iteration(self, btn_active_low=True):
        """单次 loop 循环模拟"""
        if self.btn_pressed:
            now = self.millis()
            # 消抖: 距离上次按下 > DEBOUNCE_MS
            if now - self.last_press_time_ms > 50:
                self.last_press_time_ms = now
                self.toggle_count += 1
                self.println('{"evt":"btn_toggle"}')
            self.btn_pressed = False


def test_initial_state_no_press():
    a = FakeArduino()
    a.loop_iteration()
    assert a.btn_pressed is False
    assert a.toggle_count == 0


def test_single_button_press_emits_one_event():
    a = FakeArduino()
    a.set_time(0)
    a.btnISR()
    a.loop_iteration()
    assert a.toggle_count == 1
    assert '{"evt":"btn_toggle"}' in a.serial_lines


def test_debounce_suppresses_repeated_presses():
    a = FakeArduino()
    a.set_time(0)
    a.btnISR()
    a.loop_iteration()
    # 30ms 内再次按下,应被消抖
    a.set_time(30)
    a.btnISR()
    a.loop_iteration()
    assert a.toggle_count == 1   # 仍然只发了 1 次事件


def test_debounce_allows_press_after_threshold():
    a = FakeArduino()
    a.set_time(0)
    a.btnISR()
    a.loop_iteration()
    # 60ms 后再次按下,允许触发
    a.set_time(60)
    a.btnISR()
    a.loop_iteration()
    assert a.toggle_count == 2


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
