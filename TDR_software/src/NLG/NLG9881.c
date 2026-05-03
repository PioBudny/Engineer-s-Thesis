#include "NLG9881.h"
#include "pico/stdlib.h"
#include <stdlib.h>

// Uruchomienie I2C i konfiguracja pinów
void i2c_device_init(i2c_inst_t *i2c, uint sda, uint scl) {
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    sleep_ms(100);
}

// odczyt ID urządzenia
bool device_read_id(i2c_inst_t *i2c, uint16_t *id) {
    uint8_t data[2];

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, 0x0002, data, 2)) {
        return false;
    }

    *id = (data[0] << 8) | data[1];
    return true;
}

// zapis do rejestru 16-bit
bool i2c_write_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len) {
    uint8_t buffer[258];

    buffer[0] = (reg >> 8) & 0xFF;
    buffer[1] = reg & 0xFF;

    for (size_t i = 0; i < len; i++) {
        buffer[2 + i] = data[i];
    }

    int ret = i2c_write_blocking(i2c, addr, buffer, len + 2, false);
    return (ret >= 0);
}

// odczyt z rejestru 16-bit
bool i2c_read_reg16(i2c_inst_t *i2c, uint8_t addr, uint16_t reg, uint8_t *data, size_t len) {
    uint8_t reg_buf[2];

    reg_buf[0] = (reg >> 8) & 0xFF;
    reg_buf[1] = reg & 0xFF;

    // ustaw adres rejestru
    if (i2c_write_blocking(i2c, addr, reg_buf, 2, true) < 0) {
        return false;
    }

    // czytaj dane
    if (i2c_read_blocking(i2c, addr, data, len, false) < 0) {
        return false;
    }

    return true;
}

bool device_set_output_cmos(i2c_inst_t *i2c, uint8_t channel) {
    if (channel > 3) return false;

    uint16_t reg_addr;
    uint8_t reg;
    uint8_t shift;

    if (channel == 0) {
        reg_addr = 0x003E;
        shift = 0;
    } else if (channel == 1) {
        reg_addr = 0x003E;
        shift = 4;
    } else if (channel == 2) {
        reg_addr = 0x003D;
        shift = 0;
    } else {
        reg_addr = 0x003D;
        shift = 4;
    }

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, reg_addr, &reg, 1)) {
        return false;
    }

    reg &= ~(0b111 << shift);
    reg |= (0b011 << shift);

    return i2c_write_reg16(i2c, DEVICE_ADDR, reg_addr, &reg, 1);
}

bool device_step_output(i2c_inst_t *i2c, uint8_t channel) {
    if (channel > 3) return false;

    uint8_t reg;

    // odczytaj aktualny OUTEN
    if (!i2c_read_reg16(i2c, DEVICE_ADDR, 0x0039, &reg, 1)) {
        return false;
    }

    // ustaw bit kanału
    reg |= (1 << channel);

    // zapisz
    return i2c_write_reg16(i2c, DEVICE_ADDR, 0x0039, &reg, 1);
}

bool device_pulse_output(i2c_inst_t *i2c, uint8_t channel) {
    if (channel > 3) return false;

    uint8_t reg;

    // odczytaj
    if (!i2c_read_reg16(i2c, DEVICE_ADDR, 0x0039, &reg, 1)) {
        return false;
    }

    // włącz
    uint8_t reg_on = reg | (1 << channel);
    if (!i2c_write_reg16(i2c, DEVICE_ADDR, 0x0039, &reg_on, 1)) {
        return false;
    }

    // natychmiast wyłącz
    uint8_t reg_off = reg & ~(1 << channel);
    if (!i2c_write_reg16(i2c, DEVICE_ADDR, 0x0039, &reg_off, 1)) {
        return false;
    }

    return true;
}

bool device_set_frequency(i2c_inst_t *i2c, uint8_t channel, uint32_t fout_hz) {
    if (channel > 3 || fout_hz == 0) return false;

    // STAŁE PLL (przykład)
    const uint32_t fvco = 2500000000ULL; // 2.5 GHz

    // oblicz divider
    uint32_t odiv = fvco / fout_hz;

    // ograniczenia (typowe dla takich układów)
    if (odiv < 1 || odiv > 0xFFFF) return false;

    uint16_t reg_addr;

    // mapowanie kanałów (sprawdź datasheet!)
    switch (channel) {
        case 0: reg_addr = 0x0050; break;
        case 1: reg_addr = 0x0052; break;
        case 2: reg_addr = 0x0054; break;
        case 3: reg_addr = 0x0056; break;
        default: return false;
    }

    uint8_t data[2];
    data[0] = (odiv >> 8) & 0xFF;
    data[1] = odiv & 0xFF;

    if (!i2c_write_reg16(i2c, DEVICE_ADDR, reg_addr, data, 2)) {
        return false;
    }

    // APPLY / UPDATE (bardzo ważne)
    uint8_t update = 0x01;
    if (!i2c_write_reg16(i2c, DEVICE_ADDR, 0x000F, &update, 1)) {
        return false;
    }

    return true;
}


bool device_config_gpio_as_enable(i2c_inst_t *i2c) {
    uint8_t val;

    val = 0x01;
    return i2c_write_reg16(i2c, DEVICE_ADDR, 0x0208, &val, 1);
}

bool device_disable_output(i2c_inst_t *i2c, uint8_t channel) {
    if (channel > 3) return false;

    uint8_t reg;

    if (!i2c_read_reg16(i2c, DEVICE_ADDR, 0x0039, &reg, 1)) {
        return false;
    }

    reg &= ~(1 << channel);

    return i2c_write_reg16(i2c, DEVICE_ADDR, 0x0039, &reg, 1);
}