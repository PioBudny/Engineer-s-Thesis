#ifndef I2C_DEVICE_H
#define I2C_DEVICE_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdbool.h>

// Adres urządzenia (S_A0=0, S_A1=0)
#define DEVICE_ADDR 0x7C

void i2c_device_init(i2c_inst_t *i2c, uint sda, uint scl);

// podstawowe operacje
bool i2c_write_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len);
bool i2c_read_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len);
bool device_set_frequency(i2c_inst_t *i2c, uint8_t channel, uint32_t fout_hz);
bool device_pulse_output(i2c_inst_t *i2c, uint8_t channel);
bool device_step_output(i2c_inst_t *i2c, uint8_t channel);
bool device_disable_output(i2c_inst_t *i2c, uint8_t channel);

// helper
bool device_read_id(i2c_inst_t *i2c, uint16_t *id);

#endif