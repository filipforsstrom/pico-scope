#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define DEBUG_printf printf

#define HOST "192.168.1.204:5010"
#define URL_REQUEST "/ping"

int setup_wifi()
{
	if (cyw43_arch_init_with_country(CYW43_COUNTRY_SWEDEN))
	{
		DEBUG_printf("Failed to initialise CYW43\n");
		return 1;
	}

	cyw43_arch_enable_sta_mode();

	if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK))
	{
		DEBUG_printf("Failed to connect to %s\n", WIFI_SSID);
		return 2;
	}
	DEBUG_printf("Connected to %s\n", WIFI_SSID);
}

int main()
{
	stdio_init_all();

	setup_wifi();
	while (true)
	{
		sleep_ms(10);
	}

	cyw43_arch_deinit();
	return 0;
}