/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     INET Library
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         load_test - load_test.c
 */

#if 0
#define HDD_ETHERNET
#endif

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inet/inet.h>
#include <inet/in.h>
#ifdef HDD_ETHERNET
#include <poweroff.h>
#endif

#define BUFSIZE 2048
#define STACKSIZE 0x4000
#define PRIO USER_LOWEST_PRIORITY
#define MAX_THREAD 10
#define SERVER_PORT 9012

static void thread_func(void *);
static void make_connection(int);

struct connection_data {
	int sema;
	int fid;
};

static int pktsize = 0;
static int th_num = 0;
static char server_name[64];
static struct connection_data *connection;

int recvmsg(int cid, void *ptr, int count, int *pflags, int ms){
	int len, left = count;

	while(left > 0){
		len = sceInetRecv(cid, ptr, count, pflags, ms);
		if(len < 0)
			return len;
		else if(len == 0)
			break;
		left -= len;
		ptr += len;
	}

	return (count - left);
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

static void load_test(void){
	int i;
	struct SemaParam sema_param;

	printf("pktsize = %d, th_num = %d\n", pktsize, th_num);

	if((connection = (struct connection_data *)AllocSysMemory(0, sizeof(struct connection_data) * th_num, NULL)) == NULL){
		printf("AllocSysMemory() failed.\n");
		return;
	}

	sema_param.attr = SA_THFIFO;
	sema_param.initCount = 0;
	sema_param.maxCount = 1;
	for(i = 0; i < th_num; i++)
		connection[i].sema = CreateSema(&sema_param);

	for(i = 0; i < th_num; i++){
		connection[i].fid = i;
		start_thread(thread_func, STACKSIZE, TH_C, PRIO, &connection[i].fid);
	}

	for(i = 0; i < th_num; i++)
		WaitSema(connection[i].sema);

	return;
}

static void thread_func(void *data) {
	int fid = *(int *)data;

	make_connection(fid);
	SignalSema(connection[fid].sema);
}

static void make_connection(int fid) {
	int ret, w = 1500;
	int cid;
	struct sceInetParam param;
	char trans_buf[BUFSIZE], mem[BUFSIZE];
	int ps;
	int flag;

	memset(&param, 0, sizeof(struct sceInetParam));

	param.type = sceINETT_CONNECT;
	param.local_port = sceINETP_AUTO;
	if((ret = sceInetName2Address(0, &param.remote_addr, server_name, 0, 0)) < 0){
		printf("return value of sceInetName2Address(): %d\n", ret);
		return;
	}
	param.remote_port = SERVER_PORT;

	if((cid = sceInetCreate(&param)) <= 0){
		printf("sceInetCreate() returns %d\n", cid);
		return;
	}

	if(sceINETE_OK != (ret = sceInetOpen(cid, -1))){
		printf("test: sceInetOpen() failed.(%d)\n", ret);
		(void) sceInetClose(cid, -1);
		return;
	}

	ps = htonl(pktsize);
	flag = 0;
	ret = sceInetSend(cid, &ps, 4, &flag, -1);

	while(w--){
		if(w%100 == 0)
			printf("thread %d : w = %d\n", fid, w); 

		memcpy(mem, trans_buf, pktsize);

		flag = 0;
		ret = sceInetSend(cid, trans_buf, pktsize, &flag, -1);

		if(ret != pktsize)
			printf("write count is not %d byte, but %d byte\n", pktsize, ret);

		flag = 0;
		ret = recvmsg(cid, trans_buf, pktsize, &flag, -1);
		if(flag & sceINETF_FIN)
			return;
		else if(ret != pktsize)
			printf("read count is not %d byte, but %d byte\n", pktsize, ret);
		
		if(memcmp(trans_buf, mem, pktsize))
			printf(" sent data and receipt data is not same.\n");
	}

	(void) sceInetClose(cid, -1);

	return;
}

int start(int argc, char **argv)
{

	if(argc != 4){
		printf("usage: %s <saddr> <th_num> <pktsize>\n", argv[0]);
		return 0;
	}

	strcpy(server_name, argv[1]);

	if ((th_num = atoi(argv[2])) <= 0){
		printf("th_num = %d\n", th_num);
		return 0;
	}
	if ((pktsize = atoi(argv[3])) <= 0){
		printf("pktsize = %d\n", pktsize);
		return 0;
	}

#ifdef HDD_ETHERNET
	PreparePowerOff();
#endif

	if(0 > start_thread(load_test, STACKSIZE, TH_C, PRIO, NULL))
		return(NO_RESIDENT_END);
	else
		return(RESIDENT_END);
}
