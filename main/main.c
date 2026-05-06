
#include "esp_app_desc.h"
#include "minimonitor.h"
#include "mm_port.h"
#include "mm_conf.h"
#include "mm_cli.h"
#include "mm_ui.h"
#include "mm_net.h"
#include "mm_io.h"
#include "mm_log.h"

void app_main(void)
{
    MM_LOG_I("========================================");
    MM_LOG_I("MiniMonitor v%s", esp_app_get_description()->version);
    MM_LOG_I("Compiled for %s", CONFIG_MM_PORT_TARGET_BOARD);
    MM_LOG_I("========================================");

    mm_log_init();
    mm_conf_init();
    mm_port_init();
    mm_port_display_set_bk(0);
    mm_init();
    mm_cli_init();
    mm_ui_init();
    mm_net_init();
    mm_io_init();

    mm_start();
}
