#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
char myBuff[1000];

#define DEBUG_printf printf

const ip_addr_t SERVER_IP = IPADDR4_INIT_BYTES(192, 168, 15, 104);
#define SERVER_PORT 5010

int setup_wifi()
{
	if (cyw43_arch_init_with_country(CYW43_COUNTRY_SWEDEN))
	{
		DEBUG_printf("Failed to initialise CYW43\n");
		return 1;
	}

	cyw43_arch_enable_sta_mode();

	while (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK))
	{
		DEBUG_printf("Failed to connect to %s, retrying...\n", WIFI_SSID);
		sleep_ms(1000);
	}
	DEBUG_printf("Connected to %s\n", WIFI_SSID);
	DEBUG_printf("IP: %s\n",
				 ip4addr_ntoa(netif_ip_addr4(netif_default)));
	DEBUG_printf("Mask: %s\n",
				 ip4addr_ntoa(netif_ip_netmask4(netif_default)));
	DEBUG_printf("Gateway: %s\n",
				 ip4addr_ntoa(netif_ip_gw4(netif_default)));
	DEBUG_printf("Host Name: %s\n",
				 netif_get_hostname(netif_default));
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

	// Ensure we don't overflow myBuff
	size_t len = p->tot_len < sizeof(myBuff) - 1 ? p->tot_len : sizeof(myBuff) - 1;
	pbuf_copy_partial(p, myBuff, len, 0);
	myBuff[len] = '\0'; // Null-terminate the buffer

	printf("%s\n", myBuff);
	return ERR_OK;
}

int main()
{
	stdio_init_all();

	setup_wifi();

	httpc_connection_t settings;
	settings.result_fn = result;
	settings.headers_done_fn = headers;

	err_t err = httpc_get_file(
		&SERVER_IP,
		SERVER_PORT,
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