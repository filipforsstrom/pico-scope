#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "setupWifi.h"
#include "lwip/altcp.h"
#include "hardware/adc.h"

#include "http_request.h"
#include "ws_request.h"
#include "memory.h"
#include "debug.h"

#define BUF_SIZE 4096
char myBuff1[BUF_SIZE];

#define SPEED 10

struct Payload
{
	uint64_t timestamp;
	uint16_t adc[3];
};

char json_payload[128];

struct Payload payload;

bool send_udp_packet_callback(__unused struct repeating_timer *t)
{
	struct http_connectionState *cs = *(struct http_connectionState **)t->user_data;

	return true;
}

void updatePayload(struct ws_connectionState *cs)
{
	adc_select_input(0);
	payload.adc[0] = adc_read();
	adc_select_input(1);
	payload.adc[1] = adc_read();
	adc_select_input(2);
	payload.adc[2] = adc_read();

	payload.timestamp = time_us_64();

	int json_length = snprintf(json_payload, sizeof(json_payload), "{\"timestamp\":%llu,\"adc\":[%u,%u,%u]}",
							   payload.timestamp, payload.adc[0], payload.adc[1], payload.adc[2]);

	cs->sendData = json_payload;
}

bool send_ws_callback(__unused struct repeating_timer *t)
{
	struct ws_connectionState *cs = *(struct ws_connectionState **)t->user_data;
	updatePayload(cs);
	cs->state = WS_SENDING;
	return true;
}

int main()
{
	stdio_init_all();

	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);

	struct repeating_timer timer;

	connect(WIFI_SSID, WIFI_PASSWORD);
	ip_addr_t ip;
	uint16_t port = 5010;
	IP4_ADDR(&ip, 192, 168, 1, 204);
	char hostname[] = "192.168.1.204";

	char pingHeader[] = "GET /ping HTTP/1.1\r\nHOST:192.168.1.204\r\nConnection: close\r\n\r\n";
	struct http_connectionState *http_cs = http_doRequest(&ip, hostname, port, pingHeader, "", myBuff1);

	while (http_pollRequest(&http_cs))
	{
		sleep_ms(200);
	}
	printf("Buffer=%s\n", myBuff1);

	struct ws_connectionState *ws_cs = ws_doWsHandshakeRequest(&ip, hostname, port, myBuff1);

	while (ws_pollRequest(&ws_cs))
	{
		sleep_ms(200);
	}
	printf("Buffer=%s\n", myBuff1);

	ws_cs = ws_dataTransfer(&ws_cs);
	add_repeating_timer_ms(SPEED, send_ws_callback, &ws_cs, &timer);

	while (ws_pollRequest(&ws_cs))
	{
		// updatePayload(ws_cs);
		// sleep_ms(200);
	}

	cyw43_arch_deinit();
	return 0;
}
