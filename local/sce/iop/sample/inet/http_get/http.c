/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                           - http_get -
 *
 *                        Version 1.0.0
 *                          Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * http.c - http sample for IOP
 *
 * $Id: http.c,v 1.4 2001/05/23 09:12:52 komaki Exp $
 */

#if 0
#define HDD_ETHERNET
#endif

#include <kernel.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>

#include <inet/inet.h>
#ifdef HDD_ETHERNET
#include <poweroff.h>
#endif

#define BUFSIZE 1460
#define STACKSIZE 0x4000
#define PRIO	USER_LOWEST_PRIORITY

#define REQUEST_GET1 "GET / HTTP/1.1\r\nHOST: "
#define REQUEST_GET2 "\r\n\r\n"

ModuleInfo Module = { "http_client_iop", 0x0101 };

static char buf[BUFSIZE];
static char server_name[128];

static void http_client(void){
    struct sceInetParam inet_param;
    int r, n, cid, flags, fd, total = 0;
	char get_cmd[128];

    bzero(&inet_param, sizeof(inet_param));
    inet_param.type = sceINETT_CONNECT;
    inet_param.local_port = sceINETP_AUTO;
    if(0 > (r = sceInetName2Address(0, &inet_param.remote_addr, server_name, 0, 0))){
        printf("sceInetName2Address -> %d\n", r);
	return;
    }
    inet_param.remote_port = 80;

    if((r = sceInetAddress2String(buf, 128, &inet_param.remote_addr)) < 0) {
		printf("sceInetAddress2String() failed. ret = %d\n", r);
		return;
	}
    printf("retrieved address for %s = %s\n", server_name, buf);

    if(0 > (cid = sceInetCreate(&inet_param))){
	printf("sceInetCreate -> %d\n", cid);
	return;
    }
    if(0 > (r = sceInetOpen(cid, -1))){
	printf("sceInetOpen -> %d\n", r);
	sceInetClose(cid, -1);
	return;
    }

	strcpy(get_cmd, REQUEST_GET1);
	strcat(get_cmd, server_name);
	strcat(get_cmd, REQUEST_GET2);

	flags = 0;
    n = sceInetSend(cid, get_cmd, strlen(get_cmd), &flags, -1);

    fd = open("host1:index.html", O_CREAT | O_TRUNC | O_RDWR);
    if(fd < 0) {
        printf("failed to open index.html\n");
        return;
    }
 
    while(1){
	flags = 0;
	if(0 > (n = sceInetRecv(cid, buf, BUFSIZE, &flags, -1))){
            printf("sceInetRecv -> %d\n", n);
	    break;
	}
        if(n > 0) {
	    total += n;
	    printf("received %d bytes, total %d\n", n, total);
            write(fd, buf, n);
	}
		if(flags & sceINETF_FIN)
			break;
    }

    close(fd);
    if(0 > (r = sceInetClose(cid, -1))){
	printf("sceInetClose -> %d\n", r);
	return;
    }
	printf("done.\n");

}

static int start_thread(void *func, int size, int attr, int prio, void *argp){

    struct ThreadParam thread_param;
    int thread_id, r;

    thread_param.attr = attr;
    thread_param.entry = func;
    thread_param.initPriority = prio;
    thread_param.stackSize = size;
    thread_param.option = 0;

    if(0 >= (thread_id = CreateThread(&thread_param))){
	printf("CreateThread (%d)\n", thread_id);
	return(-1);
    }
    if(KE_OK != (r = StartThread(thread_id, (u_long)argp))){
	printf("StartThread (%d)\n", r);
	DeleteThread(thread_id);
	return(-1);
    }
    return(0);

}

int start(int ac, char *av[]){

	if(ac != 2){
		printf("usage: %s <URL>\n", av[0]);
		return NO_RESIDENT_END;
	}
	strcpy(server_name, av[1]);

#ifdef HDD_ETHERNET
	PreparePowerOff();
#endif

    if(0 > start_thread(http_client, STACKSIZE, TH_C, PRIO, NULL))
    	return(NO_RESIDENT_END);
	else
    	return(RESIDENT_END);

}
