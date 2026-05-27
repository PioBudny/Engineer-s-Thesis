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

static bool i2c_address_exists(uint8_t addr)
{
    uint8_t dummy = 0;
    int result;

    // 1) Najpierw próbujemy samo potwierdzenie adresu: write bez danych.
    result = i2c_write_blocking(I2C_PORT, addr, &dummy, 0, false);
    if (result >= 0) {
        return true;
    }

    // 2) Spróbuj odczytu 1 bajtu (niektóre urządzenia odpowiadają tylko na read).
    result = i2c_read_blocking(I2C_PORT, addr, &dummy, 1, false);
    if (result >= 0) {
        return true;
    }

    // 3) Spróbuj zapisu 1 bajtu, co może zadziałać dla urządzeń wymagających minimalnej sekwencji.
    result = i2c_write_blocking(I2C_PORT, addr, &dummy, 1, false);
    return result >= 0;
}

void scan_i2c()
{
    printf("Skanowanie magistrali I2C...\n");

    int devices_found = 0;

    for(uint8_t addr = 0x03; addr <= 0x77; addr++)
    {
        if (i2c_address_exists(addr))
        {
            printf("Znaleziono urządzenie: 0x%02X\n", addr);
            devices_found++;
        }
    }

    if(devices_found == 0)
    {
        printf("Nie znaleziono urządzeń I2C\n");
    }
    else
    {
        printf("Koniec skanowania\n");
    }
}

void clock_ctrl_init() {
    gpio_init(CLOCK_CTRL_PIN);
    gpio_set_dir(CLOCK_CTRL_PIN, GPIO_OUT);
    //gpio_set_function(CLOCK_CTRL_PIN, GPIO_FUNC_GPCK);  //GPIO 10 jako clock output

    gpio_put(CLOCK_CTRL_PIN, 0);
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
    gpio_put(LED_PIN, 0);
    clock_ctrl_init();
    i2c_device_init(I2C_PORT, I2C_SDA, I2C_SCL);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicjalizacja urządzenia NLG9881
    uint16_t device_id;
    if (device_read_id(I2C_PORT, &device_id)) {
        printf("Device ID: 0x%04X\n", device_id);
    } else {
        printf("Failed to read device ID\n");
    }
    printf("Ready for commands...\n");

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
                else if (strncmp(buffer, "DEBUG_LED_ON", 12) == 0) {
                    // Skonfiguruj wyjście 1 w tryb CMOS
                    if (device_set_output_cmos(I2C_PORT, 1)) {
                        // Włącz sygnał zegarowy na wyjściu 1 (10 MHz)
                        if (device_set_frequency(I2C_PORT, 1, 10000000)) {
                            printf("Ustawiona częstotliwość 10 MHz na kanale 1\n");
                            if (device_step_output(I2C_PORT, 1)) {
                                printf("Clock signal 10 MHz enabled on channel 1 (CMOS)\n");
                                // Włącz zegar na GPIO 10
                                clock_start_fast();
                                // Włącz LED
                                gpio_put(LED_PIN, 1);
                            }
                        }
                    }
                    printf("OK2\n");
                    //gpio_put(LED_PIN, 1);
                    uint8_t dummy = 0;
                    scan_i2c();


                }
                else if (strncmp(buffer, "DEBUG_LED_OFF", 13) == 0) {
                    // Wyłącz sygnał zegarowy na wyjściu 1
                    if (device_disable_output(I2C_PORT, 1)) {
                        printf("Clock signal disabled on channel 1\n");
                        // Wyłącz zegar na GPIO 10
                        clock_stop_fast();
                        // Wyłącz LED
                        gpio_put(LED_PIN, 0);
                    }
                    printf("OK\n");
                    //gpio_put(LED_PIN, 0);
                }
                else if (strncmp(buffer, "READ_ID", 7) == 0) {
                    uint16_t device_id2;
                    if (device_read_id(I2C_PORT, &device_id2)) {
                        printf("Device ID: 0x%04X\n", device_id2);
                    } else {
                        printf("Failed to read device ID\n");
                    }
                    printf("OK\n");
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