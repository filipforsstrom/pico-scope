#ifndef REQUST_H
#define REQUST_H

#include <stdint.h>
#include "pico/cyw43_arch.h"
#include "lwip/altcp.h"

struct connectionState
{
	int state;
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
			cs->state = 4;
			altcp_recved(pcb, p->tot_len);
		}
		pbuf_free(p);
	}
	else
	{
		cs->state = 6;
	}
	return ERR_OK;
}

static err_t connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
	struct connectionState *cs = (struct connectionState *)arg;
	cs->state = 2;
	return ERR_OK;
}

err_t poll(void *arg, struct altcp_pcb *pcb)
{
	printf("Connection Closed \n");
	struct connectionState *cs = (struct connectionState *)arg;
	cs->state = 6;
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
	cs->state = 0;
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
	cs->state = 1;
	return cs;
}

int pollRequest(struct connectionState **pcs)
{
	if (*pcs == NULL)
		return 0;
	struct connectionState *cs = *pcs;
	switch (cs->state)
	{
	case 0:
	case 1:
	case 3:
		break;

	case 2:
		cs->state = 3;
		cyw43_arch_lwip_begin();
		err_t err = altcp_write(cs->pcb, cs->sendData, strlen(cs->sendData), 0);
		err = altcp_output(cs->pcb);
		cyw43_arch_lwip_end();
		break;
	case 4:
		cs->state = 5;
		break;
	case 6:
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