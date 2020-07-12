#include <stdio.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#define TAG "ESP32_KBM_CONSOLE"

const char *prompt = LOG_COLOR_I "> " LOG_RESET_COLOR;

/** Arguments used by 'passkey' function */
static struct {
    struct arg_int *passkey;
    struct arg_end *end;
} passkey_args;

extern void bluetooth_send_passkey(uint32_t passkey);
int reply_with_passkey(int argc, char **argv);

void initialise_console()
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
    };

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
                                        256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);
}

void config_prompts()
{
    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status)
    { /* zero indicates success */
        ESP_LOGI(TAG, "\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "> ";
#endif //CONFIG_LOG_COLORS
    }
}

void watch_prompts()
{
    while (1)
    {
        char *line = linenoise(prompt);
        if (line == NULL)
        { /* Got EOF or error */
            continue;
        }
        /* Add the command to the history if not empty*/
        if (strlen(line) > 0)
        {
            linenoiseHistoryAdd(line);
        }
        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            // command was empty
        }
        else if (err == ESP_OK && ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        }
        else if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
}

int reply_with_passkey(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &passkey_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, passkey_args.end, argv[0]);
        return 1;
    }

    ESP_LOGI(TAG, "Replying with passkey %zu", passkey_args.passkey->ival[0]);
    bluetooth_send_passkey(passkey_args.passkey->ival[0]);

    return 0;
}

void console_register_bluetooth_commands()
{
    /**
     * Reply to client with passkey
     */
    passkey_args.passkey = arg_int0(NULL, NULL, "<pass>", "passkey");
    passkey_args.end = arg_end(0);

    const esp_console_cmd_t passkey_cmd = {
        .command = "passkey",
        .help = "Reply to initiator with a 6-digit passkey",
        .hint = "passkey 999999",
        .func = &reply_with_passkey,
        .argtable = &passkey_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&passkey_cmd));

    /**
     * Translate to HID values 
     */
}