# main.py
import time
import ubinascii
import machine
from status_led import StatusLED

from config import (
    WIFI_SSID, WIFI_PASSWORD,
    SERVER_BASE_URL, PING_PATH, API_KEY,
    PING_INTERVAL_S, WIFI_CONNECT_TIMEOUT_S,
    FAKE_REQUESTS
)

from wifi import ensure_wifi
from client import get_with_params


def build_url(base: str, path: str) -> str:
    if base.endswith("/") and path.startswith("/"):
        return base[:-1] + path
    if (not base.endswith("/")) and (not path.startswith("/")):
        return base + "/" + path
    return base + path


def device_id() -> str:
    # Стабільний ідентифікатор девайса (MAC/UID) у зручному вигляді
    return ubinascii.hexlify(machine.unique_id()).decode()


def sleep_ms_cooperative(total_ms: int, step_ms: int = 200):
    """
    Не спимо одним великим sleep(60).
    Це дає змогу потім легко додати:
    - перевірки станів
    - watchdog
    - реакцію на кнопки/сенсори
    """
    end = time.ticks_add(time.ticks_ms(), total_ms)
    while time.ticks_diff(end, time.ticks_ms()) > 0:
        time.sleep_ms(step_ms)


def main():
    url = build_url(SERVER_BASE_URL, PING_PATH)
    boot_ms = time.ticks_ms()
    led = StatusLED(pin=2)

    wlan = None
    print("Boot OK. Device:", device_id())
    print("Ping URL:", url)

    while True:
        cycle_start = time.ticks_ms()

        # 1) Wi-Fi (підтримка підключення)
        try:
            wlan = ensure_wifi(wlan, WIFI_SSID, WIFI_PASSWORD, timeout_s=WIFI_CONNECT_TIMEOUT_S)
            led.on()   # Wi-Fi є
        except Exception as e:
            print("Wi-Fi error:", repr(e))
            led.blink(times=4)  # Wi-Fi немає
            # Якщо Wi-Fi не піднявся — почекаємо і спробуємо знову
            sleep_ms_cooperative(5_000)
            continue
        print("WIFI rssi:", wlan.status("rssi"))

        # 2) GET-запит
        try:
            if FAKE_REQUESTS:
                status, text = 0, "Fake request"
            else:
                status, text = get_with_params(url, {"channel_key": API_KEY})
            print("GET", status, text)
            
            if status == 0:
                led.blink(times=1)
            elif status == 429:
                led.blink(times=2)
            elif status >= 500:
                led.blink(times=5)
            led.on()
        except Exception as e:
            print("HTTP error:", repr(e))
            led.blink(times=5)
            led.on()

        # 3) Чіткий інтервал ~ раз на хвилину
        print("uptime_s:", time.ticks_diff(time.ticks_ms(), boot_ms) // 1000)
        print("ts:", time.ticks_ms())
        print("")
        elapsed_ms = time.ticks_diff(time.ticks_ms(), cycle_start)
        wait_ms = max(0, PING_INTERVAL_S * 1000 - elapsed_ms)
        sleep_ms_cooperative(wait_ms)


if __name__ == "__main__":
    main()