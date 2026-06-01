#ifndef I2C_DEVICE_H
#define I2C_DEVICE_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Hardware address (S_A0=0, S_A1=0)
#define DEVICE_ADDR 0x7C

// Startup & Device ID Control
#define REG_STARTUP_CTRL_0        0x0000 // R/W Boot control:  0=full OTP+EEPROM, 1=full OTP only, 2=8B OTP+EEPROM, 3=8B OTP only Default: 0x01
#define REG_STARTUP_CTRL_1        0x0001 // R/W - External EEPROM addressing configuration: b[7] (1-tryb 15-bit 0-Standard), b[6:0] (adres I2C)


//Device ID, serial number 
#define REG_DEVICE_REV_ID         0x0002 // R   - Revision ID and DEV_ID[15:12]
#define REG_DEVICE_ID_11_4        0x0003 // R   - DEV_ID[11:4]
#define REG_DEVICE_ID_DASH_CODE   0x0004 // R   - DEV_ID[3:0] and DASH_CODE[10:7]
#define REG_DASH_CODE_LSB         0x0005 // R/W - DASH_CODE[6:0] -Do not overwrite


// Serial Interface Control
#define REG_SERIAL_INTF_ADDR      0x0006 // R/W  - Adrres I2C: b[6:2] - (I2C base addres, Default - 0x7C), b[1] - (S_A1 value) b[0] - (S_A0 value) 
#define REG_SERIAL_INTF_RSVD      0x0007 // R/W  - Reserved: b[7:1] (Always must be 0), b[0] (always must be 1)
#define REG_DPLL_CTRL_0           0x0008 // R/W
// Digital PLL Control
#define REG_DPLL_PRIORITY_CTRL    0x0009 // R/W - Input Priority: b0 (0=CLK0, 1=CLK1), Default: 0x00
#define REG_DPLL_STATE_CTRL       0x000A // R/W - PLL State: b[1:0] (0=Auto, 1=Freerun, 2=Normal, 3=Holdover), b6 (REFDIS1), b5 (REFDIS0), Default: 0x00

// Input Dividers
#define REG_DPLL_PRE0_MSB         0x000B // R/W - PRE0 Divider (CLK0): b[4:0] = bits [20:16]
#define REG_DPLL_PRE0_MID         0x000C // R/W - PRE0 Divider (CLK0): b[7:0] = bits [15:8]
#define REG_DPLL_PRE0_LSB         0x000D // R/W - PRE0 Divider (CLK0): b[7:0] = bits [7:0]

#define REG_DPLL_PRE1_MSB         0x000E // R/W - PRE1 Divider (CLK1): b[4:0] = bits [20:16]
#define REG_DPLL_PRE1_MID         0x000F // R/W - PRE1 Divider (CLK1): b[7:0] = bits [15:8]
#define REG_DPLL_PRE1_LSB         0x0010 // R/W - PRE1 Divider (CLK1): b[7:0] = bits [7:0]

// Feedback Dividers
#define REG_DPLL_M1_0_MSB         0x0011 // R/W - M1_0 Feedback Divider (CLK0): bits [23:16], Default: 0x07
#define REG_DPLL_M1_0_MID         0x0012 // R/W - M1_0 Feedback Divider (CLK0): bits [15:8], Default: 0x00
#define REG_DPLL_M1_0_LSB         0x0013 // R/W - M1_0 Feedback Divider (CLK0): bits [7:0], Default: 0x00

#define REG_DPLL_M1_1_MSB         0x0014 // R/W - M1_1 Feedback Divider (CLK1): bits [23:16], Default: 0x07
#define REG_DPLL_M1_1_MID         0x0015 // R/W - M1_1 Feedback Divider (CLK1): bits [15:8], Default: 0x00
#define REG_DPLL_M1_1_LSB         0x0016 // R/W - M1_1 Feedback Divider (CLK1): bits [7:0], Default: 0x00

// Loop Filter Configuration
#define REG_DPLL_BW_CTRL          0x0017 // R/W - Loop Bandwidth: b[7:4] (Locked BW), b[3:0] (Acquisition BW), Default: 0x77
#define REG_DPLL_DAMP_GAIN_CTRL   0x0018 // R/W - DPLL Damping/Gain: b[7:5] (Locked Damping), b[4:2] (Acq Damping), b[1:0] (Loop Gain), Default: 0x6D

// Holdover / Lock Control
#define REG_DPLL_HOLD_FASTLCK     0x0023 // R/W - Holdover & Fast Lock: b[7:6] (SLEW), b[4:3] (HOLD Mode), b1 (HOLDAVG), b0 (FASTLCK), Default: 0x00
#define REG_DPLL_LOCK_WIN         0x0024 // R/W - Lock Window: b[7:0] (2.5 ns steps), Default: 0x3F

// Delta-Sigma Modulator (DSM)
#define REG_DPLL_DSM_INT_MSB      0x0025 // R/W - DSM Integer: b0 = bit [8], Default: 0x00
#define REG_DPLL_DSM_INT_LSB      0x0026 // R/W - DSM Integer: bits [7:0], Default: 0x2D

#define REG_DPLL_DSM_FRAC_MSB     0x0028 // R/W - DSM Fractional: bits [20:16], Default: 0x00
#define REG_DPLL_DSM_FRAC_MID     0x0029 // R/W - DSM Fractional: bits [15:8], Default: 0x00
#define REG_DPLL_DSM_FRAC_LSB     0x002A // R/W - DSM Fractional: bits [7:0], Default: 0x00

#define REG_DPLL_DSM_ORD_GAIN     0x002F // R/W - DSM Configuration: b[7:6] (Order), b[5:4] (DCXO Gain), b[2:0] (Dither Gain), Default: 0xD0



// GPIO Control
// GPIO , all b[3:0], b[0] = GPIO0, b[1] = GPIO1, b[2] = GPIO2, b[3] = GPIO3

#define REG_GPIO_DIR_CTRL         0x0030 // R/W - GPIO Direction: b[3:0] (0=Input, 1=Output), Default: 0x00

#define REG_GPI_SEL_2             0x0031 // R/W - Input Function Select Bit[2] (must be 0)
#define REG_GPI_SEL_1             0x0032 // R/W - Input Function Select Bit[1] (must be 0)
#define REG_GPI_SEL_0             0x0033 // R/W - Input Function Select Bit[0] (0=GPI, 1=OSEL/CSEL)

#define REG_GPO_SEL_2             0x0034 // R/W - Output Function Select Bit[2] (must be 0)
#define REG_GPO_SEL_1             0x0035 // R/W - Output Function Select Bit[1] (must be 0)
#define REG_GPO_SEL_0             0x0036 // R/W - Output Function Select [0] - b0(HOLD), b1(LOS1), b2(LOS0), b3(LOL)

#define REG_GPO_OUTPUT_VAL        0x0038 // R/W - GPIO Output Value: b[3:0] = GPIO[3:0], Default: 0x00


// Output Driver Control
#define REG_OUTPUT_EN_CTRL        0x0039 // R/W - Output Enable: b[3:0] (b0=Q0, b1=Q1, b2=Q2, b3=Q3), 0=disabled, 1=Enabled, Default: 0x00
#define REG_OUTPUT_POL_CTRL       0x003A // R/W - Output Polarity: b[3:0] (b0=Q0, b1=Q1, b2=Q2, b3=Q3), 0=Normal, 1=Inverted, Default: 0x00
#define REG_OUTPUT_MODE_3_2       0x003D // R/W - Output Mode: b[7:4]=Q3, b[3:0]=Q2, 0x0=LVPECL, 0x1=LVDS, 0x2=HCSL, 0x3=LVCMOS, Default: 0x22
#define REG_OUTPUT_MODE_1_0       0x003E // R/W - Output Mode: b[7:4]=Q1, b[3:0]=Q0, 0x0=LVPECL, 0x1=LVDS, 0x2=HCSL, 0x3=LVCMOS, Default: 0x22

// Output Dividers Q0 (Integer Divider)
#define REG_DIV_INT_Q0_CTRL       0x003F // R/W - Q0 Stage-1 Divider: b[6:5] (00=/5, 01=/6, 10=/4), Default: 0x40
#define REG_DIV_INT_Q0_NS2_MSB    0x0040 // R/W - Q0 Stage-2 Divider: bits [15:8], Default: 0x00
#define REG_DIV_INT_Q0_NS2_LSB    0x0041 // R/W - Q0 Stage-2 Divider: bits [7:0], Ratio=2×value, 0=Bypass, Default: 0x02

// Output Dividers Q1 (Integer Portion)
#define REG_DIV_INT_Q1_MSB        0x0042 // R/W - Q1 Integer Divider: bits [17:16], Default: 0x02
#define REG_DIV_INT_Q1_MID        0x0043 // R/W - Q1 Integer Divider: bits [15:8], Default: 0x00
#define REG_DIV_INT_Q1_LSB        0x0044 // R/W - Q1 Integer Divider: bits [7:0], Default: 0x02
// Output Dividers Q2 (Integer Portion)
#define REG_DIV_INT_Q2_MSB        0x0045 // R/W - Q2 Integer Divider: bits [17:16], Default: 0x02
#define REG_DIV_INT_Q2_MID        0x0046 // R/W - Q2 Integer Divider: bits [15:8], Default: 0x00
#define REG_DIV_INT_Q2_LSB        0x0047 // R/W - Q2 Integer Divider: bits [7:0], Default: 0x02
// Output Dividers Q3 (Integer Portion)
#define REG_DIV_INT_Q3_MSB        0x0048 // R/W - Q3 Integer Divider: bits [17:16], Default: 0x02
#define REG_DIV_INT_Q3_MID        0x0049 // R/W - Q3 Integer Divider: bits [15:8], Default: 0x00
#define REG_DIV_INT_Q3_LSB        0x004A // R/W - Q3 Integer Divider: bits [7:0], Default: 0x02


// Output Divider Q1 (Fractional portion)
#define REG_DIV_FRAC_Q1_MSB       0x0057 // R/W Część ułamkowa Q1: b[3:0] (bity 27:24), Default: 0x00
#define REG_DIV_FRAC_Q1_MID1      0x0058 // R/W Część ułamkowa Q1 (bity 23:16), Default: 0x00
#define REG_DIV_FRAC_Q1_MID0      0x0059 // R/W Część ułamkowa Q1 (bity 15:8), Default: 0x00
#define REG_DIV_FRAC_Q1_LSB       0x005A // R/W Część ułamkowa Q1 (bity 7:0), Default: 0x00

// Output Divider Q2 (Fractional portion)
#define REG_DIV_FRAC_Q2_MSB       0x005B // R/W Część ułamkowa Q2: b[3:0] (bity 27:24), Default: 0x00
#define REG_DIV_FRAC_Q2_MID1      0x005C // R/W Część ułamkowa Q2 (bity 23:16), Default: 0x00
#define REG_DIV_FRAC_Q2_MID0      0x005D // R/W Część ułamkowa Q2 (bity 15:8), Default: 0x00
#define REG_DIV_FRAC_Q2_LSB       0x005E // R/W Część ułamkowa Q2 (bity 7:0), Default: 0x00

// Output Divider Q3 (Fractional portion)
#define REG_DIV_FRAC_Q3_MSB       0x005F // R/W Część ułamkowa Q3: b[3:0] (bity 27:24), Default: 0x00
#define REG_DIV_FRAC_Q3_MID1      0x0060 // R/W Część ułamkowa Q3 (bity 23:16), Default: 0x00
#define REG_DIV_FRAC_Q3_MID0      0x0061 // R/W Część ułamkowa Q3 (bity 15:8), Default: 0x00
#define REG_DIV_FRAC_Q3_LSB       0x0062 // R/W Część ułamkowa Q3 (bity 7:0), Default: 0x00

// PLL Source & Analog PLL Control
#define REG_OUT_CLK_SRC_SYNC      0x0063 // R/W - Output Source & Sync: b7(Output Sync), b[5:4](Q3 Source: 0=PLL, 1=CLK0, 2=CLK1, 3=XTAL), b[1:0](Q2 Source: 0=PLL, 1=CLK0, 2=CLK1, 3=XTAL), Default: 0x00
#define REG_APLL_CTRL_0           0x0068 // R/W - Analog PLL Filter Control: b[7:5](Charge Pump), b[4:3](RS Filter), b[2:1](CP Filter), b0(2nd Pole Enable), Default: 0x8B
#define REG_APLL_CTRL_1           0x0069 // R/W - Analog PLL Mode Control: b5(TDC Disable: 0=Enable, 1=Disable), b4(0=Jitter Attenuator, 1=Synthesizer), b1(Lock Counter Enable), b0(Manual Lock), Default: 0x02
#define REG_APLL_CTRL_2           0x006A // R/W - VCO Control: b[7:5](Manual VCO Select: 000=VCO0, 001=VCO1), b[4:0](Digital Lock Control), Default: 0x2B


// Power Down & Monitor Control
#define REG_PWR_DN_CTRL_0         0x006C // R/W - LOL & XTAL Control: b2(LOL Mode: 0=Lock, 1=Lock+Stable), b1(XTAL Doubler: 0=Enable, 1=Disable), Default: 0x00
#define REG_PWR_DN_CTRL_1         0x006D // R/W - Input Power Down: b1(CLK1), b0(CLK0), 0=Enable, 1=Power Down, Default: 0x00
#define REG_PWR_DN_CTRL_2         0x006F // R/W - Output Power Down: b[3:0] (Q3-Q0), 0=Enable, 1=Power Down & Hi-Z, Default: 0x00
#define REG_PWR_DN_CTRL_3         0x0070 // R/W - Block Power Down: b2(DPLL), b1(DSM), b0(APLL Calibration Reset), 0=Enable, 1=Disable, Default: 0x00

// Input Monitor (LOS0)
#define REG_IN_MON_LOS0_MSB       0x0071 // R/W - LOS0 Monitor Counter: bit [16], Default: 0x01
#define REG_IN_MON_LOS0_MID       0x0072 // R/W - LOS0 Monitor Counter: bits [15:8], Default: 0xFF
#define REG_IN_MON_LOS0_LSB       0x0073 // R/W - LOS0 Monitor Counter: bits [7:0], Default: 0xFF

// Input Monitor (LOS1)
#define REG_IN_MON_LOS1_MSB       0x0074 // R/W - LOS1 Monitor Counter: bit [16], Default: 0x01
#define REG_IN_MON_LOS1_MID       0x0075 // R/W - LOS1 Monitor Counter: bits [15:8], Default: 0xFF
#define REG_IN_MON_LOS1_LSB       0x0076 // R/W - LOS1 Monitor Counter: bits [7:0], Default: 0xFF


// Interrupt Control
#define REG_INT_EN_CTRL           0x0079 // R/W - Interrupt Enable: b6(LOL), b4(HOLD), b1(LOS1), b0(LOS0), Default: 0x00


// Factory Settings
#define REG_FACTORY_SETTING_0     0x007A // R/W - Factory Setting 0: Always write 0x27
#define REG_FACTORY_SETTING_1     0x007B // R/W - Factory Setting 1: Program per Table 7O, Default: 0x14


// Status Registers
#define REG_INT_STATUS_0          0x0200 // R/W1C - Interrupt Status: b6(LOL), b4(HOLD), b1(LOS1), b0(LOS0), write '1' to clear
#define REG_INT_STATUS_1          0x0201 // R/W - Reserved, always write 0x00
#define REG_GPI_VAL_STATUS        0x020C // R - GPIO Input Status: b[3:0] = GPIO[3:0] logic level
#define REG_GLOBAL_INT_STATUS     0x020D // R - Global Interrupt Status: b0(0=No Active Interrupt, 1=Interrupt Active)
#define REG_BOOT_STATUS_0         0x0210 // R - Boot Status: b1(EEP_ERR - EEPROM CRC Error), b0(BOOTFAIL - EEPROM/I2C Read Failure)
#define REG_BOOT_STATUS_1         0x0211 // R - EEPROM Status: b0(EEPDONE - EEPROM Boot Complete)


// podstawowe operacje
void i2c_device_init(i2c_inst_t *i2c, uint sda, uint scl);
bool i2c_write_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len);
bool i2c_read_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len);
void dump_all_regs(i2c_inst_t *i2c);
bool pll_enable_q1_led(i2c_inst_t *i2c);
void Flag_Clear(i2c_inst_t *i2c);
bool pll_load_tcs(i2c_inst_t *i2c);


#endif