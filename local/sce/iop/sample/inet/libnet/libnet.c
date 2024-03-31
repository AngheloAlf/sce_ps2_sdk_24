/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *                     INET Library
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         libnet - libnet.c
 */

#include <stdio.h>
#include <kernel.h>
#include <common.h>
#include <netcnf.h>
#include <inet/inet.h>
#include <inet/inetctl.h>
#include <inet/in.h>
#include <sifrpc.h>
#include <msifrpc.h>

#define	PRIVATE	static
#define	PUBLIC

#define BASE_priority       64

/* flags for interface event */
#define sceLIBNET_IEV_Attach	0x01
#define sceLIBNET_IEV_Detach	0x02
#define sceLIBNET_IEV_Start	0x04
#define sceLIBNET_IEV_Stop	0x08
#define sceLIBNET_IEV_Error	0x10
#define sceLIBNET_IEV_Conf	0x20
#define sceLIBNET_IEV_NoConf	0x40

#define dump_byte(p, size)  ({ \
	int i; \
	for(i = 0; i < (size); i++) \
	printf("%02x", *((char *)(p) + i) & 0xff); \
	printf("\n"); \
	})

/* satatic structure and value for configuration */
PRIVATE	sceNetCnfEnv_t env;
PRIVATE	u_char mem_area[0x1000];

/* satatic structure and value for interface event handling */
PRIVATE	struct sceInetCtlEventHandlers ev_handlers;
PRIVATE	int if_evid;
PRIVATE	int if_id;

ModuleInfo Module = {"libnet", ( ( 1 << 8 ) | 50 ) };

PRIVATE	void	set_event( int id, u_long flags )
{
	int 	ret;

	if_id = id;
	ret = SetEventFlag( if_evid, flags );
	if ( ret != KE_OK ) {
		printf( "SetEventFlag failed. ret = %d\n", ret );
	}
}

PRIVATE	void	if_ev_handler( int id, int type )
{
	if ( type == sceINETCTL_IEV_Attach ) {
		set_event( id, sceLIBNET_IEV_Attach );
	}
	if ( type == sceINETCTL_IEV_Start ) {
		set_event( id, sceLIBNET_IEV_Start );
	}
}

PRIVATE	void *libnet_dispatch(u_int fno, void *data, int size)
{

	switch(fno) {
	case RPC_SCENAME2ADDR:
		{
		int flags = ((int *)data)[0];
		int ms = ((int *)data)[1];
		int nretry = ((int *)data)[2];
		char name[64];

		strcpy(name, (char *)&((int *)data)[3]);
		((int *)data)[0] =
			sceInetName2Address(flags, (struct sceInetAddress *)&((int *)data)[1], name, ms, nretry);
		}
		break;
	case RPC_SCEADDR2STR:
		{
		struct sceInetAddress addr;
		int len = ((int *)data)[0];

		memcpy(&addr, &((int *)data)[1], sizeof(struct sceInetAddress));
		((int *)data)[0] =
			sceInetAddress2String((char *)&((int *)data)[1], len, &addr);
		}
		break;
	case RPC_SCEADDR2NAME:
		{
		int flags = ((int *)data)[0];
		int len = ((int *)data)[1];
		int ms = ((int *)data)[2];
		int nretry = ((int *)data)[3];
		struct sceInetAddress addr;

		memcpy(&addr, &((int *)data)[4], sizeof(struct sceInetAddress));
		((int *)data)[0] =
			sceInetAddress2Name(flags, (char *)&((int *)data)[1], len, &addr, ms, nretry);
		}
		break;
	case RPC_SCECREATE:
		{
		struct sceInetParam *param = (struct sceInetParam *)data;
		((int *)data)[0] = sceInetCreate(param);
		}
		break;
	case RPC_SCEOPEN:
		{
		int cid = ((int *)data)[0];
		int ms = ((int *)data)[1];
		((int *)data)[0] = sceInetOpen(cid, ms);
		}
		break;
	case RPC_SCECLOSE:
		{
		int cid = ((int *)data)[0];
		int ms = ((int *)data)[1];
		((int *)data)[0] = sceInetClose(cid, ms);
		}
		break;
	case RPC_SCERECV:
		{
		int cid = ((int *)data)[0];
		int count = ((int *)data)[2];
		int ms = ((int *)data)[3];

		((int *)data)[0] =
			sceInetRecv(cid, &((int *)data)[2], count, &((int *)data)[1], ms);
		}
		break;
	case RPC_SCERECVF:
		{
		int cid = ((int *)data)[0];
		int count = ((int *)data)[2];
		int ms = ((int *)data)[3];

		((int *)data)[0] =
			sceInetRecvFrom(cid, &((int *)data)[7], count, &((int *)data)[1],
				(struct sceInetAddress *)&((int *)data)[2], &((int *)data)[6],
				ms);
#if DEBUG
		printf("sceInetRecv(): cid %d: ", cid);
		dump_byte(&((int *)data)[2], count);
#endif
		}
		break;
	case RPC_SCESEND:
		{
		int cid = ((int *)data)[0];
		int count = ((int *)data)[2];
		int ms = ((int *)data)[3];

		((int *)data)[0] =
			sceInetSend(cid, &((int *)data)[4], count, &((int *)data)[1], ms);
		}
		break;
	case RPC_SCESENDT:
		{
		int cid = ((int *)data)[0];
		int count = ((int *)data)[1];
		int flags = ((int *)data)[2];
		int ms = ((int *)data)[3];
		int port = ((int *)data)[4];

		((int *)data)[0] =
			sceInetSendTo(cid, &((int *)data)[9], count, &flags,
				(struct sceInetAddress *)&((int *)data)[5], port, ms);
		}
		break;
	case RPC_SCEABORT:
		{
		int cid = ((int *)data)[0];
		int flags = ((int *)data)[1];
		((int *)data)[0] = sceInetAbort(cid, flags);
		}
		break;
	case RPC_SCECTL:
		{
		int cid = ((int *)data)[0];
		int code = ((int *)data)[1];
		int len = ((int *)data)[2];

		if(len == 0)
			((int *)data)[0] = sceInetControl(cid, code, NULL, len);
		else
			((int *)data)[0] =
				sceInetControl(cid, code, &((int *)data)[3], len);
		}
		break;
	case RPC_SCEGIFLIST:
		{
		int n = ((int *)data)[0];

		((int *)data)[0] =
			sceInetGetInterfaceList(&((int *)data)[1], n);
		}
		break;
	case RPC_SCEIFCTL:
		{
		int interface_id = ((int *)data)[0];
		int code = ((int *)data)[1];
		int len = ((int *)data)[2];

		if(len == 0)
			((int *)data)[0] =
				sceInetInterfaceControl(interface_id, code, NULL, len);
		else
			((int *)data)[0] =
				sceInetInterfaceControl(interface_id, code, &((int *)data)[3], len);
		}
		break;
	case RPC_SCEGETRT:
		{
		int n = ((int *)data)[0];

		((int *)data)[0] =
			sceInetGetRoutingTable((struct sceInetRoutingEntry *)&((int *)data)[1], n);
		}
		break;
	case RPC_SCEGETNS:
		{
		int n = ((int *)data)[0];

		((int *)data)[0] =
			sceInetGetNameServers((struct sceInetAddress *)&((int *)data)[1], n);
		}
		break;
	case RPC_SCECHPRI:
		{
		int prio = ((int *)data)[0];

		((int *)data)[0] = sceInetChangeThreadPriority(prio);
		}
		break;
	case RPC_SCEGETLOG:
		{
		int len = ((int *)data)[0];
		int ms = ((int *)data)[1];

		((int *)data)[0] = sceInetGetLog((char *)&((int *)data)[1], len, ms);
		}
		break;
	case RPC_SCEABORTLOG:
		{
		((int *)data)[0] = sceInetAbortLog();
		}
		break;
	case RPC_REGHANDLER:
		{
		struct EventFlagParam ev_param;

		ev_param.attr = EA_MULTI;
		ev_param.initPattern = 0;
		ev_param.option = 0;

		if((if_evid = CreateEventFlag(&ev_param)) < 0){
			((int *)data)[0] = if_evid;
			break;
		}

		ev_handlers.func = if_ev_handler;

		((int *)data)[0] = sceInetCtlRegisterEventHandler(&ev_handlers);
		}
		break;
	case RPC_UNREGHANDLER:
		{

		if(KE_OK != (((int *)data)[0] = DeleteEventFlag(if_evid)) < 0)
			break;

		((int *)data)[0] = sceInetCtlUnregisterEventHandler(&ev_handlers);
		}
		break;
	case RPC_LOADNETCNF:
		{
		char fname[sceNETCNF_STR_MAX];
		char usr_name[sceNETCNF_STR_MAX];
		int len;

		strcpy(fname, (char *)&((int *)data)[0]);
		len = strlen(fname) + 1;
		strcpy(usr_name, (char *)&((int *)data)[0] + len);

		env.f_no_decode = 1;
		env.mem_last
			= (env.mem_base = env.mem_ptr = mem_area) + sizeof(mem_area);
		((int *)data)[0] =
			sceNetCnfLoadEntry(fname, 0, usr_name, &env);
		}
		break;
	case RPC_SETCONFIG:
		{
		if((((int *)data)[0] = sceInetCtlSetAutoMode(1)) < 0)
			break;
		((int *)data)[0] = sceInetCtlSetConfiguration(&env);
		}
		break;
	case RPC_SETCONFIGONLY:
		{
		((int *)data)[0] = sceInetCtlSetConfiguration(&env);
		}
		break;
	case RPC_UPIF:
		{
		int id = ((int *)data)[0];

		((int *)data)[0] = sceInetCtlUpInterface(id);
		}
		break;
	case RPC_DOWNIF:
		{
		int id = ((int *)data)[0];

		((int *)data)[0] = sceInetCtlDownInterface(id);
		}
		break;
	case RPC_WAITIFATTACHED:
		{
		if(KE_OK != (((int *)data)[0] = WaitEventFlag(if_evid, sceLIBNET_IEV_Attach, EW_OR, NULL)))
			break;

		((int *)data)[1] = if_id;
		}
		break;
	case RPC_WAITIFSTARTED:
		{
		if(KE_OK != (((int *)data)[0] = WaitEventFlag(if_evid, sceLIBNET_IEV_Start, EW_OR, NULL)))
			break;

		((int *)data)[1] = if_id;
		}
		break;
	case RPC_SCETERM:
		{
		}
		break;
	default:
		break;
	}

	return data;
}

PRIVATE	int th_libnet()
{
	sceSifMServeEntry se;

	sceSifMInitRpc(0);

	sceSifMEntryLoop(&se, SIFNUM_libnet, libnet_dispatch, 0);

	return 0;
}

PRIVATE	int Cthread(p, name, f, size, pri)
	struct ThreadParam *p;
	const char *name;
	void *f;
	int size;
	int pri;
{
	int th;

	p->initPriority = pri;
	p->stackSize = size;
	p->entry = f;
	th = CreateThread(p);
	if (th > 0){
		StartThread(th, 0);
		return 0;
	}
	else {
		printf("Create %s thread failed.\n", name);
		return -1;
	}
}

int start(int argc, char *argv[])
{
	struct ThreadParam p;
	int ret;

	sceSifInitRpc(0);

	/* create new thread */
	p.attr = TH_C;
	p.option = 0;

	ret += Cthread(&p, "libnet_dispatch", th_libnet, 8192, BASE_priority);

	return 0;
}
