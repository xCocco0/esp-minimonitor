/**
 * @file
 *
 * @brief HTTP request handlers
 *
 * Copyright 2026 Francesco Aiello
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mm_net_http_func.h"

#include "minimonitor.h"
#include "mm_panic.h"
#include "mm_sensor.h"
#include "mm_conf.h"
#include "mm_macros.h"
#include "protocol_examples_utils.h"
#include "mm_log.h"


#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (80)

/* note: http server is synchronous and the buffers exclusively used by the http task */
static char buf1[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN];
static char buf2[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN];
static char buf3[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN];

/* a version of http_key_query_key_value that truncate the query value to fit
into the destination buffer and discards the rest */
static esp_err_t query_key_value_trunc(const char *qry_str, const char *key, char *val, size_t val_size);

/* this is required because the libraries do not parse the + symbol in the query string */
static void __querystring_decode_supplement(char *buf, size_t buf_len);

static inline int __opt_from_query_str(const char* buf, const char* key, char *param_buf, size_t param_buf_size, char *out_buf)
{
    if(query_key_value_trunc(buf, key, param_buf, param_buf_size) == ESP_OK) {
        example_uri_decode(out_buf, param_buf, 1+strnlen(param_buf, param_buf_size));
        return 0;
    }
    return 1;
}

static inline int __opt_from_query_with_scanf(const char* buf, const char* key, char *param_buf, size_t param_buf_size, char *out_str, void *out_val, const char *fmt)
{
    if(httpd_query_key_value(buf, key, param_buf, param_buf_size) == ESP_OK) {
        example_uri_decode(out_str, param_buf, 1+strnlen(param_buf, param_buf_size));
        if(sscanf(out_str, fmt, out_val) == 1) {
            return 0;
        }
    }
    return 1;
}

static inline int __opt_from_query_bool(const char* buf, const char* key, char *param_buf, size_t param_buf_size, char *out_str, int *out_val)
{
    if(httpd_query_key_value(buf, key, param_buf, param_buf_size) == ESP_OK) {
        example_uri_decode(out_str, param_buf, 1+strnlen(param_buf, param_buf_size));
        if(strcmp(out_str, "true") == 0 || strcmp(out_str, "1") == 0) {
            *out_val = 1;
            return 0;
        }
        if(strcmp(out_str, "false") == 0 || strcmp(out_str, "0") == 0) {
            *out_val = 0;
            return 0;
        }
    }
    return 1;
}

esp_err_t mm_net_http_sensorconfig_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    struct mm_sensor_meter_config_s config = MM_SENSOR_METER_CONFIG_DEFAULT;

    int v = 0;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if(buf_len > 1) {
        buf = malloc(buf_len);
        if(!buf) {
            return ESP_ERR_NO_MEM;
        }
        if(httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};

            __querystring_decode_supplement(buf, buf_len);

            __opt_from_query_with_scanf(buf, "i", param, sizeof(param), dec_param, &config.index, "%d");
            if(__opt_from_query_str(buf, "lbl", param, sizeof(param), dec_param) == 0) {
                strncpy(config.label, dec_param, sizeof(config.label));
            }
            if(__opt_from_query_str(buf, "fmt", param, sizeof(param), dec_param) == 0) {
                strncpy(config.fmt, dec_param, sizeof(config.fmt));
            }
            if(__opt_from_query_str(buf, "pfmt", param, sizeof(param), dec_param) == 0) {
                strncpy(config.panel.fmt, dec_param, sizeof(config.panel.fmt));
            }
            __opt_from_query_with_scanf(buf, "max", param, sizeof(param), dec_param, &config.max_value, "%"PRIu32"");
            __opt_from_query_with_scanf(buf, "t", param, sizeof(param), dec_param, &config.timeout_ms, "%"PRIu32"");
            __opt_from_query_with_scanf(buf, "c0", param, sizeof(param), dec_param, &config.color.base, "%"PRIx32"");
            __opt_from_query_with_scanf(buf, "c1", param, sizeof(param), dec_param, &config.color.warn, "%"PRIx32"");
            __opt_from_query_with_scanf(buf, "c2", param, sizeof(param), dec_param, &config.color.alert, "%"PRIx32"");
            __opt_from_query_with_scanf(buf, "s0", param, sizeof(param), dec_param, &config.threshold.low, "%"PRIu32"");
            __opt_from_query_with_scanf(buf, "s1", param, sizeof(param), dec_param, &config.threshold.high, "%"PRIu32"");
            __opt_from_query_bool(buf, "weak", param, sizeof(param), dec_param, &v);
            if(v) {
                config.type = MM_METER_TYPE_WEAK;
                v = 0;
            }
            __opt_from_query_bool(buf, "panel", param, sizeof(param), dec_param, &v);
            if(v) {
                config.type = MM_METER_TYPE_PANEL;
            }
            __opt_from_query_with_scanf(buf, "a", param, sizeof(param), dec_param, &config.wakeup_threshold, "%"PRIu32"");
            __opt_from_query_with_scanf(buf, "ws", param, sizeof(param), dec_param, &config.wakeup_arg, "%d");
            __opt_from_query_with_scanf(buf, "cs", param, sizeof(param), dec_param, &config.color_arg, "%d");
            __opt_from_query_with_scanf(buf, "d", param, sizeof(param), dec_param, &config.decimals, "%d");

            mm_confsensor(&config);
        }
        free(buf);
    }

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t mm_net_http_setsensor_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    int index;
    uint32_t v1, v2 = 0, vP = 0;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if(buf_len > 1) {
        buf = malloc(buf_len);
        if(!buf) {
            return ESP_ERR_NO_MEM;
        }
        if(httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            //ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};

            __querystring_decode_supplement(buf, buf_len);

            if(__opt_from_query_with_scanf(buf, "i", param, sizeof(param), dec_param, &index, "%d") == 0) {
                if(__opt_from_query_with_scanf(buf, "v1", param, sizeof(param), dec_param, &v1, "%"PRIu32"") == 0) {
                    __opt_from_query_with_scanf(buf, "v2", param, sizeof(param), dec_param, &v2, "%"PRIu32"");
                    __opt_from_query_with_scanf(buf, "vP", param, sizeof(param), dec_param, &vP, "%"PRIu32"");
                    mm_setsensor(index, v1, v2, vP);
                }
            }
        }
        free(buf);
    }

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t mm_net_http_index_get_handler(httpd_req_t *req)
{
    char json_buf[512];
    size_t json_len;
    extern const unsigned char index_start[] asm("_binary_index_html_start");
    extern const unsigned char index_end[]   asm("_binary_index_html_end");
    const size_t index_size = (index_end - index_start);
    struct mm_sensor_meter_config_s config;
    const char *meter_config_placeholder = "/*PLACEHOLDER_METER_CONFIG*/";
    const char *system_config_placeholder = "/*PLACEHOLDER_SYSTEM_CONFIG*/";
    char *meter_config_pos, *system_config_pos;
    char *part1_start, *part2_start, *part3_start;
    size_t part1_len, part2_len, part3_len;
    size_t config_val_buf_size = 64;
    char *config_val_buf;

    #if 0 /* html page subdivision */
         index_start ->  <html> this is the first part
    meter_config_pos ->  /*PLACEHOLDER_METER_CONFIG*/
         part2_start ->  this is the second part
    system_config_pos->  /*PLACEHOLDER_SYSTEM_CONFIG*/
         part3_start ->  this is the last part </html>  <- index_end
    #endif

    meter_config_pos = memmem(index_start, index_size, meter_config_placeholder, strlen(meter_config_placeholder));
    system_config_pos = memmem(index_start, index_size, system_config_placeholder, strlen(system_config_placeholder));

    part1_start = (char *) index_start;
    part2_start = meter_config_pos + strlen(meter_config_placeholder);
    part3_start = system_config_pos + strlen(system_config_placeholder);

    part1_len = (size_t)(meter_config_pos - part1_start);
    part2_len = (size_t)(system_config_pos - part2_start);
    part3_len = (size_t)((char *) index_end - part3_start);

    if(meter_config_pos == NULL || system_config_pos == NULL) {
        return httpd_resp_send(req, (const char *) index_start, index_size);
    }

    /* send first part */
    httpd_resp_send_chunk(req, (const char *) part1_start, part1_len);

    /* read meter configs from nvs and embed values into html code */
    config.index = 0;
    while(mm_sensor_fetch_meter_config(&config) == MM_OK) {
        json_len = snprintf(json_buf, sizeof(json_buf), "{index:%d,label:\"%s\",max_value:%"PRIu32"},",
                config.index, config.label, config.max_value);
        httpd_resp_send_chunk(req, json_buf, MIN(json_len, sizeof(json_buf)));
        config.index++;
    }

    /* send second part */
    httpd_resp_send_chunk(req, part2_start, part2_len);

    /* read config and embed into html var */
    config_val_buf = malloc(config_val_buf_size);
    MM_CHECK_E(config_val_buf != NULL);

    for(int i = 0; i < MM_CONF_SIZE; ++i) {
        mm_conf_get_as_string(i, NULL, &config_val_buf_size);
        config_val_buf = realloc(config_val_buf, config_val_buf_size);
        mm_conf_get_as_string(i, config_val_buf, &config_val_buf_size);
        MM_CHECK_E(config_val_buf != NULL);
        json_len = snprintf(json_buf, sizeof(json_buf), "{config_name:\"%s\",current_value:\"%s\"},",
                mm_conf_get_name(i), config_val_buf);
        httpd_resp_send_chunk(req, json_buf, MIN(json_len, sizeof(json_buf)));
    }

    /* send the rest of the page */
    httpd_resp_send_chunk(req, part3_start, part3_len);

    return httpd_resp_send_chunk(req, NULL, 0);
}

esp_err_t mm_net_http_favicon_get_handler(httpd_req_t *req)
{
    extern const char favicon_html_start[] asm("_binary_favicon_ico_start");
    extern const char favicon_html_end[]   asm("_binary_favicon_ico_end");
    httpd_resp_send(req, favicon_html_start, favicon_html_end - favicon_html_start);

    return ESP_OK;
}

/* dummy function to pass to httpd_sess_set_ctx (because default one is `free`)*/
static void __empty(void *ctx) {(void) ctx;}

esp_err_t mm_net_http_ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        /* ws opened */
        MM_LOG_I("client connected to websocket /ws");
        httpd_sess_set_ctx(req->handle, httpd_req_to_sockfd(req), (void*) 1, __empty);
        return ESP_OK;
    }
    MM_LOG_I("receive on websocket /ws: discarded");
    return ESP_OK;
}

esp_err_t mm_net_http_notify_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char *const notif_name_buf = buf1;
    char *const notif_icon_buf = buf2;
    char *const notif_msg_buf = buf3;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if(!buf) {
            return ESP_ERR_NO_MEM;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            // ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};

            __querystring_decode_supplement(buf, buf_len);

            const char *name = NULL;
            const char *icon = NULL;
            uint32_t color = 0x0;
            uint32_t duration_ms = CONFIG_MM_NOTIF_DEFAULT_DURATION;
            const char *msg = NULL;
            mm_notify_stack_mode_t opt = MM_NOTIFY_DEFAULT;
            int v;

            if(__opt_from_query_str(buf, "name", param, sizeof(param), notif_name_buf) == 0) {
                name = notif_name_buf;
            };
            if(__opt_from_query_str(buf, "m", param, sizeof(param), notif_msg_buf) == 0) {
                msg = notif_msg_buf;
            };
            if(__opt_from_query_str(buf, "icon", param, sizeof(param), notif_icon_buf) == 0) {
                icon = notif_icon_buf;
            };
            v = 0;
            __opt_from_query_bool(buf, "replace", param, sizeof(param), dec_param, &v);
            if(v) opt = MM_NOTIFY_REPLACE;
            v = 0;
            __opt_from_query_bool(buf, "stack", param, sizeof(param), dec_param, &v);
            if(v) opt = MM_NOTIFY_STACK;
            v = 0;
            __opt_from_query_bool(buf, "append", param, sizeof(param), dec_param, &v);
            if(v) opt = MM_NOTIFY_APPEND;
            __opt_from_query_with_scanf(buf, "color", param, sizeof(param), dec_param, &color, "%"PRIx32"");
            __opt_from_query_with_scanf(buf, "duration", param, sizeof(param), dec_param, &duration_ms, "%"PRIu32"");

            mm_notify(name, icon, color, duration_ms, msg, opt, 0);

        }
        free(buf);
    }

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t mm_net_http_snooze_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char *notif_name = NULL;
    char *const notif_name_buf = buf1;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if(!buf) {
            return ESP_ERR_NO_MEM;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            //ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN];

            __querystring_decode_supplement(buf, buf_len);

            if(__opt_from_query_str(buf, "name", param, sizeof(param), notif_name_buf) == 0) {
                notif_name = notif_name_buf;
            }
        }
        free(buf);
    }

    mm_snooze(notif_name);

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static char *__http_send_until(httpd_req_t *req, const char* token, const char *iter, const char* end)
{
    char *new_iter = strnstr(iter, token, end - iter);
    httpd_resp_send_chunk(req, iter, new_iter - iter);
    new_iter += strlen(token);
    return new_iter;
}

esp_err_t mm_net_http_panic_get_handler(httpd_req_t *req)
{
    struct mm_panic_info_s info;
    char html_buf[180];
    char error_code[6];

    extern const char panic_start[] asm("_binary_panic_html_start");
    extern const char panic_end[]   asm("_binary_panic_html_end");

    const char *iter = panic_start;

    mm_panic_get_info(&info);
    mm_panic_info_to_code(&info, error_code);

    /* === send error general information === */

    iter = __http_send_until(req, "/*PLACEHOLDER_ERROR_INFO*/", iter, panic_end);

    if (mm_panic_info_check(&info)) {
        httpd_resp_sendstr_chunk(req, "<p style=\"color: #4caf50; font-weight: bold; margin-top: 0;\">No error detected</p>");
    } else {
        snprintf(html_buf, sizeof(html_buf), "<p style=\"color: #ff5252; font-weight: bold; margin-top: 0;\">Error %s</p>", error_code);
        httpd_resp_sendstr_chunk(req, html_buf);

        httpd_resp_sendstr_chunk(req, "<dl>");

        // reset_reason
        snprintf(html_buf, sizeof(html_buf), "<dt>Reset Reason</dt><dd>%d</dd>", info.reset_reason);
        httpd_resp_sendstr_chunk(req, html_buf);

        // elf_sha256
        snprintf(html_buf, sizeof(html_buf), "<dt>ELF SHA256</dt><dd>%s</dd>", info.elf_sha256);
        httpd_resp_sendstr_chunk(req, html_buf);

        // crashed_task
        snprintf(html_buf, sizeof(html_buf), "<dt>Crashed Task</dt><dd>%s</dd>", info.crashed_task);
        httpd_resp_sendstr_chunk(req, html_buf);

        // cause
        snprintf(html_buf, sizeof(html_buf), "<dt>Cause</dt><dd>%d</dd>", info.cause);
        httpd_resp_sendstr_chunk(req, html_buf);

        // panic_reason
        snprintf(html_buf, sizeof(html_buf), "<dt>Panic Reason</dt><dd>%s</dd>", info.panic_reason[0] != '0' ? info.panic_reason : "-");
        httpd_resp_sendstr_chunk(req, html_buf);

        // mm_err
        snprintf(html_buf, sizeof(html_buf), "<dt>MM Error</dt><dd>0x%"PRIu32"</dd>", info.mm_err);
        httpd_resp_sendstr_chunk(req, html_buf);

        httpd_resp_sendstr_chunk(req, "</dl>");
    }

    /* === send recent logs === */

    iter = __http_send_until(req, "/*PLACEHOLDER_LOGS*/", iter, panic_end);

    char *part1 = mm_log_history_part1();
    char *part2 = mm_log_history_part2();
    char *it = part1;
    char *buf_i = html_buf;
    char *buf_end = html_buf + sizeof(html_buf);
    int part = 1;
    int is_first = 1;

    //while(*iter != '\n' && *iter != '\0') iter++;

    while(1) {
        if(*it == '\0' && part == 1) {
            it = part2;
            part = 2;
        }

        if(*it == '\0' && part == 2) {
            sprintf(buf_i, "</p>");
            httpd_resp_sendstr_chunk(req, html_buf);
            break;
        }

        if(is_first) {
            buf_i = html_buf;
            if(*it == 'E') {
                buf_i += sprintf(buf_i, "<p class=error>");
            } else if(*it == 'W') {
                buf_i += sprintf(buf_i, "<p class=warning>");
            } else if(*it == 'I') {
                buf_i += sprintf(buf_i, "<p class=info>");
            } else {
                buf_i += sprintf(buf_i, "<p>");
            }
        }

        if(*it == '\n' || buf_i == buf_end) {
            snprintf(buf_i, buf_end - buf_i, "</p>");
            httpd_resp_sendstr_chunk(req, html_buf);
            buf_i = html_buf;
            it++;
            is_first = 1;
            continue;
        }

        *buf_i++ = *it++;
        is_first = 0;
    }

    /* === send error reference table === */

    iter = __http_send_until(req, "/*PLACEHOLDER_ERROR_TABLE*/", iter, panic_end);

    #define FOR_EACH_ERROR(code, desc) httpd_resp_send_chunk(req, "<tr><td><code>" #code "</code></td><td>" #desc "</td></tr>", sizeof("<tr><td><code>" #code "</code></td><td>" #desc "</td></tr>"));
    #include "mm_panic_errors.def"
    #undef FOR_EACH_ERROR

    // <tr><td><code>WD001</code></td><td>Reset (software or hardware) due to interrupt watchdog</td></tr>

    httpd_resp_send_chunk(req, iter, panic_end - iter);

    return ESP_OK;
}

esp_err_t mm_net_http_coredump_get_handler(httpd_req_t *req)
{
    uint8_t *coredump_data;
    size_t coredump_size;

    if(mm_panic_read_coredump(&coredump_data, &coredump_size) == MM_OK) {
        httpd_resp_set_type(req, "application/octet-stream");
        httpd_resp_send(req, (const char *) coredump_data, coredump_size);
    } else {
        httpd_resp_send_404(req);
    }

    return ESP_OK;
}

esp_err_t mm_net_http_setconfig_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char *const name_buf = buf1;
    char *const value_buf = buf2;
    mm_conf_entry_t entry;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if(buf_len > 1) {
        buf = malloc(buf_len);
        if(!buf) {
            return ESP_ERR_NO_MEM;
        }
        if(httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN];

            __querystring_decode_supplement(buf, buf_len);

            if(__opt_from_query_str(buf, "name", param, sizeof(param), name_buf) != 0) {
                MM_LOG_W("Missing required parameter 'name'");
                return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
            }
            if(__opt_from_query_str(buf, "value", param, sizeof(param), value_buf) != 0) {
                MM_LOG_W("Missing required parameter 'value'");
                return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
            }

            entry = mm_conf_entry_by_name(name_buf);
            if(entry == MM_CONF_NONE) {
                MM_LOG_W("Config name %s not found", name_buf);
                return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
            } else {
                MM_LOG_I("Updating config %s to %s", name_buf, value_buf);
                mm_conf_set_from_string(entry, value_buf);
            }
        }
        free(buf);
    }

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}


static esp_err_t query_key_value_trunc(const char *qry_str, const char *key, char *val, size_t val_size)
{
    if (qry_str == NULL || key == NULL || val == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *qry_ptr = qry_str;

    while (strlen(qry_ptr)) {
        /* Search for the '=' character. Else, it would mean
         * that the parameter is invalid */
        const char *val_ptr = strchr(qry_ptr, '=');
        if (!val_ptr) {
            break;
        }
        size_t offset = val_ptr - qry_ptr;

        /* If the key, does not match, continue searching.
         * Compare lengths first as key from url is not
         * null terminated (has '=' in the end) */
        if ((offset != strlen(key)) ||
            (strncasecmp(qry_ptr, key, offset))) {
            /* Get the name=val string. Multiple name=value pairs
             * are separated by '&' */
            qry_ptr = strchr(val_ptr, '&');
            if (!qry_ptr) {
                break;
            }
            qry_ptr++;
            continue;
        }

        /* Locate start of next query */
        qry_ptr = strchr(++val_ptr, '&');
        /* Or this could be the last query, in which
         * case get to the end of query string */
        if (!qry_ptr) {
            qry_ptr = val_ptr + strlen(val_ptr);
        }

        /* Query value length does not include terminating null */
        size_t val_len = qry_ptr - val_ptr;

        /* Copy value to the caller's buffer. */
        size_t copy_len = MIN(val_len, val_size - 1);
        /* If buffer length is smaller than needed, return truncation error */
        memcpy(val, val_ptr, copy_len);
        val[copy_len] = '\0';

        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

static void __querystring_decode_supplement(char *buf, size_t buf_len)
{
    for(int i = 0; i < buf_len && buf[i] != '\0'; ++i) {
        if(buf[i] == '+') {
            buf[i] = ' ';
        }
    }
}
