#ifndef WS_REQUEST_H
#define WS_REQUEST_H

#include <stdint.h>
#include "pico/cyw43_arch.h"
#include "lwip/altcp.h"

enum ws_states
{
	WS_NOT_CONNECTED = 0,
	WS_CONNECTING,
	WS_CONNECTED,
	WS_UPGRADING,
	WS_UPGRADED,
	WS_SENDING,
	WS_REQUEST_PENDING,
	WS_INITIAL_DATA_PACKET,
	WS_WAITING_MORE_DATA,
	WS_DATA_READY
};

struct ws_connectionState
{
	enum ws_states state;
	struct altcp_pcb *pcb;
	char *sendData;
	char *recvData;
	int start;
};

err_t ws_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
	printf("data sent %d\n", len);
}

err_t ws_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct ws_connectionState *cs = (struct ws_connectionState *)arg;
	if (p != NULL)
	{
		printf("recv total %d  this buffer %d next %d err %d\n", p->tot_len, p->len, p->next, err);
		if ((p->tot_len) > 2)
		{
			pbuf_copy_partial(p, (cs->recvData) + (cs->start), p->tot_len, 0);
			cs->start += p->tot_len;
			cs->recvData[cs->start] = 0;
			// cs->state = WS_INITIAL_DATA_PACKET;
			altcp_recved(pcb, p->tot_len);
		}
		pbuf_free(p);
	}
	else
	{
		// cs->state = WS_DATA_READY;
	}
	return ERR_OK;
}

void ws_createWebSocketFrame(const char *data, char *frame, size_t *frameSize)
{
	size_t dataLen = strlen(data);
	frame[0] = 0x81; // FIN bit set and text frame
	if (dataLen <= 125)
	{
		frame[1] = (uint8_t)dataLen;
		memcpy(&frame[2], data, dataLen);
		*frameSize = 2 + dataLen;
	}
	else if (dataLen <= 65535)
	{
		frame[1] = 126;
		frame[2] = (dataLen >> 8) & 0xFF;
		frame[3] = dataLen & 0xFF;
		memcpy(&frame[4], data, dataLen);
		*frameSize = 4 + dataLen;
	}
	else
	{
		// Handle larger payloads if necessary
	}
}

err_t ws_wsUpgradeRecv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct ws_connectionState *cs = (struct ws_connectionState *)arg;
	if (p != NULL)
	{
		printf("recv total %d  this buffer %d next %d err %d\n", p->tot_len, p->len, p->next, err);
		if ((p->tot_len) > 2)
		{
			pbuf_copy_partial(p, (cs->recvData) + (cs->start), p->tot_len, 0);
			cs->start += p->tot_len;
			cs->recvData[cs->start] = 0;
			altcp_recved(pcb, p->tot_len);

			// Check for "101 Switching Protocols"
			if (strncmp(cs->recvData, "HTTP/1.1 101", 12) == 0)
			{
				printf("Switching to WebSocket\n");
				cs->state = WS_UPGRADED;
			}
		}
		// pbuf_free(p);
	}
	else
	{
		// cs->state = WS_DATA_READY;
	}
	return ERR_OK;
}

err_t ws_wsRecv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
	printf("wsRecv\n");
	return ERR_OK;
}

static err_t ws_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
	struct ws_connectionState *cs = (struct ws_connectionState *)arg;
	cs->state = WS_CONNECTED;
	return ERR_OK;
}

err_t ws_poll(void *arg, struct altcp_pcb *pcb)
{
	printf("Connection Closed \n");
	struct ws_connectionState *cs = (struct ws_connectionState *)arg;
	// cs->state = WS_DATA_READY;
}

void ws_err(void *arg, err_t err)
{
	if (err != ERR_ABRT)
	{
		printf("client_err %d\n", err);
	}
}

// struct ws_connectionState *newConnection(char *sendData, char *recvData)
// {
// 	struct ws_connectionState *cs = (struct ws_connectionState *)malloc(sizeof(struct ws_connectionState));
// 	cs->state = WS_NOT_CONNECTED;
// 	cs->pcb = altcp_new(NULL);
// 	altcp_recv(cs->pcb, ws_recv);
// 	altcp_sent(cs->pcb, ws_sent);
// 	altcp_err(cs->pcb, ws_err);
// 	altcp_poll(cs->pcb, ws_poll, 10);
// 	altcp_arg(cs->pcb, cs);
// 	cs->sendData = sendData;
// 	cs->recvData = recvData;
// 	cs->start = 0;
// 	return cs;
// }

// struct ws_connectionState *ws_doRequest(ip_addr_t *ip, char *host, u16_t port, char *header, char *sendData, char *recvData)
// {
// 	int len = strlen(header) + strlen(sendData);
// 	char *requestData = malloc(len + 1);
// 	snprintf(requestData, len + 1, "%s%s", header, sendData);
// 	struct ws_connectionState *cs = newConnection(requestData, recvData);
// 	cyw43_arch_lwip_begin();
// 	err_t err = altcp_connect(cs->pcb, ip, port, ws_connected);
// 	cyw43_arch_lwip_end();
// 	cs->state = WS_CONNECTING;
// 	return cs;
// }

struct ws_connectionState *ws_newWsUpgradeConnection(char *sendData, char *recvData)
{
	struct ws_connectionState *cs = (struct ws_connectionState *)malloc(sizeof(struct ws_connectionState));
	cs->pcb = altcp_new(NULL);
	altcp_recv(cs->pcb, ws_wsUpgradeRecv);
	altcp_sent(cs->pcb, ws_sent);
	altcp_err(cs->pcb, ws_err);
	// altcp_poll(cs->pcb, poll, 10);
	altcp_arg(cs->pcb, cs);
	cs->sendData = sendData;
	cs->recvData = recvData;
	cs->start = 0;
	return cs;
}

struct ws_connectionState *ws_doWsHandshakeRequest(ip_addr_t *ip, char *host, u16_t port, char *recvData)
{
	char header[] = "GET /ws HTTP/1.1\r\n"
					"Host: 192.168.1.204\r\n"
					"Upgrade: websocket\r\n"
					"Connection: Upgrade\r\n"
					"Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
					"Sec-WebSocket-Version: 13\r\n\r\n";
	int len = strlen(header);
	char *requestData = malloc(len + 1);
	snprintf(requestData, len + 1, "%s", header);
	struct ws_connectionState *cs = ws_newWsUpgradeConnection(requestData, recvData);
	cyw43_arch_lwip_begin();
	err_t err = altcp_connect(cs->pcb, ip, port, ws_connected);
	cyw43_arch_lwip_end();
	cs->state = WS_UPGRADING;
	return cs;
}

// struct ws_connectionState *ws_newWsConnection(char *sendData, char *recvData)
// {
// 	struct ws_connectionState *cs = (struct ws_connectionState *)malloc(sizeof(struct ws_connectionState));
// 	cs->pcb = altcp_new(NULL);
// 	altcp_recv(cs->pcb, ws_wsRecv);
// 	altcp_sent(cs->pcb, ws_sent);
// 	altcp_err(cs->pcb, ws_err);
// 	// altcp_poll(cs->pcb, poll, 10);
// 	altcp_arg(cs->pcb, cs);
// 	cs->sendData = sendData;
// 	cs->recvData = recvData;
// 	cs->start = 0;
// 	return cs;
// }

// struct ws_connectionState *ws_doWsRequest(ip_addr_t *ip, char *host, u16_t port, char *header, char *sendData, char *recvData)
// {
// 	int len = strlen(header) + strlen(sendData);
// 	char *requestData = malloc(len + 1);
// 	snprintf(requestData, len + 1, "%s%s", header, sendData);
// 	struct ws_connectionState *cs = ws_newWsConnection(requestData, recvData);
// 	cyw43_arch_lwip_begin();
// 	err_t err = altcp_connect(cs->pcb, ip, port, NULL);
// 	cyw43_arch_lwip_end();
// 	cs->state = WS_SENDING;
// 	return cs;
// }

struct ws_connectionState *ws_dataTransfer(struct ws_connectionState **pcs)
{
	if (*pcs == NULL)
		return 0;
	struct ws_connectionState *cs = *pcs;
	// TODO: Update pcb with suitable callbacks
	cs->state = WS_SENDING;
	return cs;
}

int ws_pollRequest(struct ws_connectionState **pcs)
{
	if (*pcs == NULL)
		return 0;
	struct ws_connectionState *cs = *pcs;
	err_t err;
	switch (cs->state)
	{
	case WS_NOT_CONNECTED:
	case WS_CONNECTING:
	case WS_UPGRADING:
		break;
	case WS_UPGRADED:
		return 0;
	case WS_SENDING:
		printf("Sending WebSocket frame\n");
		char data[32];
		snprintf(data, sizeof(data), "%f", 3.0);
		char frame[256];
		size_t frameSize;
		ws_createWebSocketFrame(data, frame, &frameSize);

		err_t err = altcp_write(cs->pcb, frame, frameSize, 0);
		// err = altcp_output(cs->pcb); //?
		if (err != ERR_OK)
		{
			printf("Error sending WebSocket frame: %d\n", err);
		}
		break;
	case WS_REQUEST_PENDING:
		break;
	case WS_CONNECTED:
		cs->state = WS_REQUEST_PENDING;
		cyw43_arch_lwip_begin();
		err = altcp_write(cs->pcb, cs->sendData, strlen(cs->sendData), 0);
		err = altcp_output(cs->pcb);
		cyw43_arch_lwip_end();
		break;
	case WS_INITIAL_DATA_PACKET:
		cs->state = WS_WAITING_MORE_DATA;
		break;
	case WS_DATA_READY:
		cyw43_arch_lwip_begin();
		altcp_close(cs->pcb);
		cyw43_arch_lwip_end();
		free(cs);
		*pcs = NULL;
		return 0;
	}
	return cs->state;
}

#endif