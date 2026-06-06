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
static uint8_t reg;


typedef enum {
    CMD_UNKNOWN,
    CMD_IMPULSE_START,
    CMD_IMPULSE_STOP,
    CMD_LOAD_CONFIG,
    CMD_READ_REGS,
    CMD_DEFAULT_CONFIG
} CommandType;

static CommandType parse_command(const char *buffer)
{
    if (strncmp(buffer, "READ_REGS", 9) == 0)
        return CMD_READ_REGS;

    if (strncmp(buffer, "DEFAULT_CONFIG", 14) == 0)
        return CMD_DEFAULT_CONFIG;

    if (strncmp(buffer, "LOAD_CONFIG", 11) == 0)
        return CMD_LOAD_CONFIG;

    if (strncmp(buffer, "IMPULSE_START", 13) == 0)
        return CMD_IMPULSE_START;

    if (strncmp(buffer, "IMPULSE_STOP", 12) == 0)
        return CMD_IMPULSE_STOP;

    return CMD_UNKNOWN;
}

void innit(){
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 1.5);

    i2c_device_init(I2C_PORT, I2C_SDA, I2C_SCL);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    load_tab(I2C_PORT, Innit_NLG, Innit_NLG_count);
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

                    case CMD_READ_REGS: { // Wypisywanie wszystkich rejestrów NLG
                        dump_all_regs(I2C_PORT);
                        printf("OK\n");
                        break;
                    }

                    case CMD_DEFAULT_CONFIG: {
                        load_tab(I2C_PORT, MainPLL, MainPLL_count);
                        printf("Default configuration loaded\n");
                        break;
                    }

                    case CMD_LOAD_CONFIG: {
                        uint8_t q1_source, q2_source, q1_bypass, q2_bypass, Synth_mode;
                        uint16_t q1_freq, q2_freq;

                        sscanf(buffer,
                            "LOAD_CONFIG,%hhu,%hhu,%hhu,%hu,%hu",
                            &Synth_mode,
                            &q1_source,
                            &q2_source,
                            &q1_freq,
                            &q2_freq);

                        printf(
                            "CFG: outputs=%u Q1 src=%u freq=%u | Q2 src=%u freq=%u\n",
                            Synth_mode,
                            q1_source,
                            q1_freq,
                            q2_source,
                            q2_freq
                        );
                            // ── Q1 ──────────────────────────────────────────
                            if (q1_source == 0) {           // Crystal
                                if      (q1_freq == 1) { /* load_tab Crystal 1MHz  */ }
                                else if (q1_freq == 4) { /* load_tab Crystal 4MHz  */ }
                            }
                            else if (q1_source == 1) {      // External
                                if      (q1_freq == 1)  { /* load_tab External 1MHz  */ }
                                else if (q1_freq == 16) { /* load_tab External 16MHz */ }
                            }
                            else if (q1_source == 2) {      // PLL
                                if      (q1_freq == 25)  { /* load_tab PLL 25MHz  */ }
                                else if (q1_freq == 50)  { /* load_tab PLL 50MHz  */ }
                                else if (q1_freq == 100) { /* load_tab PLL 100MHz */ }
                                else if (q1_freq == 200) { /* load_tab PLL 200MHz */ }
                            }

                            // ── Q2 ──────────────────────────────────────────
                            if (q2_source == 0) {           // Crystal
                                if      (q2_freq == 1) { /* load_tab Crystal 1MHz  */ }
                                else if (q2_freq == 4) { /* load_tab Crystal 4MHz  */ }
                            }
                            else if (q2_source == 1) {      // External
                                if      (q2_freq == 1)  { /* load_tab External 1MHz  */ }
                                else if (q2_freq == 16) { /* load_tab External 16MHz */ }
                            }
                            else if (q2_source == 2) {      // PLL
                                if      (q2_freq == 25)  { /* load_tab PLL 25MHz  */ }
                                else if (q2_freq == 50)  { /* load_tab PLL 50MHz  */ }
                                else if (q2_freq == 100) { /* load_tab PLL 100MHz */ }
                                else if (q2_freq == 200) { /* load_tab PLL 200MHz */ }
                            }

                            if (Synth_mode == 1) {
                                // Logika na pll in jitter mode

                            }

                        printf("Load configuration successful\n");
                        break;
                    }

                    case CMD_IMPULSE_START: {

                        static uint8_t reg;
                        reg = 0x0F;

                        uint8_t output_en = 0, q1_single = 0, q2_single = 0;
                        sscanf(buffer, "IMPULSE_START,%hhu,%hhu,%hhu", &output_en, &q1_single, &q2_single);

                        printf(
                            "IMPULSE_START: output_en=%u, Q1=%s, Q2=%s\n",
                            output_en,
                            q1_single ? "Single" : "Continuous",
                            q2_single ? "Single" : "Continuous"
                        );

                        if (output_en & 0x04)
                        {
                            if (q1_single)
                            {
                                reg &= ~(1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1);
                                reg |= (1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1);
                            }
                            else
                            {
                                reg &= ~(1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1); //Q1
                            }
                        }

                        if (output_en & 0x08)
                        {
                            if (q2_single)
                            {
                                reg &= ~(1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1);
                                reg |= (1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1);
                            }
                            else
                            {
                                reg &= ~(1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1); // Q2
                            }
                        }
                        printf("Outputs: ");


                        if (output_en & 0x04)
                        {
                            printf("Q1 (%s) ",
                                q1_single ? "Single" : "Continuous");
                        }

                        if (output_en & 0x08)
                        {
                            printf("Q2 (%s) ",
                                q2_single ? "Single" : "Continuous");
                        }

                        printf("\n");
                        break;
                    }

                    case CMD_IMPULSE_STOP: { //Wyłącznie wyjść
                        reg = 0x0F;
                        i2c_write_reg16(I2C_PORT, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1);
                        printf("Impulse stopped\n");
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