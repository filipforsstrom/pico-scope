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

	struct connectionState *cs = doRequest(&ip, "192.168.1.204", 5010, "GET", "/ping", NULL, myBuff1);

	while (pollRequest(&cs))
	{
		sleep_ms(200);
	}
	printf("Buffer=%s\n", myBuff1);


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