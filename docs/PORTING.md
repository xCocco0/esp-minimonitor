# Porting to other platforms

Every port must include:
- A codename, say `my_board` to be used throughout the project
- An `sdkconfig.my_board` file ovverriding the default `sdkconfig.defaults`
- A subdirectory `my_board/` in the `mm_port` component with board-specific code
- Its dependencies in the `idf_component.yml` of `mm_port`

## sdkconfig file

This file must define the CONFIG_MM_PORT_TARGET_BOARD variable set to the board name (e.g. `my_board`). This is the only
requirement, as unspecified configuration options are read from the main `sdkconfig.defaults` file.

## Dependencies and compilation

To add a dependency in the `idf_component.yml` for this board, remember to add this condition to the component:
```yml
    matches:
      - if: "$MM_TARGET == my_board"
```

To add dependencies/sources to `idf_component_register`, define the following variables inside `mm_port/my_board/include.cmake`:
- `board_src`, to specify additional sources to pass to `SRCS` option
- `board_includes`, to specify include directories for `INCLUDE_DIRS` option
- `board_priv_includes`, for `PRIV_INCLUDE_DIRS`
- `board_requires`, for component requirements in `PRIV_REQUIRES`

## C definitions

You must define:
- The display startup and utility functions (see [mm_port_display.h](components\mm_port\include\mm_port_display.h))
- If `CONFIG_MM_PORT_HAS_IMU=y`, the functions for initializing and reading the gyro and accelerator (see [mm_port_imu.h](components\mm_port\include\mm_port_imu.h))
- If `CONFIG_MM_PORT_HAS_SDCARD=y`, the filesystem initialization functions (see [mm_port_fs.h](components\mm_port\include\mm_port_fs.h))

If needed you can define `mm_port_init` that replaces the default `mm_port` initialization function.

All functions are defined as weak symbols, with a default implementation in [mm_port_defaults.c](components\mm_port\mm_port_defaults.c)
(most just raise an error).

## UI Layout

You must include a `mm_port_ui_customization.h` with screen layout adjustments. It is mandatory to specify the font
size to use with the `MM_PORT_FONT_SIZE_xyz` variables, while other options are optional and automatically generated
starting from the screen size and the font size.

---

### A note on display rotation

The rotation specified by the `rotate` command is relative to the final rotation after `CONFIG_MM_PORT_LCD_ROTATION_x`
is applied. So `rotate 0` will rotate the display by `CONFIG_MM_PORT_LCD_ROTATION_x` degrees with respect to the
default `CONFIG_MM_PORT_LCD_H_RES`x`CONFIG_MM_PORT_LCD_V_RES` layout.

This way you can set `CONFIG_MM_PORT_LCD_H/V_RES` to match the physical display dimensions and initialize the logic
accordingly. You can then take into account for `CONFIG_MM_PORT_LCD_ROTATION_x` when you define the rotation
function.
