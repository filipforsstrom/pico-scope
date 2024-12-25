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
	uint32_t sequence_number; // 4 bytes
	uint64_t timestamp;		  // 8 bytes
	char data[32];			  // Fixed-length for simplicity
};

char json_payload[128];

uint32_t sequence_number = 0;
struct Payload payload;

bool send_udp_packet_callback(__unused struct repeating_timer *t)
{
	struct udp_pcb *pcb = *(struct udp_pcb **)t->user_data;

	adc_select_input(0);
	uint16_t result = adc_read();

	// Update payload
	payload.sequence_number = sequence_number++;
	payload.timestamp = time_us_64();
	snprintf(payload.data, sizeof(payload.data), "%d", result);

	// Format payload as JSON
	int json_length = snprintf(json_payload, sizeof(json_payload), "{\"sequence_number\":%u,\"timestamp\":%llu,\"data\":\"%s\"}",
							   payload.sequence_number, payload.timestamp, payload.data);

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

	IP4_ADDR(&ip, 192, 168, 15, 104);

	udp_connect(pcb, &ip, port);

	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);

	struct repeating_timer timer;
	add_repeating_timer_ms(1, send_udp_packet_callback, &pcb, &timer); // Set to 1 ms for high frequency

	while (true)
	{
		// No wait here, sending packets as fast as possible
	}
}