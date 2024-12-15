#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
char myBuff[1000];

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

void result(void *arg, httpc_result_t httpc_result,
			u32_t rx_content_len, u32_t srv_res, err_t err)

{
	printf("transfer complete\n");
	printf("local result=%d\n", httpc_result);
	printf("http result=%d\n", srv_res);
}

err_t headers(httpc_state_t *connection, void *arg,
			  struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
	printf("headers recieved\n");
	printf("content length=%d\n", content_len);
	printf("header length %d\n", hdr_len);
	pbuf_copy_partial(hdr, myBuff, hdr->tot_len, 0);
	printf("headers \n");
	printf("%s", myBuff);
	return ERR_OK;
}

err_t body(void *arg, struct altcp_pcb *conn,
		   struct pbuf *p, err_t err)
{
	printf("body\n");
	pbuf_copy_partial(p, myBuff, p->tot_len, 0);
	printf("%s", myBuff);
	return ERR_OK;
}

int main()
{
	stdio_init_all();

	setup_wifi();

	ip_addr_t ip;
	IP4_ADDR(&ip, 192, 168, 1, 204);
	uint16_t port = 5010;
	httpc_connection_t settings;
	settings.result_fn = result;
	settings.headers_done_fn = headers;

	err_t err = httpc_get_file(
		&ip,
		port,
		"/ping",
		&settings,
		body,
		NULL,
		NULL);

	printf("status %d \n", err);

	while (true)
	{
		sleep_ms(10);
	}

	cyw43_arch_deinit();
	return 0;
}