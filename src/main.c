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

struct Payload
{
	uint32_t sequence_number;
	uint64_t timestamp;
	uint16_t adc[3];
};

char json_payload[128];

uint32_t sequence_number = 0;
struct Payload payload;

bool send_udp_packet_callback(__unused struct repeating_timer *t)
{
	struct udp_pcb *pcb = *(struct udp_pcb **)t->user_data;

	adc_select_input(0);
	payload.adc[0] = adc_read();
	adc_select_input(1);
	payload.adc[1] = adc_read();
	adc_select_input(2);
	payload.adc[2] = adc_read();

	// Update payload
	payload.sequence_number = sequence_number++;
	payload.timestamp = time_us_64();

	// Format payload as JSON
	int json_length = snprintf(json_payload, sizeof(json_payload), "{\"sequence_number\":%u,\"timestamp\":%llu,\"adc\":[%u,%u,%u]}",
							   payload.sequence_number, payload.timestamp, payload.adc[0], payload.adc[1], payload.adc[2]);

	// Allocate pbuf and copy JSON payload
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, json_length, PBUF_RAM);

	memcpy(p->payload, json_payload, json_length);

	// Send UDP packet
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

	IP4_ADDR(&ip, 192, 168, 1, 204);

	udp_connect(pcb, &ip, port);

	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);

	struct repeating_timer timer;
	add_repeating_timer_ms(10, send_udp_packet_callback, &pcb, &timer);

	while (true)
	{
	}
}