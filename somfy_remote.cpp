#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "radio.h"
#include "remote.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi_default
#define PIN_MISO 4
#define PIN_SCK  6
#define PIN_MOSI 7

// Status LED
#define PIN_LED 13

#define PIN_CS_RADIO   5
// Hardware reset
#define PIN_RESET_RADIO 15




int main()
{
    stdio_init_all();

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);

    // SPI initialisation. Set SPI speed to 0.5MHz. Absolute max supported is 10Mhz
    spi_init(SPI_PORT, 500*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    /*(if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }*/

    RFM69Radio radio(SPI_PORT, PIN_CS_RADIO, PIN_RESET_RADIO);


    sleep_ms(6000);
    puts("Starting...\n");

    radio.Reset();
    puts("Reset complete!\n");
    radio.Initialize();

    radio.SetSymbolWidth(635);
    radio.SetFrequency(433.44);

    auto fq = radio.GetFrequency();
    printf("Radio frequency: %.3f\n", fq);
    auto br = radio.GetBitRate();
    printf("BitRate: %dbps\n", br);
  
    auto ver = radio.GetVersion();
    printf("Radio version: 0x%02x\n", ver);

    SomfyRemote remote(&radio, 0x27962a, 2612);


    uint16_t loopCounter = 0;
    while (true) {
        loopCounter++;
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        gpio_put(PIN_LED, 0);
        sleep_ms(250);
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        gpio_put(PIN_LED, 1);
        sleep_ms(250);
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);

        if((loopCounter & 0x1F) == 0x0F)
        {
            remote.PressButtons(SomfyButton::Up, 4);
            puts("Going Up...!\n");
        }
        if((loopCounter & 0x1F) == 0x1F)
        {
            remote.PressButtons(SomfyButton::Down, 4);
            puts("Going Down...!\n");

            return 0;
        }

    }
    puts("Hello, world!");

    return 0;
}
