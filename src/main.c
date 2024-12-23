#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "lwip/tcp.h"
#include <stdlib.h> // For rand()
#include "setupWifi.h"
char myBuff[1000];

#define DEBUG_printf printf

const ip_addr_t SERVER_IP = IPADDR4_INIT_BYTES(192, 168, 1, 204);
#define SERVER_PORT 5010

struct tcp_pcb *tcp_client_pcb;
bool websocket_connected = false;

// Callback functions
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void tcp_client_err(void *arg, err_t err);

// Function to set up a TCP connection
err_t tcp_setup(void)
{
	ip_addr_t server_ip;
	IP4_ADDR(&server_ip, 192, 168, 1, 204); // Server IP address

	tcp_client_pcb = tcp_new();
	tcp_bind(tcp_client_pcb, IP_ADDR_ANY, 0);
	tcp_client_pcb->so_options |= SOF_KEEPALIVE;
	tcp_err(tcp_client_pcb, tcp_client_err);
	err_t err = tcp_connect(tcp_client_pcb, &server_ip, SERVER_PORT, tcp_client_connected);
	return err;
}

// Function to send an HTTP request
void tcp_send_request(struct tcp_pcb *tpcb)
{
	const char *request = "GET /ws HTTP/1.1\r\n"
						  "Host: 192.168.1.204\r\n"
						  "Upgrade: websocket\r\n"
						  "Connection: Upgrade\r\n"
						  "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
						  "Sec-WebSocket-Version: 13\r\n\r\n";
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
		// Check if WebSocket upgrade is successful
		// For simplicity, assume upgrade is successful upon receiving any data
		websocket_connected = true;

		// Free the received pbuf
		pbuf_free(p);
	}
	else
	{
		// Close the TCP connection
		tcp_close(tpcb);
	}
	return ERR_OK;
}

void tcp_client_err(void *arg, err_t err)
{
	// Handle error
	DEBUG_printf("TCP error: %d\n", err);
}

void send_ws(struct tcp_pcb *tpcb)
{
	float random_value = (float)rand() / (float)(RAND_MAX);
	// Prepare WebSocket frame (unmasked, binary frame)
	uint8_t ws_frame[2 + sizeof(float)];
	ws_frame[0] = 0x82;			 // FIN bit set, binary frame
	ws_frame[1] = sizeof(float); // Payload length
	memcpy(&ws_frame[2], &random_value, sizeof(float));

	tcp_write(tpcb, ws_frame, sizeof(ws_frame), TCP_WRITE_FLAG_COPY);
	tcp_output(tpcb);
	DEBUG_printf("Sent: %f\n", random_value);
}

int main()
{
	stdio_init_all();

	connect(WIFI_SSID, WIFI_PASSWORD);

	err_t err = tcp_setup();
	if (err == ERR_OK)
	{
		while (true)
		{
			if (websocket_connected)
			{
				send_ws(tcp_client_pcb);
				sleep_ms(10);
			}
			else
			{
				sleep_ms(100);
			}
		}
	}

	cyw43_arch_deinit();
	return 0;
}