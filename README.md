# ESP32 MicroPython Project

This repository contains a minimal MicroPython project for an ESP32 board.

## Getting started

1. **Create a virtual environment** (already done):
   ```
   python -m venv .venv
   .\.venv\Scripts\activate
   pip install --upgrade pip
   pip install esptool mpremote
   ```

2. **Find the COM port** for your ESP32:
   - Open **Device Manager** on Windows
   - Expand **Ports (COM & LPT)** and note the `COMx` associated with your board.

3. **Flash MicroPython firmware** (replace `COMx` and `firmware.bin`):
   ```sh
   .venv\Scripts\esptool.exe --chip esp32 --port COMx write_flash -z 0x1000 firmware.bin
   ```

4. **Open the REPL** using mpremote:
   ```sh
   .venv\Scripts\mpremote connect COMx repl
   ```

5. **Upload, sync or retrieve files** with MicroPico extension:
   - Open the Command Palette (`Ctrl+Shift+P`).
   - Run **MicroPico: Configure project** and select your COM port (e.g. `COM3`).
   - After configuration the extension drops a `.micropico.json` file in the workspace, which stores the chosen port.
   - Use **MicroPico: Upload** or **MicroPico: Sync** to send `main.py`, `boot.py`, etc.
   - **To copy existing files from the board** without flashing, you can either:
     * Run the `mpremote` command line from the venv (see below), or
     * Use MicroPico's file explorer to download individual files.

   After configuration, the extension creates a `.micropico.json` or similar file in the workspace.

## Project files

- `boot.py` &ndash; executed on boot; currently minimal.
- `main.py` &ndash; blinks onboard LED and prints status.
- `.vscode/settings.json` &ndash; points to the virtual environment interpreter.
- `.gitignore` &ndash; ignores virtual environment and caches.

## Tips

- Change `LED_PIN` in `main.py` if your board uses a different GPIO for the onboard LED.
- Format-on-save is enabled in workspace settings.

Enjoy developing with MicroPython on your ESP32!