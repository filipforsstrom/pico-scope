#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "lwip/tcp.h"
char myBuff[1000];

#define DEBUG_printf printf

const ip_addr_t SERVER_IP = IPADDR4_INIT_BYTES(192, 168, 15, 104);
#define SERVER_PORT 5010

struct tcp_pcb *tcp_client_pcb;

// Callback functions
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void tcp_client_err(void *arg, err_t err);

// Function to set up a TCP connection
void tcp_setup(void)
{
	ip_addr_t server_ip;
	IP4_ADDR(&server_ip, 192, 168, 15, 104); // Server IP address

	tcp_client_pcb = tcp_new();
	tcp_bind(tcp_client_pcb, IP_ADDR_ANY, 0);
	tcp_client_pcb->so_options |= SOF_KEEPALIVE;
	tcp_err(tcp_client_pcb, tcp_client_err);
	tcp_connect(tcp_client_pcb, &server_ip, SERVER_PORT, tcp_client_connected);
}

// Function to send an HTTP request
void tcp_send_request(struct tcp_pcb *tpcb)
{
	const char *request = "HEAD /ping HTTP/1.0\r\nHost: 192.168.15.104\r\n\r\n";
	err_t err = tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
	if (err == ERR_OK)
	{
		tcp_output(tpcb);
	}
}

// Callback when connection is established
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if (err == ERR_OK)
	{
		tcp_recv(tpcb, tcp_client_recv);
		tcp_send_request(tpcb);
	}
	else
	{
		tcp_close(tpcb);
	}
	return ERR_OK;
}

// Callback when data is received
err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	if (p != NULL)
	{
		// Process received data
		pbuf_free(p);
	}
	else
	{
		tcp_close(tpcb);
	}
	return ERR_OK;
}

// Error callback
void tcp_client_err(void *arg, err_t err)
{
	// Handle error
}

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

int main()
{
	stdio_init_all();

	setup_wifi();
	tcp_setup();

	while (true)
	{
		sleep_ms(10);
	}

	cyw43_arch_deinit();
	return 0;
}