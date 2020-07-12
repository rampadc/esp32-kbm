/******************************************************************************
 * Dependencies
 *****************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdio.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "hid_dev.h"
#include "ble_kbm_types.h"
#include "commands.h"

/******************************************************************************
 * File variables
 *****************************************************************************/
#define TAG "ESP32_KBM_CONSOLE"

const char *prompt = LOG_COLOR_I "> " LOG_RESET_COLOR;

/** Arguments used by 'passkey' function */
static struct
{
    struct arg_int *passkey;
    struct arg_end *end;
} passkey_args;

/** Arguments used by 'r <raw>' function */
static struct
{
    struct arg_int *modifier;
    struct arg_int *keycode;
    struct arg_end *end;
} raw_keycode_args;

static struct 
{
    struct arg_int *mouse_buttons;
    struct arg_int *movement_x;
    struct arg_int *movement_y;
    struct arg_end *end;
} mouse_args;

/******************************************************************************
 * External variables
 *****************************************************************************/
extern QueueHandle_t passkey_queue;
extern QueueHandle_t keyboard_queue;
extern QueueHandle_t mouse_queue;
extern QueueHandle_t commands_queue;

/******************************************************************************
 * External functions
 *****************************************************************************/
extern void bluetooth_send_passkey(uint32_t passkey);
extern void bluetooth_send_character(char);

/******************************************************************************
 * Function declarations
 *****************************************************************************/
int reply_with_passkey(int argc, char **argv);
int send_modifier_keycode(int argc, char **argv);
void config_prompts();
void watch_prompts();
void console_register_bluetooth_commands();

/******************************************************************************
 * Function implementation
 *****************************************************************************/
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
            // ESP_LOGE(TAG, "Got EOF or error");
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

/******************************************************************************
 * Console command handlers
 *****************************************************************************/
int reply_with_passkey(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&passkey_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, passkey_args.end, argv[0]);
        return 1;
    }

    ESP_LOGI(TAG, "Replying with passkey %zu", passkey_args.passkey->ival[0]);

    /* send passkey to bluetooth task */
    uint32_t value = passkey_args.passkey->ival[0];

    if (passkey_queue != 0)
    {
        if (xQueueSend(passkey_queue, (void *)&value, (TickType_t)10) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to send passkey to queue");
            return 1;
        }

        ESP_LOGI(TAG, "Passkey sent to queue");
    }
    return 0;
}

int send_modifier_keycode(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&raw_keycode_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, raw_keycode_args.end, argv[0]);
        return 1;
    }

    uint8_t modifier = raw_keycode_args.modifier->ival[0];
    uint8_t keycode = raw_keycode_args.keycode->ival[0];

    ESP_LOGD(TAG, "modifier: %d", modifier);
    ESP_LOGD(TAG, "keycode: %d", keycode);

    keyboard_t keyboard_value = {
        .modifier = modifier,
        .keycode = keycode
    };

    if (keyboard_queue != 0)
    {
        if (xQueueSend(keyboard_queue, (void *)&keyboard_value, (TickType_t)10) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to send keyboard value to queue");
            return 1;
        }

        ESP_LOGI(TAG, "Keyboard value sent to queue");
    }
    return 0;
}

int send_mouse(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&mouse_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, mouse_args.end, argv[0]);
        return 1;
    }

    uint8_t buttons = mouse_args.mouse_buttons->ival[0];
    int8_t x = mouse_args.movement_x->ival[0];
    int8_t y = mouse_args.movement_y->ival[0];

    mouse_t mouse_data = {
        .mouse_buttons = buttons,
        .movement_x = x,
        .movement_y = y
    };

    if (mouse_queue != 0)
    {
        if (xQueueSend(mouse_queue, (void *)&mouse_data, (TickType_t)10) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to send mouse data to queue");
            return 1;
        }

        ESP_LOGI(TAG, "Mouse data sent to queue");
    }
    return 0;
}

int delete_bondings(int argc, char **argv)
{
    uint8_t command = DELETE_ALL_BONDINGS;
    if (commands_queue != 0)
    {
        if (xQueueSend(commands_queue, (void *)&command, (TickType_t)10) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to send DELETE_BONDING command to queue");
            return 1;
        }

        ESP_LOGI(TAG, "DELETE_BONDING command sent to queue");
    }
    return 0;
}

int list_bondings(int argc, char **argv)
{
    uint8_t command = LIST_BONDINGS;
    if (commands_queue != 0)
    {
        if (xQueueSend(commands_queue, (void *)&command, (TickType_t)10) != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to send LIST_BONDINGS command to queue");
            return 1;
        }

        ESP_LOGI(TAG, "LIST_BONDINGS command sent to queue");
    }
    return 0;
}

/******************************************************************************
 * Register console commands
 *****************************************************************************/
void console_register_bluetooth_commands()
{
    /**
     * Reply to client with passkey
     */
    passkey_args.passkey = arg_int0(NULL, NULL, "<pass>", "passkey");
    passkey_args.end = arg_end(0);

    const esp_console_cmd_t passkey_cmd = {
        .command = "p",
        .help = "Send a 6-digit passkey",
        .hint = "p 999999",
        .func = &reply_with_passkey,
        .argtable = &passkey_args};

    ESP_ERROR_CHECK(esp_console_cmd_register(&passkey_cmd));

    /**
     * Send raw keycode value
     */
    raw_keycode_args.modifier = arg_int0(NULL, NULL, "<modifier>", "modifier");
    raw_keycode_args.keycode = arg_int1(NULL, NULL, "<keycode>", "keycode");
    raw_keycode_args.end = arg_end(1);

    const esp_console_cmd_t raw_keycode_cmd = {
        .command = "r",
        .help = "Send raw keycode with modifier and keycode",
        .hint = "r modifier keycode",
        .func = &send_modifier_keycode,
        .argtable = &raw_keycode_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&raw_keycode_cmd));

    /**
     * Send mouse values
     */
    mouse_args.mouse_buttons = arg_int0(NULL, NULL, "<btn>", "buttons");
    mouse_args.movement_x = arg_int1(NULL, NULL, "<x>", "x");
    mouse_args.movement_y = arg_int1(NULL, NULL, "<y>", "y");
    mouse_args.end = arg_end(2);

    const esp_console_cmd_t mouse_cmd = {
        .command = "m",
        .help = "Send mouse command",
        .hint = "m buttons x y",
        .func = &send_mouse,
        .argtable = &mouse_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&mouse_cmd));

    /**
     * Delete all bondings
     */
    const esp_console_cmd_t delete_bondings_cmd = {
        .command = "db",
        .help = "Delete all bondings",
        .hint = "db",
        .func = &delete_bondings,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&delete_bondings_cmd));

    /**
     * List bondings
     */
    const esp_console_cmd_t list_bondings_cmd = {
        .command = "lb",
        .help = "List bondings",
        .hint = "lb",
        .func = &list_bondings,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&list_bondings_cmd));

}