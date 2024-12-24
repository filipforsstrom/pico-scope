#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "setupWifi.h"

int main()
{
	stdio_init_all();
	connect(WIFI_SSID, WIFI_PASSWORD);

	srand(time(NULL));

	struct udp_pcb *pcb = udp_new();
	udp_bind(pcb, IP_ADDR_ANY, 8080);

	ip_addr_t ip;
	IP4_ADDR(&ip, 192, 168, 15, 104);

	udp_connect(pcb, &ip, 11000);

	char message[32];
	while (true)
	{
		float random_value = (float)rand() / (float)(RAND_MAX);
		snprintf(message, sizeof(message), "%f", random_value);

		struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(message) + 1, PBUF_RAM);

		snprintf(p->payload, strlen(message) + 1, "%s", message);

		err_t er = udp_sendto(pcb, p, &ip, 11000);
		pbuf_free(p);

		sleep_ms(10);
	}
}
