#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stdint.h>
#include "pico/cyw43_arch.h"
#include "lwip/altcp.h"
#include "debug.h"

enum http_states
{
	HTTP_NOT_CONNECTED = 0,
	HTTP_CONNECTING,
	HTTP_CONNECTED,
	HTTP_REQUEST_PENDING,
	HTTP_INITIAL_DATA_PACKET,
	HTTP_WAITING_MORE_DATA,
	HTTP_DATA_READY
};

struct http_connectionState
{
	enum http_states state;
	struct altcp_pcb *pcb;
	char *sendData;
	char *recvData;
	int start;
};

err_t http_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
	DEBUG_PRINTF("data sent %d\n", len);
}

err_t http_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct http_connectionState *cs = (struct http_connectionState *)arg;
	if (p != NULL)
	{
		printf("recv total %d  this buffer %d next %d err %d\n", p->tot_len, p->len, p->next, err);
		if ((p->tot_len) > 2)
		{
			pbuf_copy_partial(p, (cs->recvData) + (cs->start), p->tot_len, 0);
			cs->start += p->tot_len;
			cs->recvData[cs->start] = 0;
			cs->state = HTTP_INITIAL_DATA_PACKET;
			altcp_recved(pcb, p->tot_len);
		}
		pbuf_free(p);
	}
	else
	{
		cs->state = HTTP_DATA_READY;
	}
	return ERR_OK;
}

static err_t http_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
	DEBUG_PRINTF("http connected\n");
	struct http_connectionState *cs = (struct http_connectionState *)arg;
	cs->state = HTTP_CONNECTED;
	return ERR_OK;
}

err_t http_poll(void *arg, struct altcp_pcb *pcb)
{
	DEBUG_PRINTF("Connection Closed \n");
	struct http_connectionState *cs = (struct http_connectionState *)arg;
	cs->state = HTTP_DATA_READY;
}

void http_err(void *arg, err_t err)
{
	if (err != ERR_ABRT)
	{
		printf("client_err %d\n", err);
	}

	// struct http_connectionState *cs = (struct http_connectionState *)arg;
	// if (cs->state == HTTP_CONNECTING)
	// {
	// }
	// cs->state = HTTP_FAILED_TO_CONNECT;
}

struct http_connectionState *http_newConnection(char *sendData, char *recvData)
{
	struct http_connectionState *cs = (struct http_connectionState *)malloc(sizeof(struct http_connectionState));
	cs->state = HTTP_NOT_CONNECTED;
	cs->pcb = altcp_new(NULL);
	altcp_recv(cs->pcb, http_recv);
	altcp_sent(cs->pcb, http_sent);
	altcp_err(cs->pcb, http_err);
	altcp_poll(cs->pcb, http_poll, 10);
	altcp_arg(cs->pcb, cs);
	cs->sendData = sendData;
	cs->recvData = recvData;
	cs->start = 0;
	return cs;
}

struct http_connectionState *http_doRequest(ip_addr_t *ip, char *host, u16_t port, char *header, char *sendData, char *recvData)
{
	int len = strlen(header) + strlen(sendData);
	char *requestData = malloc(len + 1);
	snprintf(requestData, len + 1, "%s%s", header, sendData);
	struct http_connectionState *cs = http_newConnection(requestData, recvData);
	cyw43_arch_lwip_begin();
	err_t err = altcp_connect(cs->pcb, ip, port, http_connected);
	cyw43_arch_lwip_end();
	cs->state = HTTP_CONNECTING;
	return cs;
}

int http_pollRequest(struct http_connectionState **pcs)
{
	if (*pcs == NULL)
		return 0;
	struct http_connectionState *cs = *pcs;
	err_t err;
	switch (cs->state)
	{
	case HTTP_NOT_CONNECTED:
	case HTTP_CONNECTING:
	case HTTP_REQUEST_PENDING:
		break;
	case HTTP_CONNECTED:
		cs->state = HTTP_REQUEST_PENDING;
		cyw43_arch_lwip_begin();
		err = altcp_write(cs->pcb, cs->sendData, strlen(cs->sendData), 0);
		err = altcp_output(cs->pcb);
		cyw43_arch_lwip_end();
		break;
	case HTTP_INITIAL_DATA_PACKET:
		cs->state = HTTP_WAITING_MORE_DATA;
		break;
	case HTTP_DATA_READY:
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