#include "NLG9881.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>

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

static const struct
{
    uint16_t reg;
    uint8_t value;
} pll_cfg1[] =
{
    {0x0008, 0x03},
    {REG_DPLL_PRIORITY_CTRL, 0x00},
    {REG_DPLL_STATE_CTRL, 0x31},
    {REG_DPLL_PRE0_MSB, 0x00},
    {REG_DPLL_PRE0_MID, 0x00},
    {REG_DPLL_PRE0_LSB, 0x01},
    {REG_DPLL_PRE1_MSB, 0x00},
    {REG_DPLL_PRE1_MID, 0x00},
    {REG_DPLL_PRE1_LSB, 0x01},
    {REG_DPLL_M1_0_MSB, 0x07},
    {REG_DPLL_M1_0_MID, 0x00},
    {REG_DPLL_M1_0_LSB, 0x00},
    {REG_DPLL_M1_1_MSB, 0x07},
    {REG_DPLL_M1_1_MID, 0x00},
    {REG_DPLL_M1_1_LSB, 0x00},
    {REG_DPLL_BW_CTRL, 0x77},
    {REG_DPLL_DAMP_GAIN_CTRL, 0x6D},
    {0x0019, 0x00},
    {0x001A, 0x00},
    {0x001B, 0x00},
    {0x001C, 0x00},
    {0x001D, 0x00},
    {0x001E, 0x00},
    {0x001F, 0xFF},
    {0x0020, 0xFF},
    {0x0021, 0xFF},
    {0x0022, 0xFF},
    {REG_DPLL_HOLD_FASTLCK, 0x03},
    {REG_DPLL_LOCK_WIN, 0x3F},
    {REG_DPLL_DSM_INT_MSB, 0x00},
    {REG_DPLL_DSM_INT_LSB, 0x50},
    {0x0027, 0x00},
    {REG_DPLL_DSM_FRAC_MSB, 0x00},
    {REG_DPLL_DSM_FRAC_MID, 0x00},
    {REG_DPLL_DSM_FRAC_LSB, 0x00},
    {0x002B, 0x00},
    {0x002C, 0x01},
    {0x002D, 0x00},
    {0x002E, 0x00},
    {REG_DPLL_DSM_ORD_GAIN, 0x10},
    {REG_GPIO_DIR_CTRL, 0x00},
    {REG_GPI_SEL_2, 0x00},
    {REG_GPI_SEL_1, 0x00},
    {REG_GPI_SEL_0, 0x00},
    {REG_GPO_SEL_2, 0x00},
    {REG_GPO_SEL_1, 0x00},
    {REG_GPO_SEL_0, 0x00},
    {0x0037, 0x00},
    {REG_GPO_OUTPUT_VAL, 0x00},
    {REG_OUTPUT_EN_CTRL, 0x02},
    {REG_OUTPUT_POL_CTRL, 0x00},
    {0x003B, 0x00},
    {0x003C, 0x00},
    {REG_OUTPUT_MODE_3_2, 0x66},
    {REG_OUTPUT_MODE_1_0, 0x66},
    {REG_DIV_INT_Q0_CTRL, 0x00},
    {REG_DIV_INT_Q0_NS2_MSB, 0x00},
    {REG_DIV_INT_Q0_NS2_LSB, 0x00},
    {REG_DIV_INT_Q1_MSB, 0x00},
    {REG_DIV_INT_Q1_MID, 0x03},
    {REG_DIV_INT_Q1_LSB, 0xE8},
    {REG_DIV_INT_Q2_MSB, 0x00},
    {REG_DIV_INT_Q2_MID, 0x00},
    {REG_DIV_INT_Q2_LSB, 0x00},
    {REG_DIV_INT_Q3_MSB, 0x00},
    {REG_DIV_INT_Q3_MID, 0x00},
    {REG_DIV_INT_Q3_LSB, 0x00},
    {0x004B, 0x00},
    {0x004C, 0x00},
    {0x004D, 0x00},
    {0x004E, 0x00},
    {0x004F, 0x00},
    {0x0050, 0x00},
    {0x0051, 0x00},
    {0x0052, 0x00},
    {0x0053, 0x00},
    {0x0054, 0x00},
    {0x0055, 0x00},
    {0x0056, 0x00},
    {REG_DIV_FRAC_Q1_MSB, 0x00},
    {REG_DIV_FRAC_Q1_MID1, 0x00},
    {REG_DIV_FRAC_Q1_MID0, 0x00},
    {REG_DIV_FRAC_Q1_LSB, 0x00},
    {REG_DIV_FRAC_Q2_MSB, 0x00},
    {REG_DIV_FRAC_Q2_MID1, 0x00},
    {REG_DIV_FRAC_Q2_MID0, 0x00},
    {REG_DIV_FRAC_Q2_LSB, 0x00},
    {REG_DIV_FRAC_Q3_MSB, 0x00},
    {REG_DIV_FRAC_Q3_MID1, 0x00},
    {REG_DIV_FRAC_Q3_MID0, 0x00},
    {REG_DIV_FRAC_Q3_LSB, 0x00},
    {REG_OUT_CLK_SRC_SYNC, 0x00},
    {REG_APLL_CTRL_0, 0xE9},
    {REG_APLL_CTRL_1, 0x0A},
    {REG_APLL_CTRL_2, 0x2B},
    {0x006B, 0x20},
    {REG_PWR_DN_CTRL_0, 0x00},
    {REG_PWR_DN_CTRL_1, 0x00},
    {0x006E, 0x00},
    {REG_PWR_DN_CTRL_2, 0x0D},
    {REG_PWR_DN_CTRL_3, 0x00},
    {REG_IN_MON_LOS0_MSB, 0x00},
    {REG_IN_MON_LOS0_MID, 0x00},
    {REG_IN_MON_LOS0_LSB, 0x00},
    {REG_IN_MON_LOS1_MSB, 0x00},
    {REG_IN_MON_LOS1_MID, 0x00},
    {REG_IN_MON_LOS1_LSB, 0x00},
    {0x0077, 0x00},
    {0x0078, 0x00},
    {REG_INT_EN_CTRL, 0x00},
    {REG_FACTORY_SETTING_0, 0x27},
    {REG_FACTORY_SETTING_1, 0xCC},
};
static const struct
{
    uint16_t reg;
    uint8_t value;
} pll_cfg[] =
{
    {0x0008, 0x03},

    {REG_DPLL_PRIORITY_CTRL,    0x00},
    {REG_DPLL_STATE_CTRL,       0x00},

    {REG_DPLL_PRE0_MSB,         0x00},
    {REG_DPLL_PRE0_MID,         0x00},
    {REG_DPLL_PRE0_LSB,         0x07},

    {REG_DPLL_PRE1_MSB,         0x00},
    {REG_DPLL_PRE1_MID,         0x00},
    {REG_DPLL_PRE1_LSB,         0x07},

    {REG_DPLL_M1_0_MSB,         0x00},
    {REG_DPLL_M1_0_MID,         0x00},
    {REG_DPLL_M1_0_LSB,         0x77},

    {REG_DPLL_M1_1_MSB,         0x6D},
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

    {REG_DPLL_DSM_ORD_GAIN,     0xD0},

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
    {REG_DIV_INT_Q1_MID,        0x0C},
    {REG_DIV_INT_Q1_LSB,        0x00},

    {REG_DIV_INT_Q2_MSB,        0x00},
    {REG_DIV_INT_Q2_MID,        0x00},
    {REG_DIV_INT_Q2_LSB,        0x14},

    {REG_DIV_INT_Q3_MSB,        0x00},
    {REG_DIV_INT_Q3_MID,        0x00},
    {REG_DIV_INT_Q3_LSB,        0x00},

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

    {REG_OUT_CLK_SRC_SYNC,      0x03},

    {REG_APLL_CTRL_0,           0x8A},
    {REG_APLL_CTRL_1,           0x02},
    {REG_APLL_CTRL_2,           0x2B},

    {0x006B,                    0x20},

    {REG_PWR_DN_CTRL_0,         0x00},
    {REG_PWR_DN_CTRL_1,         0x00},
    {0x006E,                    0x00},
    {REG_PWR_DN_CTRL_2,         0x01},
    {REG_PWR_DN_CTRL_3,         0x00},

    {REG_IN_MON_LOS0_MSB,       0x00},
    {REG_IN_MON_LOS0_MID,       0x00},
    {REG_IN_MON_LOS0_LSB,       0x00},

    {REG_IN_MON_LOS1_MSB,       0x00},
    {REG_IN_MON_LOS1_MID,       0x00},
    {REG_IN_MON_LOS1_LSB,       0x00},

    {0x0077,                    0x00},
    {0x0078,                    0x00},

    {REG_INT_EN_CTRL,           0x00},

    {REG_FACTORY_SETTING_0,     0x27},
    {REG_FACTORY_SETTING_1,     0xCC},
    {REG_INT_STATUS_0,          0x00},

};
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

bool pll_enable_q1_led(i2c_inst_t *i2c)
{
    uint8_t reg;
    uint8_t status;

    printf("=====================================\n");
    printf("Configuring Q1 LED output\n");
    printf("=====================================\n");

    // ========================================
    // 1. SYNTHESIZER MODE
    // ========================================

    printf("Setting SYN_MODE...\n");

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, REG_APLL_CTRL_1, &reg, 1))
    {
        printf("ERROR: Cannot read REG_APLL_CTRL_1\n");
        return false;
    }

    reg |= (1 << 4); // SYN_MODE = 1

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_APLL_CTRL_1, &reg, 1))
    {
        printf("ERROR: Cannot write REG_APLL_CTRL_1\n");
        return false;
    }

    printf("SYN_MODE enabled\n");

    // ========================================
    // 2. FORCE FREERUN
    // ========================================

    printf("Setting FREERUN mode...\n");

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, REG_DPLL_STATE_CTRL, &reg, 1))
    {
        printf("ERROR: Cannot read REG_DPLL_STATE_CTRL\n");
        return false;
    }

    reg &= ~0x03;
    reg |= 0x01; // STATE = FREERUN

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_DPLL_STATE_CTRL, &reg, 1))
    {
        printf("ERROR: Cannot write REG_DPLL_STATE_CTRL\n");
        return false;
    }

    printf("FREERUN enabled\n");

    // ========================================
    // 3. POWER UP Q1
    // ========================================

    printf("Powering up Q1...\n");

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1))
    {
        printf("ERROR: Cannot read REG_PWR_DN_CTRL_2\n");
        return false;
    }

    reg &= ~(1 << 1); // Q1_DIS = 0

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_PWR_DN_CTRL_2, &reg, 1))
    {
        printf("ERROR: Cannot write REG_PWR_DN_CTRL_2\n");
        return false;
    }

    printf("Q1 powered up\n");

    // ========================================
    // 4. ENABLE OUTPUT DRIVER
    // ========================================

    printf("Enabling Q1 driver...\n");

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, REG_OUTPUT_EN_CTRL, &reg, 1))
    {
        printf("ERROR: Cannot read REG_OUTPUT_EN_CTRL\n");
        return false;
    }

    reg |= (1 << 1); // OUTEN1

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_OUTPUT_EN_CTRL, &reg, 1))
    {
        printf("ERROR: Cannot write REG_OUTPUT_EN_CTRL\n");
        return false;
    }

    printf("Q1 driver enabled\n");

    // ========================================
    // 5. Q1 = LVCMOS
    // ========================================

    printf("Configuring Q1 as LVCMOS...\n");

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, REG_OUTPUT_MODE_1_0, &reg, 1))
    {
        printf("ERROR: Cannot read REG_OUTPUT_MODE_1_0\n");
        return false;
    }

    printf("Current OUTPUT_MODE_1_0 = 0x%02X\n", reg);

    // Wyczyść konfigurację Q1 (D7:D4)
    reg &= 0x0F;

    // OUTMODE1 = 011 (LVCMOS)
    reg |= (0x3 << 5);

    // SE_MODE1 = 0
    reg &= ~(1 << 4);

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_OUTPUT_MODE_1_0, &reg, 1))
    {
        printf("ERROR: Cannot write REG_OUTPUT_MODE_1_0\n");
        return false;
    }

    printf("Q1 configured as LVCMOS\n");
    printf("OUTPUT_MODE_1_0 = 0x%02X\n", reg);

    // ========================================
    // 6. SYNCHRONIZE OUTPUT DIVIDERS
    // ========================================

    printf("Synchronizing output dividers...\n");

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, REG_OUT_CLK_SRC_SYNC, &reg, 1))
    {
        printf("ERROR: Cannot read REG_OUT_CLK_SRC_SYNC\n");
        return false;
    }

    reg |= (1 << 7);

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_OUT_CLK_SRC_SYNC, &reg, 1))
    {
        printf("ERROR: Failed to set PLL_SYN\n");
        return false;
    }

    sleep_ms(1);

    reg &= ~(1 << 7);

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, REG_OUT_CLK_SRC_SYNC, &reg, 1))
    {
        printf("ERROR: Failed to clear PLL_SYN\n");
        return false;
    }

    printf("Output dividers synchronized\n");

    // ========================================
    // 7. STATUS CHECK
    // ========================================

    printf("Reading status registers...\n");

    if (i2c_read_reg16(i2c, DEVICE_ADDR, REG_INT_STATUS_0, &status, 1))
    {
        printf("INT_STATUS_0 = 0x%02X\n", status);

        if (status & (1 << 6))
            printf("WARNING: LOL active\n");

        if (status & (1 << 4))
            printf("WARNING: HOLD active\n");

        if (status & (1 << 1))
            printf("WARNING: LOS1 active\n");

        if (status & (1 << 0))
            printf("WARNING: LOS0 active\n");
    }

    if (i2c_read_reg16(i2c, DEVICE_ADDR, REG_BOOT_STATUS_0, &status, 1))
    {
        printf("BOOT_STATUS_0 = 0x%02X\n", status);
    }

    if (i2c_read_reg16(i2c, DEVICE_ADDR, REG_BOOT_STATUS_1, &status, 1))
    {
        printf("BOOT_STATUS_1 = 0x%02X\n", status);
    }

    printf("=====================================\n");
    printf("Q1 configuration complete\n");
    printf("=====================================\n");

    return true;
}


bool pll_load_tcs(i2c_inst_t *i2c)
{
    printf("Loading TCS config...\n");

for(size_t i = 0; i < sizeof(pll_cfg)/sizeof(pll_cfg[0]); i++)
{
    uint8_t value = pll_cfg[i].value;

    if(!i2c_write_reg16(i2c,
                        DEVICE_ADDR,
                        pll_cfg[i].reg,
                        &value,
                        1))
    {
        printf("Skip reg 0x%04X\n", pll_cfg[i].reg);
    }
}

    printf("Config loaded successfully\n");
   

    uint8_t v;

// Output divider sync
i2c_read_reg16(i2c1, DEVICE_ADDR, REG_OUT_CLK_SRC_SYNC, &v, 1);

v |= 0x80;

i2c_write_reg16(i2c1, DEVICE_ADDR, REG_OUT_CLK_SRC_SYNC, &v, 1);
uint8_t reg_val;
sleep_ms(10);
// Konfiguracja Q2 (bity 1:0) na CLK0 (01b)
reg_val &= ~0x03;
reg_val |= 0x01;

i2c_write_reg16(i2c, DEVICE_ADDR, 0x0063, &reg_val, 1);
// Odczyt przed skasowaniem
i2c_read_reg16(i2c1, DEVICE_ADDR, REG_INT_STATUS_0, &v, 1);
printf("Before clear = 0x%02X\n", v);

// Skasuj LOL_INT (bit 6)
uint8_t clr = 0x40;
i2c_write_reg16(i2c1, DEVICE_ADDR, REG_INT_STATUS_0, &clr, 1);

// Odczyt po skasowaniu
i2c_read_reg16(i2c1, DEVICE_ADDR, REG_INT_STATUS_0, &v, 1);
printf("After clear = 0x%02X\n", v);
i2c_write_reg16(i2c1, DEVICE_ADDR, REG_OUT_CLK_SRC_SYNC, &v, 1);
 return true;
}