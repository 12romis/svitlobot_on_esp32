# wifi.py
import network
import time

def connect_wifi(ssid: str, password: str, timeout_s: int = 15):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)

    if wlan.isconnected():
        return wlan

    wlan.connect(ssid, password)

    start = time.ticks_ms()
    while not wlan.isconnected():
        if time.ticks_diff(time.ticks_ms(), start) > timeout_s * 1000:
            raise TimeoutError("Wi-Fi connect timeout")
        time.sleep(0.2)

    return wlan


def ensure_wifi(wlan, ssid: str, password: str, timeout_s: int = 15):
    """
    Якщо Wi-Fi відвалився — перепідключаємось.
    Повертає wlan (той самий або перепідключений).
    """
    if wlan is None:
        return connect_wifi(ssid, password, timeout_s=timeout_s)

    if wlan.isconnected():
        return wlan

    # Перепідключення
    try:
        wlan.disconnect()
    except Exception:
        pass

    wlan.connect(ssid, password)

    start = time.ticks_ms()
    while not wlan.isconnected():
        if time.ticks_diff(time.ticks_ms(), start) > timeout_s * 1000:
            raise TimeoutError("Wi-Fi reconnect timeout")
        time.sleep(0.2)

    return wlan