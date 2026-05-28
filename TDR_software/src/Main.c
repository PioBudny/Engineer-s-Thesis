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

#define CLOCK_CTRL_PIN 10

static bool i2c_address_exists(uint8_t addr)
{
    uint8_t dummy = 0;
    // Wysyłamy 1 bajt z timeout'em 1s i sprawdzamy czy rzeczywiście się wysłał (ACK od urządzenia)
    int result = i2c_write_timeout_us(I2C_PORT, addr, &dummy, 1, false, 1000000);
    return result == 1;
}

void scan_i2c()
{
    printf("Skanowanie magistrali I2C...\n");

    int devices_found = 0;

    for(uint8_t addr = 0x03; addr <= 0x88; addr++)
    {
        printf("Sprawdzanie adresu: 0x%02X... ", addr);
        if (i2c_address_exists(addr))
        {
            printf("ZNALEZIONO\n");
            devices_found++;
        } else {
            printf("brak\n");
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

void dump_all_regs(i2c_inst_t *i2c)
{
    uint8_t data;

    printf("\n==============================\n");
    printf("DUMP REJESTROW 8T49N241\n");
    printf("==============================\n");


    // ========================================
    // REJESTRY KONFIGURACYJNE
    // 0x0000 - 0x007B
    // ========================================

    printf("\nCONFIG REGS\n\n");

    for(uint16_t reg = 0x0000;
        reg <= 0x007B;
        reg++)
    {
        if(i2c_read_reg16(
            i2c,
            DEVICE_ADDR,
            reg,
            &data,
            1
        ))
        {
            printf(
                "REG[0x%04X] = 0x%02X\n",
                reg,
                data
            );
        }
        else
        {
            printf(
                "REG[0x%04X] = READ ERROR\n",
                reg
            );
        }

        sleep_us(100);
    }


    // ========================================
    // REJESTRY STATUSOWE
    // 0x0200 - 0x0212
    // ========================================

    printf("\nSTATUS REGS\n\n");

    for(uint16_t reg = 0x0200;
        reg <= 0x0212;
        reg++)
    {
        if(i2c_read_reg16(
            i2c,
            DEVICE_ADDR,
            reg,
            &data,
            1
        ))
        {
            printf(
                "REG[0x%04X] = 0x%02X\n",
                reg,
                data
            );
        }
        else
        {
            printf(
                "REG[0x%04X] = READ ERROR\n",
                reg
            );
        }

        sleep_us(100);
    }

    printf("\nDUMP COMPLETE\n");
}
bool device_force_freerun_mode(
    i2c_inst_t *i2c
)
{
    uint8_t val;

    printf("Ustawianie SYNTHESIZER/FREERUN...\n");


    // ========================================
    // REG 0x0069 = 0x1A
    // SYN_MODE = 1
    // ========================================

    val = 0x1A;

    if(!i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0069,
        &val,
        1
    ))
    {
        printf("REG 0x0069 write error\n");
        return false;
    }

    printf("SYN_MODE ustawiony\n");


    // ========================================
    // REG 0x000A = 0x31
    // STATE = freerun
    // ========================================

    val = 0x31;

    if(!i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x000A,
        &val,
        1
    ))
    {
        printf("REG 0x000A write error\n");
        return false;
    }

    printf("Freerun ustawiony\n");


    // ========================================
    // PLL_SYN = 1
    // ========================================

    val = 0x80;

    if(!i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0063,
        &val,
        1
    ))
    {
        printf("PLL_SYN set error\n");
        return false;
    }

    sleep_ms(10);


    // ========================================
    // PLL_SYN = 0
    // ========================================

    val = 0x00;

    if(!i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0063,
        &val,
        1
    ))
    {
        printf("PLL_SYN clear error\n");
        return false;
    }

    printf("PLL synchronized\n");

    return true;
}

bool force_q1_divider(i2c_inst_t *i2c)
{
    uint8_t val;

    printf("Setting Q1 divider...\n");


    // ========================================
    // N_Q1 = 8
    // REAL DIV = 16
    // ========================================

    val = 0x00;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0042,
        &val,
        1
    );

    val = 0x00;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0043,
        &val,
        1
    );

    val = 0x08;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0044,
        &val,
        1
    );

    printf("N_Q1 = 8\n");


    // ========================================
    // FRACTIONAL = 0
    // ========================================

    val = 0x00;

    for(uint16_t reg = 0x0057;
        reg <= 0x005A;
        reg++)
    {
        i2c_write_reg16(
            i2c,
            DEVICE_ADDR,
            reg,
            &val,
            1
        );
    }

    printf("Fractional divider cleared\n");


    // ========================================
    // DSM_INT = 70
    // VCO = 3500 MHz
    // 25 MHz crystal
    // doubler ON -> 50 MHz ref
    // 50 MHz * 70 = 3500 MHz
    // ========================================

    printf("Setting DSM_INT = 70...\n");

    val = 0x00;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0025,
        &val,
        1
    );

    val = 0x46;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0026,
        &val,
        1
    );

    printf("VCO target = 3500 MHz\n");


    // ========================================
    // CALRST TOGGLE
    // ========================================

    printf("Running VCO calibration...\n");

    val = 0x01;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0070,
        &val,
        1
    );

    sleep_ms(10);

    val = 0x00;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0070,
        &val,
        1
    );

    printf("VCO calibration done\n");


    // ========================================
    // PLL_SYN
    // ========================================

    val = 0x80;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0063,
        &val,
        1
    );

    sleep_ms(10);

    val = 0x00;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0063,
        &val,
        1
    );

    printf("PLL_SYN done\n");


    // ========================================
    // CLEAR LOL_INT
    // ========================================

    val = 0x40;

    i2c_write_reg16(
        i2c,
        DEVICE_ADDR,
        0x0200,
        &val,
        1
    );

    printf("LOL_INT cleared\n");


    // ========================================
    // WAIT FOR LOCK
    // ========================================

    sleep_ms(100);

    printf("PLL should now lock\n");

    return true;
}


void clock_ctrl_init() {

clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 10000000);
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
                    // Skonfiguruj wyjście 1 w tryb LVCMOS
                    device_force_freerun_mode(i2c0);

                    bool enabled = device_enable_q1_lvcmos(I2C_PORT);
                    if (!enabled) {
                        printf("Q1 LVCMOS enable failed\n");
                    } else {
                        printf("Q1 LVCMOS enabled\n");
                    }

                    uint8_t reg = 0;
                    if (i2c_read_reg16(I2C_PORT, DEVICE_ADDR, 0x001F, &reg, 1)) {
                        printf("PLL STATUS = 0x%02X\n", reg);
                    } else {
                        printf("PLL STATUS read failed\n");
                    }
force_q1_divider(i2c0);
                    printf("OK\n");
                    gpio_put(LED_PIN, 1);
                }
                else if (strncmp(buffer, "DEBUG_LED_OFF", 13) == 0) {
                    // Wyłącz sygnał zegarowy na wyjściu 1
                    // if (device_disable_output(I2C_PORT, 1)) {
                    //     printf("Clock signal disabled on channel 1\n");
                    //     // Wyłącz zegar na GPIO 10
                    //     clock_stop_fast();
                    //     // Wyłącz LED
                    //     //gpio_put(LED_PIN, 0);
                    // }
                    printf("OK\n");
                    gpio_put(LED_PIN, 0);
                }
                else if (strncmp(buffer, "READ_REGS", 9) == 0) {
                    uint16_t device_id2;
                    if (device_read_id(I2C_PORT, &device_id2)) {
                        printf("Device ID: 0x%04X\n", device_id2);
                    } else {
                        printf("Failed to read device ID\n");
                    }

                    // Always dump registers when READ_REGS is requested.
                    dump_all_regs(I2C_PORT);
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