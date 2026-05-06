
#ifndef __MM_PORT_UI_CUSTOMIZATION_H
#define __MM_PORT_UI_CUSTOMIZATION_H

#include "sdkconfig.h"
#include "mm_macros.h"

/* Font size section.
 * Remember to use plain numbers (i.e. no parenthesis).
 * For LVGL fonts you must also toggle the respective LV_FONT option in the Kconfig.
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

#define MM_PORT_BORDER_TOP    14
#define MM_PORT_BORDER_BOTTOM 12
#define MM_PORT_BORDER_LEFT   13
#define MM_PORT_BORDER_RIGHT  13
#define MM_PORT_BORDER_INNER  10

/* Widget dimension section.
 * If you set the font correctly this should not be needed. But if you want
 * a polished result, you can fine tune these values too.
 */

#define MM_PORT_PANEL_NORMAL_WIDTH       (MM_PORT_FONT_SIZE_NORMAL*4)
#define MM_PORT_PANEL_NORMAL_HEIGHT      (MM_PORT_FONT_SIZE_NORMAL*150/100)

#define MM_PORT_PANEL_LARGE_WIDTH        ((MM_PORT_FONT_SIZE_LARGE*80/100)*4)
#define MM_PORT_PANEL_LARGE_HEIGHT       (MM_PORT_FONT_SIZE_LARGE*130/100)

#define MM_PORT_PANEL_VERY_LARGE_WIDTH   ((MM_PORT_FONT_SIZE_VERY_LARGE*75/100)*4)
#define MM_PORT_PANEL_VERY_LARGE_HEIGHT  (MM_PORT_FONT_SIZE_VERY_LARGE*125/100)

#define MM_PORT_PANEL_NOTIF_TOP_HEIGHT   (MM_PORT_FONT_SIZE_NORMAL*130/100)
#define MM_PORT_NOTIF_ICON_SCALE         (256*MM_PORT_FONT_SIZE_NORMAL/34)

#define MM_PORT_MAX_METERS               (3)

/* Fill non defined values with defaults */
#include "mm_port_ui_customization_defaults.h"

#endif /* __MM_PORT_UI_CUSTOMIZATION_H */
