# OTA practice: ESP32-C3 RainMaker light

This project contains two deliberately separate OTA demonstrations.

| Method | Trigger | Where the image comes from |
|---|---|---|
| Local host | `ota-local` entered on the serial console | A Python HTTP server on the computer in the same LAN |
| ESP RainMaker Cloud | A job created in the RainMaker Dashboard | RainMaker firmware storage over HTTPS/MQTT |

The partition table has two OTA application slots (`ota_0` and `ota_1`), so
the active firmware continues to run while the new image is written to the
other slot. On success the boot partition changes and the ESP32-C3 reboots.

## 1. Build and flash a baseline image

Open **ESP-IDF PowerShell** and run these commands from this directory:

```powershell
idf.py -B build_ws2812 build
idf.py -B build_ws2812 -p COMx flash monitor
```

Replace `COMx` with the ESP32-C3 port. The serial log must show:

```text
Firmware running: project=5_rainmaker, version=1.0.0
```

Complete Wi-Fi provisioning and wait for the node to be online before trying
either OTA method.

## 2. Create a visibly newer firmware image

Make a small harmless visible change, for example change a log message in
`main/app_main.c`. Then increment the version in the project `CMakeLists.txt`:

```cmake
set(PROJECT_VER "1.0.1")
```

Build it, but **do not flash it by USB**:

```powershell
idf.py -B build_ws2812 build
```

The OTA image is:

```text
build_ws2812/5_rainmaker.bin
```

Its size must be at most 1664 KB, the size of one OTA partition.

## 3. OTA through a local host

This is only for the lab. Both the computer and ESP32 must be connected to the
same Wi-Fi. The firmware accepts `http://` here so do not use it on an
untrusted or production network.

1. In another PowerShell window, serve the built `.bin` file:

   ```powershell
   cd D:\PBL5\book-esp32c3-iot-projects-main\device_firmware\5_rainmaker\build_ws2812
   py -m http.server 8000 --bind 0.0.0.0
   ```

2. Find the computer Wi-Fi IPv4 address with `ipconfig`, for example
   `192.168.1.25`. Allow Python through Windows Firewall on **Private**
   networks if Windows asks.

3. In the running `idf.py monitor` serial console, enter:

   ```text
   ota-local http://192.168.1.25:8000/5_rainmaker.bin
   ```

4. The expected log ends with `Local-host OTA successful` and, after reboot,
   `version=1.0.1`.

## 4. OTA through ESP RainMaker Cloud

The project uses `esp_rmaker_ota_enable_default()`, which is the current
Dashboard-compatible **OTA using topics** workflow.

1. The node must already be claimed, attached to your RainMaker account, and
   online.
2. Sign in to the ESP RainMaker Dashboard with an account that has firmware
   management/admin permission.
3. Go to **Firmware Images** -> **Add Image**, then upload
   `build_ws2812/5_rainmaker.bin`. Leave model/project and version detection
   enabled; the binary should report model `5_rainmaker`, version `1.0.1`.
4. Select the new image -> **Start OTA** -> select this node -> choose
   **Force Push** for the demo -> start the job.
5. Watch the Dashboard job and `idf.py monitor`. The node prints
   `RainMaker OTA: starting`, download progress, then success and reboots.

For cloud OTA, the incoming image must have the **same project name**
(`5_rainmaker`) and a **different firmware version**. A normal end-user
account in the phone app may control a node but cannot create firmware images
or OTA jobs; use a RainMaker Dashboard account with the required admin access.

## After the practice

For a production firmware, remove `CONFIG_ESP_HTTPS_OTA_ALLOW_HTTP=y` from
`sdkconfig` and `sdkconfig.defaults`, then run `idf.py fullclean build`.
Keep the RainMaker cloud OTA workflow because it uses authenticated HTTPS or
MQTT rather than an unauthenticated LAN HTTP server.
