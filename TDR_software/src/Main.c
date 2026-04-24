#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define LED_PIN 25

int main()
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    char buffer[64];

    while (true) {

        // odbieranie danych z USB
        if (fgets(buffer, sizeof(buffer), stdin)) {

            if (strncmp(buffer, "LED_ON", 6) == 0) {
                gpio_put(LED_PIN, 1);
                printf("OK\n");
            }
            else if (strncmp(buffer, "LED_OFF", 7) == 0) {
                gpio_put(LED_PIN, 0);
                printf("OK\n");
            }
            else {
                printf("UNKNOWN\n");
            }
        }
    }
}