#ifndef REQUST_H
#define REQUST_H

#include <stdint.h>
#include "pico/cyw43_arch.h"
#include "lwip/altcp.h"

enum states
{
	NOT_CONNECTED = 0,
	CONNECTING,
	CONNECTED,
	UPGRADED,
	REQUEST_PENDING,
	INITIAL_DATA_PACKET,
	WAITING_MORE_DATA,
	DATA_READY
};

struct connectionState
{
	enum states state;
	struct altcp_pcb *pcb;
	char *sendData;
	char *recvData;
	int start;
};

err_t sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
	printf("data sent %d\n", len);
}

err_t recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct connectionState *cs = (struct connectionState *)arg;
	if (p != NULL)
	{
		printf("recv total %d  this buffer %d next %d err %d\n", p->tot_len, p->len, p->next, err);
		if ((p->tot_len) > 2)
		{
			pbuf_copy_partial(p, (cs->recvData) + (cs->start), p->tot_len, 0);
			cs->start += p->tot_len;
			cs->recvData[cs->start] = 0;
			cs->state = INITIAL_DATA_PACKET;
			altcp_recved(pcb, p->tot_len);

			// Check for "101 Switching Protocols"
			if (strncmp(cs->recvData, "HTTP/1.1 101", 12) == 0)
			{
				printf("Switching to WebSocket\n");
				cs->state = UPGRADED;
			}
		}
		pbuf_free(p);
	}
	else
	{
		cs->state = DATA_READY;
	}
	return ERR_OK;
}

static err_t connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
	struct connectionState *cs = (struct connectionState *)arg;
	cs->state = CONNECTED;
	return ERR_OK;
}

err_t poll(void *arg, struct altcp_pcb *pcb)
{
	printf("Connection Closed \n");
	struct connectionState *cs = (struct connectionState *)arg;
	cs->state = DATA_READY;
}

void err(void *arg, err_t err)
{
	if (err != ERR_ABRT)
	{
		printf("client_err %d\n", err);
	}
}

struct connectionState *newConnection(char *sendData, char *recvData)
{
	struct connectionState *cs = (struct connectionState *)malloc(sizeof(struct connectionState));
	cs->state = NOT_CONNECTED;
	cs->pcb = altcp_new(NULL);
	altcp_recv(cs->pcb, recv);
	altcp_sent(cs->pcb, sent);
	altcp_err(cs->pcb, err);
	altcp_poll(cs->pcb, poll, 10);
	altcp_arg(cs->pcb, cs);
	cs->sendData = sendData;
	cs->recvData = recvData;
	cs->start = 0;
	return cs;
}

struct connectionState *doRequest(ip_addr_t *ip, char *host, u16_t port, char *header, char *sendData, char *recvData)
{
	int len = strlen(header) + strlen(sendData);
	char *requestData = malloc(len + 1);
	snprintf(requestData, len + 1, "%s%s", header, sendData);
	struct connectionState *cs = newConnection(requestData, recvData);
	cyw43_arch_lwip_begin();
	err_t err = altcp_connect(cs->pcb, ip, port, connected);
	cyw43_arch_lwip_end();
	cs->state = CONNECTING;
	return cs;
}

int pollRequest(struct connectionState **pcs)
{
	if (*pcs == NULL)
		return 0;
	struct connectionState *cs = *pcs;
	switch (cs->state)
	{
	case NOT_CONNECTED:
	case CONNECTING:
	case UPGRADED:
	case REQUEST_PENDING:
		break;

	case CONNECTED:
		cs->state = REQUEST_PENDING;
		cyw43_arch_lwip_begin();
		err_t err = altcp_write(cs->pcb, cs->sendData, strlen(cs->sendData), 0);
		err = altcp_output(cs->pcb);
		cyw43_arch_lwip_end();
		break;
	case INITIAL_DATA_PACKET:
		cs->state = WAITING_MORE_DATA;
		break;
	case DATA_READY:
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