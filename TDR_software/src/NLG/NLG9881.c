#include "NLG9881.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Uruchomienie I2C i konfiguracja pinów
void i2c_device_init(i2c_inst_t *i2c, uint sda, uint scl) {
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    sleep_ms(200);
}


// zapis do rejestru 16-bit
bool i2c_write_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len) {
    uint8_t buffer[258];

    buffer[0] = (reg >> 8) & 0xFF;
    buffer[1] = reg & 0xFF;

    for (size_t i = 0; i < len; i++) {
        buffer[2 + i] = data[i];
    }

    int ret = i2c_write_timeout_us(i2c, addr, buffer, len + 2, false, 1000000); // 1s timeout
    return (ret >= 0);
}

// odczyt z rejestru 16-bit
bool i2c_read_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len) {
    uint8_t reg_buf[2];

    reg_buf[0] = (reg >> 8) & 0xFF;
    reg_buf[1] = reg & 0xFF;

    // ustaw adres rejestru
    if (i2c_write_timeout_us(i2c, addr, reg_buf, 2, true, 1000000) < 0) { // 1s timeout
        return false;
    }

    // czytaj dane
    if (i2c_read_timeout_us(i2c, addr, data, len, false, 1000000) < 0) { // 1s timeout
        return false;
    }

    return true;
}


const pll_cfg_t Innit_NLG[] =
{
    {REG_STARTUP_CTRL_0,        0x09},
    {REG_STARTUP_CTRL_1,        0x50},

    //0x02 - 0x07 - Do not touch

    {REG_DPLL_CTRL_0,           0x03},

    {REG_DPLL_PRIORITY_CTRL,    0x00},
    {REG_DPLL_STATE_CTRL,       0x31}, //important

    {REG_DPLL_PRE0_MSB,         0x00},
    {REG_DPLL_PRE0_MID,         0x00},
    {REG_DPLL_PRE0_LSB,         0x01},

    {REG_DPLL_PRE1_MSB,         0x00},
    {REG_DPLL_PRE1_MID,         0x00},
    {REG_DPLL_PRE1_LSB,         0x01},

    {REG_DPLL_M1_0_MSB,         0x07},
    {REG_DPLL_M1_0_MID,         0x00},
    {REG_DPLL_M1_0_LSB,         0x00},

    {REG_DPLL_M1_1_MSB,         0x07},
    {REG_DPLL_M1_1_MID,         0x00},
    {REG_DPLL_M1_1_LSB,         0x00},

    
    {REG_DPLL_BW_CTRL,          0x77},
    {REG_DPLL_DAMP_GAIN_CTRL,   0x6D},

    //Reserved
    {0x0019,                    0x00},
    {0x001A,                    0x00},
    {0x001B,                    0x00},
    {0x001C,                    0x00},
    {0x001D,                    0x00},
    {0x001E,                    0x00},

    {0x001F,                    0xFF},
    {0x0020,                    0xFF},
    {0x0021,                    0xFF},
    {0x0022,                    0xFF},


    {REG_DPLL_HOLD_FASTLCK,     0x03},
    {REG_DPLL_LOCK_WIN,         0x3F},

    {REG_DPLL_DSM_INT_MSB,      0x00},
    {REG_DPLL_DSM_INT_LSB,      0x2D}, // DSM_INT = 80

    {0x0027,                    0x00},

    {REG_DPLL_DSM_FRAC_MSB,     0x00},
    {REG_DPLL_DSM_FRAC_MID,     0x00},
    {REG_DPLL_DSM_FRAC_LSB,     0x00},

    {0x002B,                    0x00},
    {0x002C,                    0x01},
    {0x002D,                    0x00},
    {0x002E,                    0x00},

    {REG_DPLL_DSM_ORD_GAIN,     0x10},

    {REG_GPIO_DIR_CTRL,         0x00},
    {REG_GPI_SEL_2,             0x00},
    {REG_GPI_SEL_1,             0x00},
    {REG_GPI_SEL_0,             0x00},
    {REG_GPO_SEL_2,             0x00},
    {REG_GPO_SEL_1,             0x00},
    {REG_GPO_SEL_0,             0x00},

    {0x0037,                    0x00},
    {REG_GPO_OUTPUT_VAL,        0x00},

    {REG_OUTPUT_EN_CTRL,        0x04},
    {REG_OUTPUT_POL_CTRL,       0x00},

    {0x003B,                    0x00},
    {0x003C,                    0x00},

    {REG_OUTPUT_MODE_3_2,       0x66},
    {REG_OUTPUT_MODE_1_0,       0x66},

    {REG_DIV_INT_Q0_CTRL,       0x00},
    {REG_DIV_INT_Q0_NS2_MSB,    0x00},
    {REG_DIV_INT_Q0_NS2_LSB,    0x00},

    // Q1 divider = 20
    {REG_DIV_INT_Q1_MSB,        0x00},
    {REG_DIV_INT_Q1_MID,        0x00},
    {REG_DIV_INT_Q1_LSB,        0x00},

    {REG_DIV_INT_Q2_MSB,        0x00},
    {REG_DIV_INT_Q2_MID,        0x00},
    {REG_DIV_INT_Q2_LSB,        0x00},

    {REG_DIV_INT_Q3_MSB,        0x00},
    {REG_DIV_INT_Q3_MID,        0x00},
    {REG_DIV_INT_Q3_LSB,        0x00},

    //Reserved
    {0x004B,                    0x00},
    {0x004C,                    0x00},
    {0x004D,                    0x00},
    {0x004E,                    0x00},
    {0x004F,                    0x00},
    {0x0050,                    0x00},
    {0x0051,                    0x00},
    {0x0052,                    0x00},
    {0x0053,                    0x00},
    {0x0054,                    0x00},
    {0x0055,                    0x00},
    {0x0056,                    0x00},

    {REG_DIV_FRAC_Q1_MSB,       0x00},
    {REG_DIV_FRAC_Q1_MID1,      0x00},
    {REG_DIV_FRAC_Q1_MID0,      0x00},
    {REG_DIV_FRAC_Q1_LSB,       0x00},

    {REG_DIV_FRAC_Q2_MSB,       0x00},
    {REG_DIV_FRAC_Q2_MID1,      0x00},
    {REG_DIV_FRAC_Q2_MID0,      0x00},
    {REG_DIV_FRAC_Q2_LSB,       0x00},

    {REG_DIV_FRAC_Q3_MSB,       0x00},
    {REG_DIV_FRAC_Q3_MID1,      0x00},
    {REG_DIV_FRAC_Q3_MID0,      0x00},
    {REG_DIV_FRAC_Q3_LSB,       0x00},

    {REG_OUT_CLK_SRC_SYNC,      0x00}, //Ustawia źródło zegara na wyjściu

    //reserved
    {0x0064,                    0x00},
    {0x0065,                    0x00},
    {0x0066,                    0x00},
    {0x0067,                    0x00},

    {REG_APLL_CTRL_0,           0xE9},
    {REG_APLL_CTRL_1,           0x0A},
    {REG_APLL_CTRL_2,           0x2B},

    {0x006B,                    0x20},

    {REG_PWR_DN_CTRL_0,         0x00},
    {REG_PWR_DN_CTRL_1,         0x00}, //zmiana względer TIMCOM
    {0x006E,                    0x00},
    {REG_PWR_DN_CTRL_2,         0x0F},
    {REG_PWR_DN_CTRL_3,         0x00}, //Wyłączanie PLL

    {REG_IN_MON_LOS0_MSB,       0x00},
    {REG_IN_MON_LOS0_MID,       0x00},
    {REG_IN_MON_LOS0_LSB,       0x00}, //zmianna względem TIMCOM

    {REG_IN_MON_LOS1_MSB,       0x00},
    {REG_IN_MON_LOS1_MID,       0x00},
    {REG_IN_MON_LOS1_LSB,       0x00},

    //reservced
    {0x0077,                    0x00},
    {0x0078,                    0x00},

    {REG_INT_EN_CTRL,           0x00},

    {REG_FACTORY_SETTING_0,     0x27},
    {REG_FACTORY_SETTING_1,     0xCC},

    //flagi
    {REG_INT_STATUS_0,          0x53},
    {REG_INT_STATUS_1,          0x00},
    //{REG_GPI_VAL_STATUS,        0x00},
    //{REG_GLOBAL_INT_STATUS,     0x00},
    //{REG_BOOT_STATUS_0,         0x00},
    //{REG_BOOT_STATUS_1,         0x00},

};
const size_t Innit_NLG_count = sizeof(Innit_NLG) / sizeof(Innit_NLG[0]);

const pll_cfg_t MainPLL[] =
{
    {REG_STARTUP_CTRL_0,        0x09},
    {REG_STARTUP_CTRL_1,        0x50},

    //0x02 - 0x07 - Do not touch

    {REG_DPLL_CTRL_0,           0x03},

    {REG_DPLL_PRIORITY_CTRL,    0x00},
    {REG_DPLL_STATE_CTRL,       0x31}, //important

    {REG_DPLL_PRE0_MSB,         0x00},
    {REG_DPLL_PRE0_MID,         0x00},
    {REG_DPLL_PRE0_LSB,         0x4F},

    {REG_DPLL_PRE1_MSB,         0x00},
    {REG_DPLL_PRE1_MID,         0x00},
    {REG_DPLL_PRE1_LSB,         0x01},

    {REG_DPLL_M1_0_MSB,         0x00},
    {REG_DPLL_M1_0_MID,         0x7B},
    {REG_DPLL_M1_0_LSB,         0x70},

    {REG_DPLL_M1_1_MSB,         0x07},
    {REG_DPLL_M1_1_MID,         0x00},
    {REG_DPLL_M1_1_LSB,         0x00},

    
    {REG_DPLL_BW_CTRL,          0x77},
    {REG_DPLL_DAMP_GAIN_CTRL,   0x6D},

    //Reserved
    {0x0019,                    0x00},
    {0x001A,                    0x00},
    {0x001B,                    0x00},
    {0x001C,                    0x00},
    {0x001D,                    0x00},
    {0x001E,                    0x00},

    {0x001F,                    0xFF},
    {0x0020,                    0xFF},
    {0x0021,                    0xFF},
    {0x0022,                    0xFF},


    {REG_DPLL_HOLD_FASTLCK,     0x03},
    {REG_DPLL_LOCK_WIN,         0x3F},

    {REG_DPLL_DSM_INT_MSB,      0x00},
    {REG_DPLL_DSM_INT_LSB,      0x50}, // DSM_INT = 80

    {0x0027,                    0x00},

    {REG_DPLL_DSM_FRAC_MSB,     0x00},
    {REG_DPLL_DSM_FRAC_MID,     0x00},
    {REG_DPLL_DSM_FRAC_LSB,     0x00},

    {0x002B,                    0x00},
    {0x002C,                    0x01},
    {0x002D,                    0x00},
    {0x002E,                    0x00},

    {REG_DPLL_DSM_ORD_GAIN,     0x10},

    {REG_GPIO_DIR_CTRL,         0x00},
    {REG_GPI_SEL_2,             0x00},
    {REG_GPI_SEL_1,             0x00},
    {REG_GPI_SEL_0,             0x00},
    {REG_GPO_SEL_2,             0x00},
    {REG_GPO_SEL_1,             0x00},
    {REG_GPO_SEL_0,             0x00},

    {0x0037,                    0x00},
    {REG_GPO_OUTPUT_VAL,        0x00},

    {REG_OUTPUT_EN_CTRL,        0x0E},
    {REG_OUTPUT_POL_CTRL,       0x00},

    {0x003B,                    0x00},
    {0x003C,                    0x00},

    {REG_OUTPUT_MODE_3_2,       0x66},
    {REG_OUTPUT_MODE_1_0,       0x66},

    {REG_DIV_INT_Q0_CTRL,       0x00},
    {REG_DIV_INT_Q0_NS2_MSB,    0x00},
    {REG_DIV_INT_Q0_NS2_LSB,    0x00},

    // Q1 divider = 20
    {REG_DIV_INT_Q1_MSB,        0x00},
    {REG_DIV_INT_Q1_MID,        0x01},
    {REG_DIV_INT_Q1_LSB,        0x90},

    {REG_DIV_INT_Q2_MSB,        0x00},
    {REG_DIV_INT_Q2_MID,        0x00},
    {REG_DIV_INT_Q2_LSB,        0x14},

    {REG_DIV_INT_Q3_MSB,        0x00},
    {REG_DIV_INT_Q3_MID,        0x00},
    {REG_DIV_INT_Q3_LSB,        0x06},

    //Reserved
    {0x004B,                    0x00},
    {0x004C,                    0x00},
    {0x004D,                    0x00},
    {0x004E,                    0x00},
    {0x004F,                    0x00},
    {0x0050,                    0x00},
    {0x0051,                    0x00},
    {0x0052,                    0x00},
    {0x0053,                    0x00},
    {0x0054,                    0x00},
    {0x0055,                    0x00},
    {0x0056,                    0x00},

    {REG_DIV_FRAC_Q1_MSB,       0x00},
    {REG_DIV_FRAC_Q1_MID1,      0x00},
    {REG_DIV_FRAC_Q1_MID0,      0x00},
    {REG_DIV_FRAC_Q1_LSB,       0x00},

    {REG_DIV_FRAC_Q2_MSB,       0x00},
    {REG_DIV_FRAC_Q2_MID1,      0x00},
    {REG_DIV_FRAC_Q2_MID0,      0x00},
    {REG_DIV_FRAC_Q2_LSB,       0x00},

    {REG_DIV_FRAC_Q3_MSB,       0x00},
    {REG_DIV_FRAC_Q3_MID1,      0x00},
    {REG_DIV_FRAC_Q3_MID0,      0x00},
    {REG_DIV_FRAC_Q3_LSB,       0x00},

    {REG_OUT_CLK_SRC_SYNC,      0x30}, //Ustawia źródło zegara na wyjściu

    //reserved
    {0x0064,                    0x00},
    {0x0065,                    0x00},
    {0x0066,                    0x00},
    {0x0067,                    0x00},

    {REG_APLL_CTRL_0,           0xE9},
    {REG_APLL_CTRL_1,           0x0A},
    {REG_APLL_CTRL_2,           0x2B},

    {0x006B,                    0x20},

    {REG_PWR_DN_CTRL_0,         0x00},
    {REG_PWR_DN_CTRL_1,         0x01}, //zmiana względer TIMCOM
    {0x006E,                    0x00},
    {REG_PWR_DN_CTRL_2,         0x01},
    {REG_PWR_DN_CTRL_3,         0x00}, //Wyłączanie PLL

    {REG_IN_MON_LOS0_MSB,       0x00},
    {REG_IN_MON_LOS0_MID,       0x00},
    {REG_IN_MON_LOS0_LSB,       0x17}, //zmianna względem TIMCOM

    {REG_IN_MON_LOS1_MSB,       0x00},
    {REG_IN_MON_LOS1_MID,       0x00},
    {REG_IN_MON_LOS1_LSB,       0x00},

    //reservced
    {0x0077,                    0x00},
    {0x0078,                    0x00},

    {REG_INT_EN_CTRL,           0x00},

    {REG_FACTORY_SETTING_0,     0x27},
    {REG_FACTORY_SETTING_1,     0xCC},

    //flagi
    {REG_INT_STATUS_0,          0x53},
    {REG_INT_STATUS_1,          0x00},
    //{REG_GPI_VAL_STATUS,        0x00},
    //{REG_GLOBAL_INT_STATUS,     0x00},
    //{REG_BOOT_STATUS_0,         0x00},
    //{REG_BOOT_STATUS_1,         0x00},

};

const size_t MainPLL_count = sizeof(MainPLL) / sizeof(MainPLL[0]);

void Flag_Clear(i2c_inst_t *i2c)
{
    uint8_t clr = 0x53;
    i2c_write_reg16(i2c, DEVICE_ADDR, REG_INT_STATUS_0, &clr, 1);
}

//Wypisuje wszystkie rejestry
void dump_all_regs(i2c_inst_t *i2c)
{
    uint8_t data;

    printf("\n==============================\n");
    printf("Rejestry 8T49N241\n");
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


bool load_tab(i2c_inst_t *i2c, const pll_cfg_t *cfg, size_t count)
{
    printf("Loading config...\n");

    for (size_t i = 0; i < count; i++)
    {
        uint16_t reg = cfg[i].reg;
        uint8_t value = cfg[i].value;

        if (!i2c_write_reg16(i2c, DEVICE_ADDR, reg, &value, 1))
        {
            printf("Skip reg 0x%04X\n", reg);
        }
    }

    printf("Config loaded successfully\n");

    return true;
}