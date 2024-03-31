/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                         - netstat -
 *
 *                        Version 1.03
 *                          Shift-JIS
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * netstat.c - show network status
 *
 * $Id: netstat.c,v 1.3 2001/08/14 06:22:50 ksh Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kernel.h>

#if defined(USE_LOCAL_INET_HEADERS)
#include "inet.h"
#else	/* USE_LOCAL_INET_HEADERS */
#include <inet/inet.h>
#endif	/* USE_LOCAL_INET_HEADERS */

ModuleInfo Module = { "INET_status", 0x0101 };

static sceInetInfo_t info_tbl[256];
static char buf[1024];

static int usage(void){
	printf("Usage: netstat\n");
	return(-1);
}

static void show_info(sceInetInfo_t *p){
	int n;

	switch(p->proto){
	case sceINETI_PROTO_TCP: printf("tcp  "); break;
	case sceINETI_PROTO_UDP: printf("udp  "); break;
	case sceINETI_PROTO_IP:  printf("ip   "); break;
	default: printf("0x%x ", p->proto); break;
	}
	printf(" %6d", p->recv_queue_length);
	printf(" %6d", p->send_queue_length);
	sceInetAddress2String(buf, sizeof(buf), &p->local_adr);
	n = printf(" %s:%d", buf, p->local_port);
	printf("%*s", 21 - n, "");
	sceInetAddress2String(buf, sizeof(buf), &p->remote_adr);
	n = printf(" %s:%d", buf, p->remote_port);
	printf("%*s", 21 - n, "");
	switch(p->state){
	case sceINETI_STATE_UNKNOWN:		printf(" UNKNOWN");	break;
	case sceINETI_STATE_CLOSED:		printf(" CLOSED");	break;
	case sceINETI_STATE_CREATED:		printf(" CREATED");	break;
	case sceINETI_STATE_OPENED:		printf(" OPENED");	break;
	case sceINETI_STATE_LISTEN:		printf(" LISTEN");	break;
	case sceINETI_STATE_SYN_SENT:		printf(" SYN_SENT");	break;
	case sceINETI_STATE_SYN_RECEIVED:	printf(" SYN_RECEIVED"); break;
	case sceINETI_STATE_ESTABLISHED:	printf(" ESTABLISHED");	break;
	case sceINETI_STATE_FIN_WAIT_1:		printf(" FIN_WAIT_1");	break;
	case sceINETI_STATE_FIN_WAIT_2:		printf(" FIN_WAIT_2");	break;
	case sceINETI_STATE_CLOSE_WAIT:		printf(" CLOSE_WAIT");	break;
	case sceINETI_STATE_CLOSING:		printf(" CLOSING");	break;
	case sceINETI_STATE_LAST_ACK:		printf(" LACK_ACK");	break;
	case sceINETI_STATE_TIME_WAIT:		printf(" TIME_WAIT");	break;
	default: printf(" 0x%x", p->state); break;
	}
	printf("\n");
}

static int netstat(int ac, char *av[]){
	sceInetInfo_t *p;
	int n;

	if(++av, --ac != 0)
		return(usage());
	if(0 > (n = sceInetControl(0, sceINETC_CODE_GET_INFO,
			info_tbl, sizeof(info_tbl)))){
		printf("sceInetControl -> %d\n", n);
		return(n);
	}
	if(n == 0)
		printf("\nNo Info\n");
	else{
		printf("\nProto Recv-Q Send-Q Local Address"
			"        Foreign Address      State\n");
		for(p = info_tbl; 0 < n--; ++p)
			show_info(p);
	}
	return(0);
}

int start(int ac, char *av[]){
	int r;

	r = netstat(ac, av);
	return((r << 4) | NO_RESIDENT_END);
}
