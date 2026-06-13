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
#define MAX_CFG_ITEMS 256

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

pll_cfg_t cfg[MAX_CFG_ITEMS];
size_t cfg_count = 0;

void cfg_add(uint16_t reg, uint8_t value)
{
    for (size_t i = 0; i < cfg_count; i++)
    {
        if (cfg[i].reg == reg)
        {
            cfg[i].value |= value; // scalanie przez OR — nie kasuje żadnych bitów
            return;
        }
    }

    cfg[cfg_count].reg = reg;
    cfg[cfg_count].value = value;
    cfg_count++;
}


typedef enum {
    CMD_UNKNOWN,
    CMD_IMPULSE_START,
    CMD_IMPULSE_STOP,
    CMD_LOAD_CONFIG,
    CMD_READ_REGS,
    CMD_DEFAULT_CONFIG,
    CMD_Innital_Config,
    CMD_CALIBRATE_PLL,
    CMD_IMPULSE_GPIO_START
} CommandType;

static CommandType parse_command(const char *buffer)
{
    if (strncmp(buffer, "READ_REGS", 9) == 0)
        return CMD_READ_REGS;

    if (strncmp(buffer, "DEFAULT_CONFIG", 14) == 0)
        return CMD_DEFAULT_CONFIG;

    if (strncmp(buffer, "Innital_Config", 14) == 0)
        return CMD_Innital_Config;

    if (strncmp(buffer, "LOAD_CONFIG", 11) == 0)
        return CMD_LOAD_CONFIG;

    if (strncmp(buffer, "IMPULSE_START", 13) == 0)
        return CMD_IMPULSE_START;

    if (strncmp(buffer, "IMPULSE_STOP", 12) == 0)
        return CMD_IMPULSE_STOP;

    if (strncmp(buffer, "CALIBRATE_PLL", 13) == 0)
        return CMD_CALIBRATE_PLL;

    if (strncmp(buffer, "IMPULSE_GPIO_START", 18) == 0)
        return CMD_IMPULSE_GPIO_START;

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

    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2, 1);

    gpio_init(3);
    gpio_set_dir(3, GPIO_OUT);
    gpio_put(3, 1);

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

                    case CMD_Innital_Config: { // Wypisywanie wszystkich rejestrów NLG
                        load_tab(I2C_PORT, Innit_NLG, Innit_NLG_count);
                        sleep_ms(100);
                        load_tab(I2C_PORT, Constant_values, Constant_values_count);
                        printf("OK\n");
                        break;
                    }

                    case CMD_DEFAULT_CONFIG: {
                        load_tab(I2C_PORT, MainPLL, MainPLL_count);
                        printf("Default configuration loaded\n");
                        break;
                    }

                    case CMD_CALIBRATE_PLL: {
                        Flag_Clear(I2C_PORT);
                        printf("Calibrating PLL");
                        break;
                    }

                    case CMD_LOAD_CONFIG: {
                    uint8_t q1_source, q2_source, jitter_ant, output_en;
                    uint16_t q1_freq, q2_freq;

                    sscanf(buffer,
                        "LOAD_CONFIG,%hhu,%hhu,%hhu,%hhu,%hu,%hu",
                        &jitter_ant,
                        &output_en,
                        &q1_source,
                        &q2_source,
                        &q1_freq,
                        &q2_freq);

                    printf(
                        "CFG: out=0x%02X Q1 src=%u freq=%u | Q2 src=%u freq=%u\n",
                        output_en,
                        q1_source,
                        q1_freq,
                        q2_source,
                        q2_freq
                    );

                        cfg_count = 0;

                            // ── Q1 ──────────────────────────────────────────
                            if(output_en ==4 || output_en == 12) {
                                printf("Configuring Q1...\n");
                                if (q1_source == 0) {               // Crystal
                                    cfg_add(0x0C, 0x03);        
                                    cfg_add(0x0D, 0x0E);        
                                    cfg_add(0x12, 0x7A);        
                                    cfg_add(0x13, 0x30);        
                                    cfg_add(0x26, 0x46); 
                                    cfg_add(0x5F, 0x00);        // NFRAC_Q3
                                    cfg_add(0x63, 0x03);        // CLK_SEL (Wybór Crystal dla wyjścia Q2)
                                    cfg_add(0x46, 0x00);        // Analog PLL Control
                                if (q1_freq == 1) {             // Crystal 1MHz
                                    cfg_add(0x47, 0x0C);        // N_Q2 (Dzielnik całkowity: 1MHz)
                                    cfg_add(0x5B, 0x08);        // NFRAC_Q2 (Dzielnik ułamkowy: 1MHz)
                                }
                                else if (q1_freq == 4) {        // Crystal 4MHz             
                                    cfg_add(0x47, 0x03);        
                                    cfg_add(0x5B, 0x02);        
                                }
                                else if (q1_freq == 10000) {        // Crystal 10kHz             
                                    cfg_add(0x46, 0x04);        
                                    cfg_add(0x47, 0xE2);
                                    cfg_add(0x5B, 0x00);        
                                }
                            }
                                // ── Q1 ──────────────────────────────────
                                else if (q1_source == 1) {          // External Clock
                                    cfg_add(0x0C, 0x03);            // PRE0 (Dzielnik wstępny)
                                    cfg_add(0x0D, 0x0E);            // PRE0 [1, 2]
                                    cfg_add(0x12, 0x7A);            // M1 (Sprzężenie zwrotne)
                                    cfg_add(0x13, 0x30);            // M1
                                    cfg_add(0x26, 0x46);   
                                    cfg_add(0x63, 0x01);            // CLK_SEL (Input Ref 0 dla Q2 i Q3)
                                    cfg_add(0x5F, 0x00);                     
                                if (q1_freq == 1) {                 // External 1MHz
                                    cfg_add(0x47, 0x64);            // N_Q2 (Dzielnik całkowity: 1MHz)
                                    cfg_add(0x5B, 0x00);            // NFRAC_Q2 (Dzielnik ułamkowy)
                                }
                                else if (q1_freq == 16) {           // External 16MHz                               
                                    cfg_add(0x47, 0x06);            // N_Q2 (Dzielnik całkowity: 16MHz)         
                                    cfg_add(0x5B, 0x04);            // NFRAC_Q2 (Dzielnik ułamkowy: 16MHz)                 
                                }
                                }
                                // ── Q1 ──────────────────────────────────
                                    else if (q1_source == 2) { 
                                    cfg_add(0x0C, 0x06);
                                    cfg_add(0x0D, 0x1C);
                                    cfg_add(0x12, 0x6A);
                                    cfg_add(0x13, 0xEA);
                                    cfg_add(0x5B, 0x00);
                                    cfg_add(0x5F, 0x00);
                                    cfg_add(0x63, 0x00);
                                    if (q1_freq == 25) {
                                        cfg_add(0x26, 0x46);
                                        cfg_add(0x47, 0x46);
                                    }
                                    else if (q1_freq == 50) {
                                        cfg_add(0x26, 0x46);
                                        cfg_add(0x47, 0x23);
                                    }
                                    else if (q1_freq == 100) {
                                        cfg_add(0x26, 0x44);
                                        cfg_add(0x47, 0x11);
                                    }
                                    else if (q1_freq == 200) {
                                        cfg_add(0x26, 0x40);
                                        cfg_add(0x47, 0x08);

                                    }
                                    }
                                }
                                    // ── Q2 ──────────────────────────────────
                                if (output_en == 8 || output_en == 12) {
                                    printf("Configuring Q2...\n");
                                    if (q2_source == 0) {               // Crystal
                                            cfg_add(0x0C, 0x03);        // PRE0 (Dzielnik wstępny) [2, 5]
                                            cfg_add(0x0D, 0x0E);        // PRE0 [2, 5]
                                            cfg_add(0x12, 0x7A);        // M1 (Sprzężenie zwrotne) [2, 6]
                                            cfg_add(0x13, 0x30);        // M1 [2, 6]
                                            cfg_add(0x26, 0x46);        // DSM_INT (Modulator) [2, 7]
                                            cfg_add(0x5B, 0x00);        // NFRAC_Q2 (Część ułamkowa fizycznego Q2) [4, 11]      
                                            cfg_add(0x63, 0x30);        // CLK_SEL (Wybór Crystal dla wyjścia Q3) [4, 12, 14]
                                        if (q2_freq == 1) {             // Crystal 1MHz
                                            cfg_add(0x4A, 0x0C);        // N_Q3 (Dzielnik całkowity fizycznego Q3: 1MHz) [2, 9, 10]
                                            cfg_add(0x5F, 0x08);        // NFRAC_Q3 (Część ułamkowa fizycznego Q3: 1MHz) [4, 12, 13]
                                        }
                                        else if (q2_freq == 4) {        // Crystal 4MHz             
                                            cfg_add(0x4A, 0x03);        // N_Q3 (Dzielnik całkowity dla 4MHz) [2, 9]       
                                            cfg_add(0x5F, 0x02);        // NFRAC_Q3 (Część ułamkowa dla 4MHz) [4, 12]    
                                        }
                                        else if (q2_freq == 10000) {        // Crystal 10kHz             
                                            cfg_add(0x49, 0x04);        
                                            cfg_add(0x4A, 0xE2);  
                                            cfg_add(0x5F, 0x00);      
                                }
                                    }
                                    // ── Q2 ──────────────────────────────────
                                    else if (q2_source == 1) {              // External Clock
                                            cfg_add(0x0C, 0x03);            // PRE0 (Dzielnik wstępny) [2, 4]
                                            cfg_add(0x0D, 0x0E);            // PRE0 [2, 4]
                                            cfg_add(0x12, 0x7A);            // M1 (Sprzężenie zwrotne) [2, 5]
                                            cfg_add(0x13, 0x30);            // M1 [2, 5]
                                            cfg_add(0x26, 0x46);            // DSM_INT (Modulator) [2, 6]
                                            cfg_add(0x63, 0x10);            // CLK_SEL (Wybór Ref 0 dla wyjść Q2/Q3) [10, 11]
                                            cfg_add(0x5B, 0x00);            // NFRAC_Q2 [2, 9]
                                        if (q2_freq == 1) {                 // External 1MHz
                                            cfg_add(0x4A, 0x64);            // N_Q3 (Dzielnik całkowity dla 1MHz na Q3) [2, 7]
                                            cfg_add(0x5F, 0x00);            // NFRAC_Q3 (Dzielnik ułamkowy dla 1MHz) [2, 10]
                                        }
                                        else if (q2_freq == 16) {           // External 16MHz                     
                                            cfg_add(0x4A, 0x06);            // N_Q3 (Dzielnik całkowity dla 16MHz) [2, 7]          
                                            cfg_add(0x5F, 0x04);            // NFRAC_Q3 (Dzielnik ułamkowy dla 16MHz) [2, 10]           
                                        }
                                    }
                                        // ── Q2 ──────────────────────────────────
                                        else if (q2_source == 2) {              // PLL Mode
                                            cfg_add(0x0C, 0x06);
                                            cfg_add(0x0D, 0x1C);
                                            cfg_add(0x12, 0x6A);
                                            cfg_add(0x13, 0xEA);
                                            cfg_add(0x5B, 0x00);
                                            cfg_add(0x5F, 0x00);
                                            cfg_add(0x63, 0x00);
                                            if (q2_freq == 25) {
                                                cfg_add(0x26, 0x46);
                                                cfg_add(0x4A, 0x46);
                                            }
                                            else if (q2_freq == 50) {
                                                cfg_add(0x26, 0x46);
                                                cfg_add(0x4A, 0x23);
                                            }
                                            else if (q2_freq == 100) {
                                                cfg_add(0x26, 0x44);
                                                cfg_add(0x4A, 0x11);
                                            }
                                            else if (q2_freq == 200) {
                                                cfg_add(0x26, 0x40);
                                                cfg_add(0x4A, 0x08);
                                            }
                                        }
                                    }

                            if (jitter_ant == 1) {
                                // Tryb: Jitter Attenuator (na podstawie konfiguracji 'Clock')
                                cfg_add(0x69, 0x02); // SYN_MODE = 0
                                cfg_add(0x0A, 0x20); // STATE = Run Automatically
                                cfg_add(0x68, 0xE8); // Analog PLL Control
                                cfg_add(0x2F, 0xD0); // DSM configuration
                                cfg_add(0x73, 0x17); // LOS Monitoring active
                            } else {
                                // Tryb: Synthesizer (na podstawie konfiguracji 'Crystal' lub 'PLL')
                                cfg_add(0x69, 0x0A); // SYN_MODE = 1
                                cfg_add(0x0A, 0x31); // STATE = Force Freerun (wymagane dla syntezatora)
                                cfg_add(0x68, 0xE9); // Analog PLL Control
                                cfg_add(0x2F, 0x10); // DSM configuration
                                cfg_add(0x73, 0x17); // LOS Monitoring off
                            }



                        printf("\nGenerated configuration:\n");
                        printf("=======================\n");

                        for(size_t i = 0; i < cfg_count; i++)
                        {
                            printf("REG[0x%04X] = 0x%02X\n",
                                cfg[i].reg,
                                cfg[i].value);
                        }

                        load_tab(I2C_PORT, cfg, cfg_count);
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
                                reg |= (1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);

                                reg &= ~(1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                                printf("Q1 impulse started\n");

                            }
                            else
                            {
                                reg |= (1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1); //Q1
                                printf("Q1 impulse started\n");
                            }
                        }

                        if (output_en & 0x08)
                        {
                            if (q2_single)
                            {
                                reg |= (1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                                reg &= ~(1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                                printf("Q2 impulse started\n");
                            }
                            else
                            {
                                reg |= (1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1); // Q2
                                printf("Q2 impulse started\n");
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


                    case CMD_IMPULSE_GPIO_START: {

                        static uint8_t reg;
                        reg = 0x0F;
                        int Alternative = 1;

                        uint8_t output_en = 0, q1_single = 0, q2_single = 0;
                        sscanf(buffer, "IMPULSE_GPIO_START,%hhu,%hhu,%hhu", &output_en, &q1_single, &q2_single);

                        printf(
                            "IMPULSE_GPIO_START: output_en=%u, Q1=%s, Q2=%s\n",
                            output_en,
                            q1_single ? "Single" : "Continuous",
                            q2_single ? "Single" : "Continuous"
                        );
                        if(Alternative==0){
                        if (output_en & 0x04)
                        {
                            if (q1_single)
                            {                                
                                gpio_put(2, 0);
                                gpio_put(2, 1);
                                printf("Q1 impulse started\n");



                            }
                            else
                            {
                                gpio_put(2, 0);
                                printf("Q1 impulse started\n");
                            }
                        }

                        if (output_en & 0x08)
                        {
                            if (q2_single)
                            {
                                gpio_put(3, 0);
                                gpio_put(3, 1);
                                printf("Q2 impulse started\n");
                            }
                            else
                            {
                                gpio_put(3, 0);
                                printf("Q2 impulse started\n");
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
                            uint8_t value;
                        if (i2c_read_reg16(I2C_PORT, DEVICE_ADDR, REG_GPI_VAL_STATUS , &value, 1))
                            {
                                printf("REG[0x200C] = 0x%02X\n", value);
                            }

                        } else{
                            reg = 0x0C;
                            i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                        if (output_en & 0x04)
                        {
                            if (q1_single)
                            {                                
                                gpio_put(2, 0);
                                gpio_put(2, 1);
                                printf("Q1 impulse started\n");



                            }
                            else
                            {
                                gpio_put(2, 0);
                                printf("Q1 impulse started\n");
                            }
                        }

                        if (output_en & 0x08)
                        {
                            if (q2_single)
                            {
                                gpio_put(3, 0);
                                gpio_put(3, 1);
                                printf("Q2 impulse started\n");
                            }
                            else
                            {
                                gpio_put(3, 0);
                                printf("Q2 impulse started\n");
                            }
                        }

                        }

                        printf("\n");
                        break;
                    }


                    case CMD_IMPULSE_STOP: { //Wyłącznie wyjść
                        reg = 0x0F;
                        i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                        gpio_put(2, 1);
                        gpio_put(3, 1);
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