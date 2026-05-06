# Minimonitor Command Reference

Reference documentation for Minimonitor CLI commands.

## Commands
- [backlight](#backlight)
- [config](#config)
- [date](#date)
- [help](#help)
- [hostname](#hostname)
- [log](#log)
- [notify](#notify)
- [reboot](#reboot)
- [rotate](#rotate)
- [sensorconfig](#sensorconfig)
- [setsensor](#setsensor)
- [setwifi](#setwifi)
- [snooze](#snooze)
- [sntpsync](#sntpsync)
- [tzset](#tzset)
- [wifistatus](#wifistatus)

---

### `backlight`
Adjusts the physical screen brightness.
- **Usage:** `backlight [<value>]`
- `<value>`: `0` for minimum brightness, `255` for maximum.

### `config`
Modify or view internal configuration entries.
- **Usage:** `config [-ar] <config_name> [<new_value>]`
- `-a`, `--all`:  Print all config entries
- `-r`, `--restore-default`:  Restore default value
- `<config_name>`:  Config entry name
- `<new_value>`: Value to set

Specify `config_name` and `new_value` to write to config. If `-r` is used, the
config entry is instead reset to its default value.

If `new_value` is omitted, the current config value is printed. Use the `-a` option to print
a list of all config entries.

Some config entries are only updated after a reboot and further calls to `config` will show the previous value until the
system is rebooted.

- **Example:** `config COLOR_BACKGROUND` prints the current background color.
- **Example:** `config COLOR_BACKGROUND 0xff0000` sets the background color as red (hex values are automatically converted).
- **Example:** `config -r COLOR_BACKGROUND` restores background color to default.

For more information on config entries see [CONFIG.md](config.md).

### `date`
View or manually set the system clock.
- **Usage:** `date [-s MMDDhhmm[[CC]YY][.ss]]`
- `-s`, `--set`:  Set time according to string



- **Example:** `date -s 122512002023` (Dec 25, 12:00 PM, 2023).

### `help`
Displays a summary of all registered commands. 
- **Usage:** `help [<command>] [-v <0|1>]`
- `<string>`: Name of command
- `-v`, `--verbose=<0|1>`: If specified, list console commands with given verbose level

Use `help <command>` for specific argument details.

### `hostname`
Sets or reads the device hostname (e.g., `minimonitor.local`). This value will be used both for MDNS and TTY prompt.
- **Usage:** `hostname [<hostname>]`

If `hostname` is specified, update new hostname, otherwise print current hostname.

### `log`
Adjusts the verbosity of system logs. 
- **Usage:** `log [-newidva] [-l <int>] [<module name>]`

If `module_name` is specified, only modify this module verbosity level; otherwise, apply to all modules.

The log level can be specified as a numeric value through the `-l` flag or by specific the respective flag.
The following are equivalent:

|        |      |             |
|--------|------|-------------|
| `-l 0` | `-n` | `--none`    |
| `-l 1` | `-e` | `--error`   |
| `-l 2` | `-w` | `--warn`    |
| `-l 3` | `-i` | `--info`    |
| `-l 4` | `-d` | `--debug`   |
| `-l 5` | `-v` | `--verbose` |
| `-l 6` | `-a` | `--all`     |


- **Note:** Currently, only system-wide log level changes persist over reboots.

### `notify`
Sends a popup notification to the display.
- **Usage:** `notify [-AFR] [-i <icon>] [-n <name>] [-d <ms>] [-c <hex>] [<message>]`
- `-i`, `--icon=<string>`: Icon name
- `-n`, `--name=<string>`: Notification name
- `-d`, `--duration=<int>`: Notification duration in ms (defaults to `CONFIG_MM_NOTIF_DEFAULT_DURATION`)
- `-c`, `--color=<hex>`: Notification theme color in RGB format
- `-A`, `--append`: Always append a new icon in notification tray
- `-R`, `--replace`: Replace previous icon in notification tray
- `<message>`: Message to display

This command will trigger:
- a popup on the screen if a message is provided,
- a colorful animation if a color is specified,
- a notification icon in the notification tray and an entry in the notifications board (page on the left) if an icon is
provided or a notification name is specified.

If the user specifies a duration (`-d` argument) equal to 0, the popup and colorful animations are disabled, regardless
of the color and message arguments.

The default behaviour of the notification tray is find a notification with the same name and add +1 on the number of the
notification. This behaviour can be modified by specifing the `-R` and `-A` arguments. Please note that this argument
has no effect on the notification board. 

- **Example:** `notify "Hello world"` shows an "Hello world" popup, without posting any modification.
- **Example:** `notify -i envelope -c 0xffff00 -n email "New email received"` posts a notification with respective popup
and a yellow animation.
- **Example:** `notify -R -i cake -n bday "John's birthday"` posts a notification without triggering any animation. If the
same command is repeated, it has no effect (unless the notification is snoozed).

### `reboot`
Restarts the device immediately.

### `rotate`
Sets the screen orientation.
- **Usage:** `rotate [<auto|degrees>]`
- `<auto|degrees>`  Rotation mode. Write "auto" to enable automatic rotation, otherwise specify screen rotation in degrees.

When a rotation is specified, automatic rotation will also be disabled.

### `sensorconfig`
Defines the visual behavior for a sensor reading received through either `setsensor` command or the respective HTTP API.
- **Usage:** `sensorconfig <index> <label> [<format string>] [-M <int>] [-d <int>] [-t <int>] [-C <int>] [-W <int>] [-S <int>] [-F <format string>] [-c <int>]... [-s <int>] [-s <int>] [-a <int>] [-wp]`
- `<index>`: Index of the meter to configure
- `<label>`: Label to display inside the meter bar
- `<format string>`: Format string to display side value
- `-M`, `--max=<int>`: Maximum value for the meter bar
- `-d`, `--decimals=<int>`: Number of decimal digits
- `-t`, `--timeout=<int>`: Timeout in milliseconds
- `-w`, `--weak`: Hide meter when no other is shown
- `-p`, `--use_panel`: Show panel with meter
- `-C`, `--color_sensor=<int>`: Which sensor to use to drive meter colors
- `-W`, `--wakeup_sensor=<int>`: Which sensor to use to reset timeout
- `-S`, `--panel_threshold=<int>`: Value threshold for showing the panel
- `-F`, `--panel_fmt=<format string>`: String format for panel value
- `-c`, `--color=<int>`: Colors for the meter bar
- `-s`, `--threshold=<int>`: Value threshold for switching color
- `-a`, `--wakeup_threshold=<int>`: Value threshold for waking up the meter

For more information on how to customize the meters, see [METERBAR.md](METERBAR.md).

### `setsensor`
Sends sensor data to a configured meter.
- **Usage:** `setsensor <index> <value1> [<value2>] [<value_panel>]`
- `<value1>`: Value to write on the meter
- `<value2>`: Second argument to use in format string
- `<value_panel>`: Value to write on the panel

For more information on how to customize the meters, see [METERBAR.md](METERBAR.md).

### `setwifi`
Configures the WiFi station credentials.
- **Usage:** `setwifi [-s <ssid>] [-p <password>]`
- `-s`, `--ssid=<ssid>` WiFi station SSID
- `-p`, `--password=<password>`: WiFi station password

If you want to use the device fully offline, you can set an empty SSID to suppress warnings. No attempt to connect will
be made and not alert will be shown on the device.

### `snooze`
Silences or hides active notifications.
- **Usage:** `snooze [-n <name>]`
- `-n`, `--notification=<string>`: Name of the notification to snooze

If no name is specified all notifications are snoozed.

- **Note:** persistent notifications (i.e. no-wifi alert) will only be cleared if snoozing by their name.

### `sntpsync`
Manually triggers a time synchronization with the SNTP server.

### `tzset`
Sets the timezone for local time adjustments.
- **Usage:** `tzset [<tz>]`
- `<tz>`: Timezone to use in TZ format

If `tz` is not specified, prints current TZ variable.

For linux users, you can retrieve your current system tz in `/etc/localtime` (last line).

### `wifistatus`
Currently not implemented.