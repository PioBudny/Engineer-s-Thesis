#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include <string.h>
#include "NLG9881.h"
#include "hardware/clocks.h"

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define LED_PIN 25

// Zmienne do obsługi impulsów i diody
volatile bool pulse_active = false;
volatile uint32_t pulse_freq_hz = 0;
volatile uint32_t last_pulse_time = 0;
volatile uint32_t pulse_period_ms = 0;
volatile bool led_blinking = false;
volatile uint32_t last_blink_time = 0;
volatile uint32_t blink_period_ms = 0;
volatile bool led_state = false;


typedef enum {
    CMD_UNKNOWN,
    CMD_IMPULSE_SINGLE,
    CMD_IMPULSE_CONTINUOUS,
    CMD_IMPULSE_STOP,
    CMD_DEBUG_LED_ON,
    CMD_DEBUG_LED_OFF,
    CMD_READ_REGS
} CommandType;

static CommandType parse_command(const char *buffer)
{
    switch (buffer[0]) {
        case 'I':
            if (strncmp(buffer, "IMPULSE_SINGLE:", 15) == 0) {
                return CMD_IMPULSE_SINGLE;
            }
            if (strncmp(buffer, "IMPULSE_CONTINUOUS:", 19) == 0) {
                return CMD_IMPULSE_CONTINUOUS;
            }
            if (strncmp(buffer, "IMPULSE_STOP", 12) == 0) {
                return CMD_IMPULSE_STOP;
            }
            break;
        case 'D':
            if (strncmp(buffer, "DEBUG_LED_ON", 12) == 0) {
                return CMD_DEBUG_LED_ON;
            }
            if (strncmp(buffer, "DEBUG_LED_OFF", 13) == 0) {
                return CMD_DEBUG_LED_OFF;
            }
            break;
        case 'R':
            if (strncmp(buffer, "READ_REGS", 9) == 0) {
                return CMD_READ_REGS;
            }
            break;
        default:
            break;
    }
    return CMD_UNKNOWN;
}

void innit(){
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 10000000);

    i2c_device_init(I2C_PORT, I2C_SDA, I2C_SCL);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}
int main()
{

    innit();

    // Inicjalizacja urządzenia NLG9881

    char buffer[64];
    int buffer_index = 0;

    while (true) {
        // Non-blocking odczyt znaku
        int ch = getchar_timeout_us(0);
        
        if (ch != PICO_ERROR_TIMEOUT) {
            if (ch == '\n') {
                buffer[buffer_index] = 0;
                buffer_index = 0;
                
                // Przetworzenie komendy
                CommandType command = parse_command(buffer);

                switch (command) {
                    case CMD_IMPULSE_SINGLE:
                        printf("OK\n");
                        gpio_put(LED_PIN, 1);
                        sleep_ms(500);
                        gpio_put(LED_PIN, 0);
                        break;

                    case CMD_IMPULSE_CONTINUOUS: {
                        uint32_t freq_hz;
                        char channels_str[10];

                        if (sscanf(&buffer[19], "%u:%9s", &freq_hz, channels_str) == 2) {
                            if (freq_hz > 0) {
                                // Proporcjonalne mruganie
                                led_blinking = true;
                                blink_period_ms = (uint32_t)(500.0 * (8000.0 / freq_hz));  // Skalowanie: 500ms dla 8kHz, ~16ms dla 250kHz
                                if (blink_period_ms < 10) blink_period_ms = 10;  // Minimalny 10ms
                                last_blink_time = to_ms_since_boot(get_absolute_time());
                                led_state = false;
                                gpio_put(LED_PIN, 0);
                                printf("OK\n");
                            } else {
                                printf("INVALID\n");
                            }
                        } else {
                            printf("INVALID\n");
                        }
                        break;
                    }

                    case CMD_IMPULSE_STOP:
                        // Wyłącz mruganie diody
                        led_blinking = false;
                        gpio_put(LED_PIN, 0);
                        printf("OK\n");
                        gpio_put(LED_PIN, 1);
                        sleep_ms(500);
                        gpio_put(LED_PIN, 0);
                        break;

                    case CMD_DEBUG_LED_ON: {
                        pll_load_tcs(I2C_PORT);

                        printf("OK\n");
                        gpio_put(LED_PIN, 1);
                        break;
                    }

                    case CMD_DEBUG_LED_OFF:
                        Flag_Clear(I2C_PORT);
                        printf("OK\n");
                        gpio_put(LED_PIN, 0);
                        break;

                    case CMD_READ_REGS: { // Wypisywanie wszystkich rejestrów NLG
                        dump_all_regs(I2C_PORT);
                        printf("OK\n");
                        break;
                    }

                    case CMD_UNKNOWN:
                    default:
                        printf("UNKNOWN\n");
                        break;
                }
            } else if (buffer_index < sizeof(buffer) - 1) {
                buffer[buffer_index++] = ch;
            }
        }
        
    }
}