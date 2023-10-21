#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "radio.h"
#include "remote.h"
#include "webInterface.h"
#include "wifiScanner.h"
#include "blockStorage.h"
#include "deviceConfig.h"

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

// This "worker" function is called to safely perform work when instructed by key_pressed_func
void key_pressed_worker_func(async_context_t *context, async_when_pending_worker_t *worker) {
    assert(worker->user_data);
    printf("Terminating\n");
    *(bool *)(worker->user_data) = true;
}

static async_when_pending_worker_t key_pressed_worker = {
        .next = NULL,
        .do_work = key_pressed_worker_func
};

void key_pressed_func(void *param) {
    assert(param);
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    printf("%c", key);
    if (key == 'd' || key == 'D') {
        // We are probably in irq context so call wifi in a "worker"
        async_context_set_work_pending((async_context_t *)param, &key_pressed_worker);
    }
}



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

    sleep_ms(5000);
    
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    if(!flash_safe_execute_core_init())
    {
        printf("Flash init failed");
        return -1;
    }

    // Store flash settings at the very top of Flash memory
    // ALL STORED DATA WILL BE LOST IF THESE ARE CHANGED
    const uint32_t StorageSize = 8 * FLASH_SECTOR_SIZE;
    BlockStorage storage(PICO_FLASH_SIZE_BYTES - StorageSize, StorageSize, 252);


    DeviceConfig config(storage);
    if(config.GetWifiConfig() == nullptr)
    {
        puts("WiFi Config not found. Resetting all settings!\n");
        config.HardReset();
    }

    puts("Checking Device config...\n");

    if(config.GetWifiConfig() == nullptr ||
        config.GetMqttConfig() == nullptr)
    {
        puts("Settings could not be read back. Error!\n");
        return -1;
    }


    puts("Starting the web interface...\n");

    // Get notified if the user presses a key
    auto context = cyw43_arch_async_context();
    auto quit = false;
    key_pressed_worker.user_data = &quit;
    async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
    stdio_set_chars_available_callback(key_pressed_func, context);

    auto webInterface = WebInterface::GetInstance(config);
    webInterface->Start();

    auto onOff = false;

    while(!quit) {

        // if you are not using pico_cyw43_arch_poll, then Wi-FI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);

        gpio_put(PIN_LED, onOff);
        onOff = !onOff;
    }

    puts("Exiting now\n");

    // dns_server_deinit(&dns_server);
    // dhcp_server_deinit(&dhcp_server);
    // cyw43_arch_deinit();
    // return 0;

/*    RFM69Radio radio(SPI_PORT, PIN_CS_RADIO, PIN_RESET_RADIO);


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

    return 0;*/
}
