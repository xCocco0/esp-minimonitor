# Minimonitor HTTP API Reference

Reference documentation for the Minimonitor HTTP request handlers. These endpoints allow for remote control and monitoring of the system using URL query parameters.

## Endpoints
- [GET /index.html](#get-indexhtml-or-)
- [GET /panic](#get-panic)
- [GET /coredump.bin](#get-coredumpbin)
- [GET /notify](#get-notify)
- [GET /sensorconfig](#get-sensorconfig)
- [GET /setconfig](#get-setconfig)
- [GET /setsensor](#get-setsensor)
- [GET /snooze](#get-snooze)
- [WebSocket /ws](#websocket-ws)

---

### `GET /index.html` (or `/`)

The main HTML dashboard.

Also shows sensor meter bars and has a section for modifying config options.

### `GET /panic`

Displays a report about the last system crash.

Provides the reset reason, crashed task name, and the log history.

### `GET /coredump.bin`

Downloads the binary core dump from the last system crash.

### `GET /notify`
Triggers a popup notification on the physical display.
- **Usage:** `/notify?name=<string>&m=<string>&icon=<string>&duration=<int>&color=<hex>&replace=<bool>&stack=<bool>&append=<bool>`
- `name`: Notification name
- `m`: Message to display
- `icon`: Icon name
- `duration`: Notification duration in ms (defaults to `CONFIG_MM_NOTIF_DEFAULT_DURATION`)
- `color`: Color in RGB format
- `replace`: Set to `true` or `1` to replace the previous icon in the tray
- `stack`: Set to `true` or `1` to stack notifications
- `append`: Set to `true` or `1` to always append a new icon

### `GET /sensorconfig`
Defines the visual behavior for a sensor meter.
- **Usage:** `/sensorconfig?i=<int>&lbl=<string>&fmt=<string>&pfmt=<string>&max=<int>&t=<int>&c0=<hex>&c1=<hex>&c2=<hex>&s0=<int>&s1=<int>&weak=<bool>&panel=<bool>&d=<int>`
- `i`: Index of the meter to configure
- `lbl`: Label to display inside the meter bar
- `fmt`: Format string for side value
- `pfmt`: Format string for panel value
- `max`: Maximum value for the meter bar
- `t`: Timeout in milliseconds
- `c0`, `c1`, `c2`: Colors to use for the meter bar
- `s0`, `s1`: Threshold values for switching colors
- `weak`: Set to `true` or `1` to hide the meter when no other is shown
- `panel`: Set to `true` or `1` to enable the associated value panel
- `d`: Number of decimal digits to display

For more information on how to customize the meters, see [METERBAR.md](METERBAR.md).

### `GET /setconfig`
Modify internal configuration entries.
- **Usage:** `/setconfig?name=<config_name>&value=<new_value>`
- `name`: Config entry name
- `value`: New value to set

For more information on config entries see [CONFIG.md](config.md).

### `GET /setsensor`
Sends sensor data to a configured meter.
- **Usage:** `/setsensor?i=<int>&v1=<int>&v2=<int>&vP=<int>`
- `i`: Index of the target meter
- `v1`: Primary value to write on the meter
- `v2`: Secondary argument for the format string
- `vP`: Value to write on the panel

For more information on how to customize the meters, see [METERBAR.md](METERBAR.md).

### `GET /snooze`
Silences or hides active notifications.
- **Usage:** `/snooze?name=<string>`
- `name`: Name of the specific notification to snooze

If no name is specified all notifications are snoozed.

- **Note:** persistent notifications (i.e. no-wifi alert) will only be cleared if snoozing by their name.

### `WebSocket /ws`
Posts sensor readings on a WebSocket. It is used by the main page to show the meter previews.

The received updates on the WS follow this format:
```json
{
    "index": meter_index (integer),
    "value": primary_value (integer),
    "value_display": side_label (string, already formatted with sensor values),
    "bar_color": meter_color (#RRGGBB string)
}
```