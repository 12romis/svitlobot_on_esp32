# status_led.py
from machine import Pin
import time

class StatusLED:
    def __init__(self, pin: int = 2):
        self.pin = Pin(pin, Pin.OUT)
        self.off()

    def on(self):
        self.pin.value(1)

    def off(self):
        self.pin.value(0)

    def blink(self, times: int = 3, on_ms: int = 500, off_ms: int = 300):
        """
        Коротке блимання (сигнал помилки).
        """
        for _ in range(times):
            self.on()
            time.sleep_ms(on_ms)
            self.off()
            time.sleep_ms(off_ms)