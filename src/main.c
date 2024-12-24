#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "setupWifi.h"
#include "lwip/altcp.h"

#include "request.h"

#define BUF_SIZE 4096
char myBuff1[BUF_SIZE];

int readTemp()
{
	return 33;
}

int main()
{
	stdio_init_all();
	connect(WIFI_SSID, WIFI_PASSWORD);

	ip_addr_t ip;
	IP4_ADDR(&ip, 192, 168, 1, 204);

	char pingHeader[] = "GET /ping HTTP/1.1\r\nHOST:192.168.1.204\r\nConnection: close\r\n\r\n";
	struct connectionState *cs = doRequest(&ip, "192.168.1.204", 5010, pingHeader, "", myBuff1);

	while (pollRequest(&cs))
	{
		sleep_ms(200);
	}
	printf("Buffer=%s\n", myBuff1);

	while (true)
	{
		char openHeader[] = "PUT /sensor HTTP/1.1\r\nHOST:192.168.1.204\r\nConnection: open\r\n\r\n";
		cs = doRequest(&ip, "192.168.1.204", 5010, openHeader, "", myBuff1);
		while (pollRequest(&cs))
		{
			// sleep_ms(200);
		}
		printf("Buffer=%s\n", myBuff1);
	}

	// char wsHeader[] = "GET /ws HTTP/1.1\r\n"
	// 				  "Host: 192.168.1.204\r\n"
	// 				  "Upgrade: websocket\r\n"
	// 				  "Connection: Upgrade\r\n"
	// 				  "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
	// 				  "Sec-WebSocket-Version: 13\r\n\r\n";
	// cs = doRequest(&ip, "192.168.1.204", 5010, wsHeader, "", myBuff1);

	// while (pollRequest(&cs))
	// {
	// 	if (cs->state == UPGRADED)
	// 	{
	// 		printf("Upgraded\n");
	// 		break;
	// 	}
	// 	sleep_ms(200);
	// }
	// printf("Buffer=%s\n", myBuff1);

	// while (true)
	// {
	// 	printf("In state: %d\n", cs->state);
	// 	sleep_ms(200);
	// }

	// while (true)
	// {
	// 	int t = readTemp();
	// 	int len = snprintf(NULL, 0, "%d", t);
	// 	char *requestData = malloc(len + 1);
	// 	snprintf(requestData, len + 1, "%d", t);
	// 	printf("%s\n", requestData);
	// 	struct connectionState *cs1 = doRequest(&ip, "192.168.1.204", 5010, "PUT", "/store", requestData, myBuff1);
	// 	while (pollRequest(&cs1))
	// 	{
	// 		sleep_ms(200);
	// 	}
	// 	printf("%s\n", myBuff1);
	// 	sleep_ms(5000);
	// }

	cyw43_arch_deinit();
	return 0;
}