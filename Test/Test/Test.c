#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdarg.h>
#include <string.h>

// Forward declaration so functions above can call it
void popup_printf(const char *fmt, ...);


// =========================
// KONFIGURACJA
// =========================

#define I2C_PORT i2c0

#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

#define I2C_BAUDRATE 100000

// Spróbuj najpierw 0x3E.
// Jeśli brak odpowiedzi -> zmień na 0x7C.
#define DEVICE_ADDR 0x3E

#define REG_DEVICE_ID_HIGH 0x0002
#define REG_DEVICE_ID_LOW  0x0003


// =========================
// SPRAWDZENIE ACK
// =========================

static bool i2c_device_exists(uint8_t addr)
{
    int ret = i2c_write_timeout_us(
        I2C_PORT,
        addr,
        NULL,
        0,
        false,
        1000
    );

    return ret >= 0;
}


// =========================
// ODCZYT 1 BAJTU
// Z REJESTRU 16-BIT
// =========================

static bool i2c_read_register(
    uint8_t dev_addr,
    uint16_t reg_addr,
    uint8_t *data
)
{
    uint8_t reg_buf[2];

    reg_buf[0] = (reg_addr >> 8) & 0xFF;
    reg_buf[1] = reg_addr & 0xFF;

    // Wysłanie adresu rejestru
    int ret = i2c_write_timeout_us(
        I2C_PORT,
        dev_addr,
        reg_buf,
        2,
        true,
        1000
    );

    if(ret < 0)
    {
        return false;
    }

    // Odczyt danych
    ret = i2c_read_timeout_us(
        I2C_PORT,
        dev_addr,
        data,
        1,
        false,
        1000
    );

    return ret == 1;
}


// =========================
// SKANOWANIE MAGISTRALI
// =========================

void scan_i2c(void)
{
    popup_printf("\n");
    popup_printf("Skanowanie I2C...\n");

    int devices_found = 0;

    for(uint8_t addr = 0x03; addr <= 0x77; addr++)
    {
        if(i2c_device_exists(addr))
        {
            popup_printf(
                "Znaleziono urzadzenie: 0x%02X\n",
                addr
            );

            devices_found++;
        }
    }

    if(devices_found == 0)
    {
        popup_printf("Brak urzadzen I2C\n");
    }

    popup_printf("Koniec skanowania\n\n");
}


// =========================
// TEST 8T49N241
// =========================

void test_8t49n241(void)
{
    popup_printf("Test 8T49N241\n");

    // Czas na autokonfigurację układu
    sleep_ms(200);

    popup_printf(
        "Sprawdzanie adresu 0x%02X...\n",
        DEVICE_ADDR
    );

    if(!i2c_device_exists(DEVICE_ADDR))
    {
        popup_printf("Brak ACK\n");
        return;
    }

    popup_printf("ACK odebrany\n");

    uint8_t id_high = 0;
    uint8_t id_low = 0;

    // Odczyt HIGH BYTE
    if(!i2c_read_register(
        DEVICE_ADDR,
        REG_DEVICE_ID_HIGH,
        &id_high
    ))
    {
        popup_printf(
            "Blad odczytu rejestru 0x0002\n"
        );

        return;
    }

    // Odczyt LOW BYTE
    if(!i2c_read_register(
        DEVICE_ADDR,
        REG_DEVICE_ID_LOW,
        &id_low
    ))
    {
        popup_printf(
            "Blad odczytu rejestru 0x0003\n"
        );

        return;
    }

    uint16_t device_id =
        ((uint16_t)id_high << 8) | id_low;

    popup_printf(
        "Device ID = 0x%04X\n",
        device_id
    );

    if(device_id == 0x0606)
    {
        popup_printf(
            "8T49N241 dziala poprawnie\n"
        );
    }
    else
    {
        popup_printf(
            "Niepoprawny Device ID\n"
        );
    }
}


// =========================
// MAIN
// =========================

int main()
{
    stdio_init_all();

    sleep_ms(2000);

    popup_printf("\n");
    popup_printf("Start programu\n");


    // =========================
    // INICJALIZACJA I2C
    // =========================

    i2c_init(
        I2C_PORT,
        I2C_BAUDRATE
    );

    gpio_set_function(
        I2C_SDA_PIN,
        GPIO_FUNC_I2C
    );

    gpio_set_function(
        I2C_SCL_PIN,
        GPIO_FUNC_I2C
    );

    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    popup_printf("I2C zainicjalizowane\n");


    // =========================
    // SKANOWANIE
    // =========================

    scan_i2c();


    // =========================
    // TEST UKLADU
    // =========================

    test_8t49n241();


    // =========================
    // PETLA GLOWNA
    // =========================

    while(true)
    {
        sleep_ms(1000);
    }
}


// =========================
// POPUP / printf wrapper
// Sends normal output and a marker line prefixed with [POPUP]
// so a host-side tool can show a Windows popup for those lines.
// =========================

void popup_printf(const char *fmt, ...)
{
    char buf[512];
    va_list ap;

    // Print normally to stdio
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    // Prepare the same message and send a marker line for the host
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // Ensure marker is on its own line
    // Remove trailing newlines from buf for consistent formatting
    size_t len = strlen(buf);
    while(len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
    {
        buf[len-1] = '\0';
        len--;
    }

    printf("[POPUP]%s\n", buf);
}