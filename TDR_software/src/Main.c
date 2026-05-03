#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include <string.h>
#include "NLG9881.h"

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

#define CLOCK_CTRL_PIN 10

void clock_ctrl_init() {
    gpio_init(CLOCK_CTRL_PIN);
    gpio_set_dir(CLOCK_CTRL_PIN, GPIO_OUT);

    gpio_put(CLOCK_CTRL_PIN, 0); // start: OFF
}
static inline void clock_stop_fast() {
    sio_hw->gpio_clr = (1u << CLOCK_CTRL_PIN);
}

static inline void clock_start_fast() {
    sio_hw->gpio_set = (1u << CLOCK_CTRL_PIN);
}
int main()
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0); // Start with LED off
    i2c_device_init(I2C_PORT, I2C_SDA, I2C_SCL);

    // Inicjalizacja urządzenia NLG9881
    uint16_t device_id;
    if (device_read_id(I2C_PORT, &device_id)) {
        printf("Device ID: 0x%04X\n", device_id);
    } else {
        printf("Failed to read device ID\n");
    }

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
                if (strncmp(buffer, "IMPULSE_SINGLE:", 15) == 0) {
                    // Parsowanie kanałów: "1", "2", "12"
                    const char *channels_str = &buffer[15];
                    
                    for (int i = 0; channels_str[i] != '\0'; i++) {
                        if (channels_str[i] == '1') {
                            if (device_pulse_output(I2C_PORT, 0)) {
                                printf("Single pulse on channel 0\n");
                            }
                        } else if (channels_str[i] == '2') {
                            if (device_pulse_output(I2C_PORT, 1)) {
                                printf("Single pulse on channel 1\n");
                            }
                        }
                    }
                    printf("OK\n");
                    gpio_put(LED_PIN, 1);
                    sleep_ms(500);
                    gpio_put(LED_PIN, 0);
                }
                else if (strncmp(buffer, "IMPULSE_CONTINUOUS:", 19) == 0) {
                    // Parsowanie: "IMPULSE_CONTINUOUS:freq:channels"
                    uint32_t freq_hz;
                    char channels_str[10];
                    
                    if (sscanf(&buffer[19], "%u:%9s", &freq_hz, channels_str) == 2) {
                        if (freq_hz > 0) {
                            // Ustawianie częstotliwości dla każdego kanału
                            for (int i = 0; channels_str[i] != '\0'; i++) {
                                if (channels_str[i] == '1') {
                                    if (device_set_frequency(I2C_PORT, 0, freq_hz)) {
                                        device_step_output(I2C_PORT, 0); // Włącz wyjście
                                        printf("Set frequency %u Hz on channel 0\n", freq_hz);
                                    }
                                } else if (channels_str[i] == '2') {
                                    if (device_set_frequency(I2C_PORT, 1, freq_hz)) {
                                        device_step_output(I2C_PORT, 1); // Włącz wyjście
                                        printf("Set frequency %u Hz on channel 1\n", freq_hz);
                                    }
                                }
                            }
                            
                            // Rozpocznij mruganie diody proporcjonalnie do częstotliwości (wolniej dla niższych freq)
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
                }
                else if (strncmp(buffer, "IMPULSE_STOP", 12) == 0) {
                    // Wyłącz wyjścia dla kanałów 0 i 1
                    device_disable_output(I2C_PORT, 0);
                    device_disable_output(I2C_PORT, 1);
                    // Wyłącz mruganie diody
                    led_blinking = false;
                    gpio_put(LED_PIN, 0);
                    printf("OK\n");
                    gpio_put(LED_PIN, 1);
                    sleep_ms(500);
                    gpio_put(LED_PIN, 0);
                }
                else {
                    printf("UNKNOWN\n");
                }
            } else if (buffer_index < sizeof(buffer) - 1) {
                buffer[buffer_index++] = ch;
            }
        }
        
        // Mruganie diody dla continuous impulse
        if (led_blinking && blink_period_ms > 0) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            
            if (now - last_blink_time >= blink_period_ms) {
                led_state = !led_state;
                gpio_put(LED_PIN, led_state);
                last_blink_time = now;
            }
        }
    }
}