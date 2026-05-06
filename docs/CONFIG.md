# Config entries reference

**HOSTNAME**
  * **Type:** `str`
  * **Default:** `"minimonitor"`
  * **Reboot Required:** Yes
  * **Description:** The hostname of the device, to use for both MDNS and as TTY prefix.

* **TZ**
  * **Type:** `str`
  * **Default:** `""`
  * **Reboot Required:** No
  * **Description:** Timezone string used to calculate local time offsets.

* **NTP_SERVER**
  * **Type:** `str`
  * **Default:** `"pool.ntp.org"`
  * **Reboot Required:** Yes
  * **Description:** The address of the NTP server used for clock synchronization.

* **CLOCK_H_MIN_SEPARATOR**
  * **Type:** `str`
  * **Default:** `":"`
  * **Reboot Required:** Yes
  * **Description:** The character string used to separate hours and minutes on the clock display.

* **CLOCK_SEPARATOR_ANIMATED**
  * **Type:** `u8`
  * **Default:** `0`
  * **Reboot Required:** Yes
  * **Description:** When enabled, the separator character will blink every second.

* **CLOCK_USE_SECONDS**
  * **Type:** `u8`
  * **Default:** `0`
  * **Reboot Required:** Yes
  * **Description:** Enables or disables the display of seconds on the main clock.

* **CLOCK_FORMAT_HOUR**
  * **Type:** `u8`
  * **Default:** `12`
  * **Reboot Required:** Yes
  * **Description:** Sets the time format (12 for 12-hour and 24 for 24-hour cycle).

* **COLOR_CARD**
  * **Type:** `u32`
  * **Default:** `0x282B30`
  * **Reboot Required:** Yes
  * **Description:** The color to use for a panel background.

* **COLOR_DETAIL**
  * **Type:** `u32`
  * **Default:** `0x393c41`
  * **Reboot Required:** Yes
  * **Description:** Unused

* **COLOR_PRIMARY**
  * **Type:** `u32`
  * **Default:** `0x6B7FD7`
  * **Reboot Required:** Yes
  * **Description:** The primary accent color for LVGL.

* **COLOR_SECONDARY**
  * **Type:** `u32`
  * **Default:** `0x6B7FD7`
  * **Reboot Required:** Yes
  * **Description:** The secondary accent color for LVGL.

* **COLOR_TEXT**
  * **Type:** `u32`
  * **Default:** `0xFAFAFA`
  * **Reboot Required:** Yes
  * **Description:** The RGB color code for text.

* **COLOR_BACKGROUND**
  * **Type:** `u32`
  * **Default:** `0x15171A`
  * **Reboot Required:** Yes
  * **Description:** The RGB color code for the screen background.

* **BACKLIGHT**
  * **Type:** `u8`
  * **Default:** `80`
  * **Reboot Required:** No
  * **Description:** Sets the display brightness level (0 min, 255 max).

* **LOGLEVEL**
  * **Type:** `u8`
  * **Default:** `3`
  * **Reboot Required:** No
  * **Description:** Controls the verbosity of system logs (0 = No logs, 1 = Errors, 2 = Warnings, 3 = Information, 4 = Debug, 5 = Verbose, 6 = All).

* **DONOTDISTURB**
  * **Type:** `u8`
  * **Default:** `0`
  * **Reboot Required:** No
  * **Description:** When enabled, suppresses notifications on the main screen (but are posted on the notification board).

* **AUTOROTATION**
  * **Type:** `u8`
  * **Default:** `0`
  * **Reboot Required:** No
  * **Description:** Enables automatic screen rotation based on sensor input.

* **PREFERRED_ROTATION**
  * **Type:** `u8`
  * **Default:** `0`
  * **Reboot Required:** No
  * **Description:** Stores the rotation setting to use at startup.

* **BACKGROUND_IMAGE**
  * **Type:** `str`
  * **Default:** `""`
  * **Reboot Required:** Yes
  * **Description:** File path on the SD card for a custom background image.