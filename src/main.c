#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "setupWifi.h"
#include "hardware/adc.h"

ip_addr_t ip;
uint port = 11000;

char message[32];

bool send_udp_packet_callback(__unused struct repeating_timer *t)
{
	struct udp_pcb *pcb = *(struct udp_pcb **)t->user_data;

	adc_select_input(0);
	uint16_t result = adc_read();

	snprintf(message, sizeof(message), "%d", result);

	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(message) + 1, PBUF_RAM);

	snprintf(p->payload, strlen(message) + 1, "%s", message);

	err_t er = udp_sendto(pcb, p, &ip, port);
	pbuf_free(p);

	return true;
}

int main()
{
	stdio_init_all();
	connect(WIFI_SSID, WIFI_PASSWORD);

	struct udp_pcb *pcb = udp_new();
	udp_bind(pcb, IP_ADDR_ANY, 8080);

	IP4_ADDR(&ip, 192, 168, 15, 104);

	udp_connect(pcb, &ip, port);

	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);

	struct repeating_timer timer;
	add_repeating_timer_ms(3, send_udp_packet_callback, &pcb, &timer);

	while (true)
	{
	}
}
