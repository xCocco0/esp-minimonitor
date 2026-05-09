# ESP32 - MiniMonitor

Turn your ESP32 device into a multi-purpose desk clock.

#### Main features
- Displays current time
- Monitors CPU/GPU/RAM/etc sensor data from a PC
- Displays received messages/notifications

## Supported platforms

<!-- checkmark: &#9989; cross: &#10060;-->
|Board                                                                                        |Touchscreen|Gyroscope|SD Card|
|---------------------------------------------------------------------------------------------|--------|--------|--------|
|[waveshare_c6_lcd_1_47](https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47)                    |&#10060;|&#10060;| &#9989;|
|[waveshare_s3_touch_lcd_1_47](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.47)        | &#9989;|&#10060;| &#9989;|
|[waveshare_s3_touch_lcd_2](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2)              | &#9989;| &#9989;| &#9989;|

For more info on porting new hardware, see [PORTING.md](docs/PORTING.md).

## Quickstart

#### MiniMonitor Connector

For a GUI-only user experience I developed a python QT app to handle the most common tasks. It also collects sensor data
from the PC and posts the data to the device.

Check out the [MiniMonitor Connector](https://github.com/xCocco0/minimonitor-connector) repo for additional information.

#### Network

To initialize the WiFi connection and enable SNTP time synchronization, access the CLI via the serial interface and execute:

```sh
setwifi -s YOUR_SSID -p YOUR_PASSWORD
```

The system will periodically trigger an SNTP sync. To force an immediate synchronization, use:

```sh
sntpsync
```

The device implements mDNS and will be reachable at `minimonitor.local`. To change the hostname (useful if have more
than one device), run the following (applies after a reboot):

```sh
hostname YOUR_HOSTNAME
```

#### Offline Usage

If you wish to use the device fully offline, you can suppress WiFi connectivity warnings by clearing the credentials:

```sh
setwifi -s ""
```

Time must then be set manually via the `date` command.

#### Timezone Configuration

Time is formatted via [localtime](https://man7.org/linux/man-pages/man3/localtime.3p.html). Set your [timezone string](https://man7.org/linux/man-pages/man3/tzset.3.html) using:

```sh
tzset YOUR_TZ
```

*Note: Linux users can retrieve their current string from the last line of `/etc/localtime`.*

#### Notifications

You can send notifications to the Minimonitor using either the CLI or HTTP interface. See
[CLI_REFERENCE.md](docs/CLI_REFERENCE.md) and [HTTP_REFERENCE.md](docs/HTTP_REFERENCE.md) for the detailed APIs.

#### PC sensor data

Before receiving raw data, you should enable and customize the appearance of the meter bars to fit your data format. You
can find a guide on customization in [METERBAR.md](docs/METERBAR.md).

Configurations are stored in non-volatile storage and persist over reboots.

The device can now receive data either on the CLI or HTTP interface. The process of gathering and sending sensor
readings is handled by 3rd party applications: in this repo we just wait for data and display it.

For further information see [CLI_REFERENCE.md](docs/CLI_REFERENCE.md) and [HTTP_REFERENCE.md](docs/HTTP_REFERENCE.md).

#### User interface

The interface is organized into a tiled view*:
- **Main tile**: Shows the clock, notification icons and sensor information
- **Notification tile** (left): Allows to see notification text and snooze notifications
- **Calendar tile** (right): if enabled (`CONFIG_MM_UI_USE_CALENDAR=y`), adds the LVGL calendar widget
- **Settings tile**: (top): adjust brightness and shows the following control buttons:
  - Shutdown: enters deep sleep mode
  - Restart: restarts the system
  - Do not disturb: disables notification animations and hides notification tray
  - Automatic rotation: enables/disables automatic rotation based on gyroscope data

*some features may be disabled depending on the target device.

Runtime UI customization is available via the `config` command. See [CONFIG.md](docs/CONFIG.md).

## Building

You can build the project using the **esp-idf** toolchain. You need to pass the `MM_TARGET` variable to idf.py commands to
select your target board.

```sh
idf.py set-target esp32xY # replace with your chip
idf.py -DMM_TARGET=your_board_here build # replace with your board
```

Flash with:

```sh
idf.py flash -p PORT
```

KConfig defaults are dependent on the target board, if you wish to fine tune the configuration options you can run:

```sh
idf.py -DMM_TARGET=your_board_here menuconfig
```

For Visual Studio Code users, you can also set the `MM_TARGET` environment variable in `.vscode/settings.json` by adding:
```json
"idf.customExtraVars": {
    "MM_TARGET": "your_board_here"
}
```
