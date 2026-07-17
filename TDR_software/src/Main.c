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

static uint8_t reg;

pll_cfg_t cfg[MAX_CFG_ITEMS];
size_t cfg_count = 0;
bool GPIO_ON;

void cfg_add(uint16_t reg, uint8_t value)
{
    for (size_t i = 0; i < cfg_count; i++)
    {
        if (cfg[i].reg == reg)
        {
            cfg[i].value |= value;
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
    CMD_READ_REGS,
    CMD_Innital_Config,
    CMD_CALIBRATE_PLL,
    CMD_PING
} CommandType;

static CommandType parse_command(const char *buffer)
{
    if (strncmp(buffer, "READ_REGS", 9) == 0)
        return CMD_READ_REGS;

    if (strncmp(buffer, "Innital_Config", 14) == 0)
        return CMD_Innital_Config;

    if (strncmp(buffer, "IMPULSE_START", 13) == 0)
        return CMD_IMPULSE_START;

    if (strncmp(buffer, "IMPULSE_STOP", 12) == 0)
        return CMD_IMPULSE_STOP;

    if (strncmp(buffer, "CALIBRATE_PLL", 13) == 0)
        return CMD_CALIBRATE_PLL;

    if (strncmp(buffer, "PING", 4) == 0)
        return CMD_PING;
        
    return CMD_UNKNOWN;
}

void innit(){
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    i2c_device_init(I2C_PORT, I2C_SDA, I2C_SCL);

}

int main()
{

    innit();

    //inicialization of NLG9881 and I2C

    char buffer[64];
    int buffer_index = 0;

    while (true) {
        int ch = getchar_timeout_us(0);
        
        if (ch != PICO_ERROR_TIMEOUT) {
            if (ch == '\n') {
                buffer[buffer_index] = 0;
                buffer_index = 0;
                

                CommandType command = parse_command(buffer);

                switch (command) {

                    case CMD_READ_REGS: {
                        dump_all_regs(I2C_PORT);
                        printf("OK\n");
                        break;
                    }

                    case CMD_Innital_Config: {
                        load_tab(I2C_PORT, Constant_values, Constant_values_count);
                        printf("OK\n");
                        break;
                    }

                    case CMD_CALIBRATE_PLL: {
                        Flag_Clear(I2C_PORT);
                        printf("Calibrating PLL");
                        break;
                    }
                    
                    case CMD_PING: {
                        printf("PONG\n");
                        break;
                    }

                    case CMD_IMPULSE_START: {

                        static uint8_t reg;
                        reg = 0x00;

                        uint8_t output_en = 0, q1_single = 0, q2_single = 0;
                        sscanf(buffer, "IMPULSE_START,%hhu,%hhu,%hhu", &output_en, &q1_single, &q2_single);

                        if (output_en & 0x04)
                        {
                            if (q1_single)
                            {                                
                                reg |= (1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);

                                reg &= ~(1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                                printf("Q1 impulse started\n");

                            }
                            else
                            {
                                reg |= (1 << 3);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1); //Q1
                                printf("Q1 impulse started\n");
                            }
                        }

                        if (output_en & 0x08)
                        {
                            if (q2_single)
                            {
                                reg |= (1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                                reg &= ~(1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
                                printf("Q2 impulse started\n");
                            }
                            else
                            {
                                reg |= (1 << 2);
                                i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1); // Q2
                                printf("Q2 impulse started\n");
                            }
                        }
                        break;
                }

                    case CMD_IMPULSE_STOP: {
                        reg = 0x00;
                        i2c_write_reg16(I2C_PORT, DEVICE_ADDR, 0x39, &reg, 1);
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