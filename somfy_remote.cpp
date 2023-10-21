#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "radio.h"
#include "remote.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"

#include "dhcpserver.h"
#include "dnsserver.h"

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


typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    async_context_t *context;
} TCP_SERVER_T;

// This "worker" function is called to safely perform work when instructed by key_pressed_func
void key_pressed_worker_func(async_context_t *context, async_when_pending_worker_t *worker) {
    assert(worker->user_data);
    printf("Disabling wifi\n");
    cyw43_arch_disable_ap_mode();
    ((TCP_SERVER_T*)(worker->user_data))->complete = true;
}

static async_when_pending_worker_t key_pressed_worker = {
        .next = NULL,
        .do_work = key_pressed_worker_func
};

const char blind1[] = "{ \"name\": \"Office\" },\n";
const char blind2[] = "{ \"name\": \"Drawing Rom\" }\n";

void key_pressed_func(void *param) {
    assert(param);
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    printf("%c", key);
    if (key == 'd' || key == 'D') {
        // We are probably in irq context so call wifi in a "worker"
        async_context_set_work_pending(((TCP_SERVER_T*)param)->context, &key_pressed_worker);
    }
}



#define HEARTBEAT_PERIOD_MS 1000

static void heartbeat_handler(async_context_t *context, async_at_time_worker_t *worker);

static async_at_time_worker_t heartbeat_worker = {
    .next = NULL,
    .do_work = heartbeat_handler
};

static void heartbeat_handler(async_context_t *context, async_at_time_worker_t *worker) {

    // Invert the led
    static int led_on = true;
    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);

    // Restart timer
    async_context_add_at_time_worker_in_ms(context, &heartbeat_worker, HEARTBEAT_PERIOD_MS);
}


 u16_t ssi_handler (int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
 {
    if(tagIndex == 0)
    {
        if(tagPart == 0)
        {
            memcpy(pcInsert, blind1, sizeof(blind1)-1);
            *nextPart = 1;
            return sizeof(blind1)-1;
        }
        if(tagPart == 1)
        {
            memcpy(pcInsert, blind2, sizeof(blind2)-1);
            return sizeof(blind2)-1;
        }

    }
    memcpy(pcInsert, "This did something", 18);
    return 18;
 }

const uint32_t wifiCredsMagic = 0x19841977;
const uint32_t wifiCredsOffset = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

struct WifiCredentials
{
    uint32_t magicNumber;
    char ssid[64];
    char password[128];
};

int main()
{
    stdio_init_all();

    TCP_SERVER_T *state = (TCP_SERVER_T *)calloc(1, sizeof(TCP_SERVER_T));

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

    auto creds = (const WifiCredentials *)(XIP_BASE + wifiCredsOffset);
    if(creds->magicNumber != wifiCredsMagic)
    {
        printf("Flash not initialized - formatting\n");
        if(flash_safe_execute( [](void *param) {
            flash_range_erase(wifiCredsOffset, FLASH_SECTOR_SIZE);
            auto credsSize = (sizeof(WifiCredentials) / FLASH_PAGE_SIZE + 1) * FLASH_PAGE_SIZE;
            auto blankCreds = (WifiCredentials *)calloc(1, credsSize);
            blankCreds->magicNumber = wifiCredsMagic;
            flash_range_program(wifiCredsOffset, (uint8_t *)blankCreds, credsSize);
        }, NULL, 1000) != PICO_OK)
        {
            printf("Unable to format flash");
            return -1;
        }

        if(creds->magicNumber != wifiCredsMagic)
        {
            puts("Something went wrong with the credentials format\n");
            return -1;
        }
    }


    // Get notified if the user presses a key
    state->context = cyw43_arch_async_context();
    key_pressed_worker.user_data = state;
    async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
    stdio_set_chars_available_callback(key_pressed_func, state);


    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    dhcp_server_t dhcp_server;
    dns_server_t dns_server;


    if(strlen(creds->ssid)!=0)
    {
        puts("Connecting to WiFi");
        cyw43_arch_enable_sta_mode();
        cyw43_arch_wifi_connect_async(creds->ssid, creds->password, CYW43_AUTH_WPA2_AES_PSK);
    }
    else
    {
        const char *ap_name = "picow_test";
        const char *password = "password";

        puts("Enabling ap mode");
        cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

        // Start the dhcp server
        dhcp_server_init(&dhcp_server, &state->gw, &mask);

        // Start the dns server
        dns_server_init(&dns_server, &state->gw);

    }


    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &heartbeat_worker, HEARTBEAT_PERIOD_MS);
    
    httpd_init();

    const char *tags[] = {
        "blinds"
    };
    http_set_ssi_handler(ssi_handler, tags, LWIP_ARRAYSIZE(tags));

    auto onOff = false;

    state->complete = false;
    while(!state->complete) {

        // if you are not using pico_cyw43_arch_poll, then Wi-FI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);

        gpio_put(PIN_LED, onOff);
        onOff = !onOff;
    }

    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    return 0;

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
