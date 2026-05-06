
#ifndef __MM_PORT_UI_CUSTOMIZATION_H
#define __MM_PORT_UI_CUSTOMIZATION_H

#include "sdkconfig.h"
#include "mm_macros.h"

/* Font size section.
 * Remember to use plain numbers (e.g. no parenthesis).
 */

#define MM_PORT_FONT_SIZE_TINY         14  /** Font size for notification icon badge. */
#define MM_PORT_FONT_SIZE_VERY_SMALL   16  /** Font size for labels inside bars. */
#define MM_PORT_FONT_SIZE_SMALL        22  /** Font size for labels beside bars and toast notifications. */
#define MM_PORT_FONT_SIZE_NORMAL       34  /** Font size for clock and fps monitor. */
#define MM_PORT_FONT_SIZE_LARGE        48  /** Font size to use for larger clock. */
#define MM_PORT_FONT_SIZE_VERY_LARGE   72  /** Font size to use when only clock is shown. */

/* Screen boarders section.
 * Increase if your screen is very large or has a weird ratio.
 */

#define MM_PORT_BORDER_TOP    16
#define MM_PORT_BORDER_BOTTOM 14
#define MM_PORT_BORDER_LEFT   15
#define MM_PORT_BORDER_RIGHT  15
#define MM_PORT_BORDER_INNER  10

/* Widget dimension section.
 * If you set the font correctly this should not be needed. But if you want
 * a polished results, you can fine tune these values too.
 */

//#define MM_PORT_PANEL_NORMAL_WIDTH       ...
//#define MM_PORT_PANEL_NORMAL_HEIGHT      ...

//#define MM_PORT_PANEL_LARGE_WIDTH        ...
//#define MM_PORT_PANEL_LARGE_HEIGHT       ...

//#define MM_PORT_PANEL_VERY_LARGE_WIDTH   ...
//#define MM_PORT_PANEL_VERY_LARGE_HEIGHT  ...

//#define MM_PORT_PANEL_NOTIF_TOP_HEIGHT   ...
//#define MM_PORT_NOTIF_ICON_SCALE         ...
//#define MM_PORT_BUTTON_SIZE              ...
//#define MM_PORT_MAX_METERS               ...

/* Fill non defined values with defaults */
#include "mm_port_ui_customization_defaults.h"

#endif /* __MM_PORT_UI_CUSTOMIZATION_H */
