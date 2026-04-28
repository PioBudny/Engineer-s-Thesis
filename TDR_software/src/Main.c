#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include <string.h>

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define LED_PIN 25

// Zmienne do obsługi mrugania
volatile bool led_blinking = false;
volatile float blink_freq = 0;
volatile uint32_t last_blink_time = 0;
volatile bool led_state = false;
volatile uint32_t half_period_ms = 0;

int main()
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

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
                if (strncmp(buffer, "IMPULSE_SINGLE", 14) == 0) {
                    led_blinking = false;
                    gpio_put(LED_PIN, 1);
                    sleep_ms(2000);
                    gpio_put(LED_PIN, 0);
                    printf("OK\n");
                }
                else if (strncmp(buffer, "IMPULSE_CONTINUOUS:", 19) == 0) {
                    float freq = atof(&buffer[19]);
                    if (freq > 0) {
                        led_blinking = true;
                        blink_freq = freq;
                        half_period_ms = (uint32_t)(100.0 / freq);
                        led_state = false;
                        last_blink_time = to_ms_since_boot(get_absolute_time());
                        gpio_put(LED_PIN, 0);
                        printf("OK\n");
                    } else {
                        printf("INVALID\n");
                    }
                }
                else if (strncmp(buffer, "IMPULSE_STOP", 12) == 0) {
                    led_blinking = false;
                    gpio_put(LED_PIN, 0);
                    printf("OK\n");
                }
                else {
                    printf("UNKNOWN\n");
                }
            } else if (buffer_index < sizeof(buffer) - 1) {
                buffer[buffer_index++] = ch;
            }
        }
        
        // Mruganie LED w głównej pętli (non-blocking)
        if (led_blinking) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            
            if (now - last_blink_time >= half_period_ms) {
                led_state = !led_state;
                gpio_put(LED_PIN, led_state);
                last_blink_time = now;
            }
        }
    }
}