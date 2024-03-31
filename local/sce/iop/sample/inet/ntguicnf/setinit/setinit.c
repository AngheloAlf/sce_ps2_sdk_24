/* SCEI CONFIDENTIAL
 * "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *	Network GUI Setting Library Sample
 *		Set Configuration Initializer
 *
 *                          Version 1.3
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *	                 All Rights Reserved.
 *
 *                          setinit.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2001.06.01      tetsu      Beta Release
 *        1.2           2001.07.19      tetsu      First Ver. Release
 *        1.3           2001.10.05      tetsu      Add Redial
 */
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
#include <netcnf.h>
#include <inet/inet.h>
#include <inet/inetctl.h>
#include <inet/netdev.h>
#include <ntguicnf.h>

#include "setinit.h"
#include "write_env.h"

#define DEBUG		(1)
#define MEM_AREA_SIZE	(0x1000)
#define MAX_STR_LEN	(256)

/*  --------------------------------------------------------------------
 *  モジュール名
 *  --------------------------------------------------------------------
 */
ModuleInfo Module = {"Set_Cofiguration_Initializer", 0x0103};

/*  --------------------------------------------------------------------
 *  データ領域
 *  --------------------------------------------------------------------
 */
static u_int buf[SSIZE / 4];
static sceNetCnfEnv_t env;
static u_char mem_area[MEM_AREA_SIZE];
static sceNetGuiCnfEnvData_t env_data __attribute__((aligned(64)));

/*  --------------------------------------------------------------------
 *  関数プロトタイプ
 *  --------------------------------------------------------------------
 */
static void setinit_interface(void);
static void *netcnf_interface(u_int fno, void *data, int size);
static void init_data(sceNetGuiCnfEnvData_t *data);
static void init_env(void);
static void dump_data(sceNetGuiCnfEnvData_t *data);
static void dump_env(sceNetCnfEnv_t *e);

int
start(int argc, char *argv[])
{
    struct ThreadParam param;
    int thid;

    /* スレッドの初期化 */
    param.attr		= TH_C;
    param.entry		= setinit_interface;
    param.initPriority	= USER_LOWEST_PRIORITY - 2;
    param.stackSize	= 0x400 * 10;
    param.option	= 0;

    /* スレッドの作成 */
    thid = CreateThread(&param);

    /* スレッドをスタート */
    if( thid > 0 ){
	StartThread(thid, 0);
	return RESIDENT_END;
    }else{
	return NO_RESIDENT_END;
    }
}

static void
setinit_interface(void)
{
    sceSifQueueData qd;
    sceSifServeData sd;

    /* RPC Initialize */
    sceSifInitRpc(0);

    /* Set RPC Queue */
    sceSifSetRpcQueue(&qd, GetThreadId());

    /* Register Function */
    sceSifRegisterRpc(&sd, SETINIT_INTERFACE, netcnf_interface, (void *)&buf[0], 0, 0, &qd);

    /* Goto Wait Loop */
    sceSifRpcLoop(&qd);
}

static void *
netcnf_interface(u_int fno, void *data, int size)
{
    static char disp_name[MAX_STR_LEN]; // inetctl 内でアクセスされるので static であること
    int ret_val       = 0;
    setinit_arg_t *p  = (setinit_arg_t *)data;
    sceNetCnfEnv_t *e = &env;
    int i, redial_count;

    switch(fno)
	{
	case SET_CNF:
	    {
#if DEBUG
		dump_data(&env_data);
#endif /* DEBUG */

		init_env();
		ret_val = write_env(&env_data, e, (void *)mem_area, MEM_AREA_SIZE);
		if(ret_val < 0){
		    printf("[SET_CNF] write_env() error(%d)\n", ret_val);
		    break;
		}

		strcpy(disp_name, "ifc_mem + dev_mem");
		e->root->pair_head->display_name = disp_name;

		/* redial_count を設定する */
		for(i = 0, redial_count = 0; i < sceNetCnf_MAX_PHONE_NUMBERS; i++){
		    if(NULL == (e->root->pair_head->ifc->phone_numbers[i])) continue;
		    switch(i){
		    case 0: redial_count++; break;
		    case 1: redial_count++; break;
		    case 2: redial_count++; break;
		    }
		}
		e->root->pair_head->ifc->redial_count = redial_count - 1;

#if DEBUG
		dump_env(e);
#endif /* DEBUG */

		/* netcnf.irx が参照する設定データを置き換える */
		ret_val = sceInetCtlSetConfiguration(e);
		if(ret_val < 0){
		    printf("[SET_CNF] sceInetCtlSetConfiguration() error(%d)\n", ret_val);
		    break;
		}

		// 接続開始
		sceInetCtlUpInterface(0);
	    }
	    break;

	case DOWN:
	    {
#if DEBUG
		printf("DOWN\n");
#endif /* DEBUG */

		/* 接続解除 */
		ret_val = sceInetCtlDownInterface(0);
		if(ret_val < 0){
		    printf("[DOWN] sceInetCtlDownInterface() error(%d)\n", ret_val);
		    break;
		}
	    }
	    break;

	case GET_ADDR:
	    {
#if DEBUG
		printf("GET_ADDR\n");
#endif /* DEBUG */

		/* env_data の初期化 */
		init_data(&env_data);

		/* env_data のアドレスを EE 側へ通知 */
		ret_val = (int)(&env_data);
	    }
	    break;
	}

    /* 返り値を設定 */
    p->data = ret_val;

    return (data);
}

static void
init_data(sceNetGuiCnfEnvData_t *data)
{
    strcpy(data->attach_ifc     , "");
    strcpy(data->attach_dev     , "");
    strcpy(data->address        , "");
    strcpy(data->netmask        , "");
    strcpy(data->gateway        , "");
    strcpy(data->dns1_address   , "");
    strcpy(data->dns2_address   , "");
    strcpy(data->phone_numbers1 , "");
    strcpy(data->phone_numbers2 , "");
    strcpy(data->phone_numbers3 , "");
    strcpy(data->auth_name      , "");
    strcpy(data->auth_key       , "");
    strcpy(data->vendor         , "");
    strcpy(data->product        , "");
    strcpy(data->chat_additional, "");
    strcpy(data->outside_number , "");
    strcpy(data->outside_delay  , "");
    strcpy(data->dhcp_host_name , "");
    strcpy(data->peer_name      , "");

    data->dialing_type = -1; /* no setting */
    data->type         = 0;  /* type any */
    data->phy_config   = -1; /* no setting */
    data->idle_timeout = -1; /* no setting */
    data->dhcp         = 0;  /* no use dhcp */
    data->dns1_nego    = -1; /* no setting */
    data->dns2_nego    = -1; /* no setting */
    data->f_auth       = 0;  /* no auth */
    data->auth         = 0;  /* default */
    data->pppoe        = 0;  /* no use pppoe */
    data->prc_nego     = -1; /* no setting */
    data->acc_nego     = -1; /* no setting */
    data->accm_nego    = -1; /* no setting */
    data->mtu          = -1; /* no setting */
}

static void
init_env(void)
{
    sceNetCnfEnv_t *e = &env;

    bzero(e, sizeof(sceNetCnfEnv_t));
    e->mem_last = (e->mem_base = e->mem_ptr = mem_area) + sizeof(mem_area);
}

#if DEBUG
static void
dump_data(sceNetGuiCnfEnvData_t *data)
{
    printf("-----------------------\n");
    printf("attach_ifc      : %s\n", data->attach_ifc);
    printf("attach_dev      : %s\n", data->attach_dev);
    printf("address         : %s\n", data->address);
    printf("netmask         : %s\n", data->netmask);
    printf("gateway         : %s\n", data->gateway);
    printf("dns1_address    : %s\n", data->dns1_address);
    printf("dns2_address    : %s\n", data->dns2_address);
    printf("phone_numbers1  : %s\n", data->phone_numbers1);
    printf("phone_numbers2  : %s\n", data->phone_numbers2);
    printf("phone_numbers3  : %s\n", data->phone_numbers3);
    printf("auth_name       : %s\n", data->auth_name);
    printf("auth_key        : %s\n", data->auth_key);
    printf("vendor          : %s\n", data->vendor);
    printf("product         : %s\n", data->product);
    printf("chat_additional : %s\n", data->chat_additional);
    printf("outside_number  : %s\n", data->outside_number);
    printf("outside_delay   : %s\n", data->outside_delay);
    printf("dhcp_host_name  : %s\n", data->dhcp_host_name);
    printf("peer_name       : %s\n", data->peer_name);
    printf("dialing_type    : %d\n", data->dialing_type);
    printf("type            : %d\n", data->type);
    printf("phy_config      : %d\n", data->phy_config);
    printf("idle_timeout    : %d\n", data->idle_timeout);
    printf("dhcp            : %x\n", data->dhcp);
    printf("dns1_nego       : %x\n", data->dns1_nego);
    printf("dns2_nego       : %x\n", data->dns2_nego);
    printf("f_auth          : %d\n", data->f_auth);
    printf("auth            : %d\n", data->auth);
    printf("pppoe           : %x\n", data->pppoe);
    printf("prc_nego        : %x\n", data->prc_nego);
    printf("acc_nego        : %x\n", data->acc_nego);
    printf("accm_nego       : %x\n", data->accm_nego);
    printf("mtu             : %d\n", data->mtu);
    printf("-----------------------\n");
}

static void
dump_env(sceNetCnfEnv_t *e)
{
    int i, dns_no;
    char str_tmp[MAX_STR_LEN];
    sceNetCnfRoot_t *r = NULL;
    sceNetCnfPair_t *p = NULL;
    sceNetCnfInterface_t *ifc = NULL;
    sceNetCnfInterface_t *dev = NULL;
    sceNetCnfCommand_t *c = NULL;
    sceNetCnfRoutingEntry_t *re = NULL;

    printf("-----------------------\n");
    if(e->root != NULL){
	r = e->root;
	if(r->pair_head != NULL){
	    p = r->pair_head;
	    printf("display_name     : \"%s\"\n", p->display_name);
	    printf("attach_ifc       : \"%s\"\n", p->attach_ifc);
	    printf("attach_dev       : \"%s\"\n", p->attach_dev);
	    if(p->ifc != NULL){
		ifc = p->ifc;
		printf("address          : \"%s\"\n", ifc->address);
		printf("netmask          : \"%s\"\n", ifc->netmask);
		printf("auth_name        : \"%s\"\n", ifc->auth_name);
		printf("auth_key         : \"%s\"\n", ifc->auth_key);
		printf("dhcp_host_name   : \"%s\"\n", ifc->dhcp_host_name);
		printf("peer_name        : \"%s\"\n", ifc->peer_name);
		printf("type(ifc)        : \"%d\"\n", ifc->type);
		printf("dhcp             : \"%x\"\n", ifc->dhcp);
		printf("want.dns1_nego   : \"%x\"\n", ifc->want.dns1_nego);
		printf("want.dns2_nego   : \"%x\"\n", ifc->want.dns2_nego);
		printf("allow.f_auth     : \"%d\"\n", ifc->allow.f_auth);
		printf("allow.auth       : \"%d\"\n", ifc->allow.auth);
		printf("pppoe            : \"%x\"\n", ifc->pppoe);
		printf("want.prc_nego    : \"%x\"\n", ifc->want.prc_nego);
		printf("want.acc_nego    : \"%x\"\n", ifc->want.acc_nego);
		printf("want.accm_nego   : \"%x\"\n", ifc->want.accm_nego);
		printf("mtu              : \"%d\"\n", ifc->mtu);
		printf("idle_timeout(ifc): \"%d\"\n", ifc->idle_timeout);
		for(i = 0, dns_no = 0, c = ifc->cmd_head; c != NULL; c = c->forw){
		    switch(c->code){
		    case sceNetCnf_CMD_ADD_NAMESERVER:
			sceNetCnfAddress2String(str_tmp, sizeof(str_tmp), (void *)(c + 1));
			printf("dns%d_address     : \"%s\"\n", dns_no + 1, str_tmp);
			dns_no++;
			break;
		    case sceNetCnf_CMD_ADD_ROUTING:
			re = (void *)(c + 1);
			sceNetCnfAddress2String(str_tmp, sizeof(str_tmp), &re->gateway);
			printf("gateway          : \"%s\"\n", str_tmp);
			break;
		    }
		}
		for(i = 0; i < sceNetCnf_MAX_PHONE_NUMBERS; i++){
		    if(NULL == (ifc->phone_numbers[i])) continue;
		    switch(i){
		    case 0:
			printf("phone_numbers1   : \"%s\"\n", ifc->phone_numbers[0]);
			break;
		    case 1:
			printf("phone_numbers2   : \"%s\"\n", ifc->phone_numbers[1]);
			break;
		    case 2:
			printf("phone_numbers3   : \"%s\"\n", ifc->phone_numbers[2]);
			break;
		    }
		}
	    }
	    if(p->dev != NULL){
		dev = p->dev;
		printf("type(dev)        : \"%d\"\n", dev->type);
		printf("vendor           : \"%s\"\n", dev->vendor);
		printf("product          : \"%s\"\n", dev->product);
		printf("chat_additional  : \"%s\"\n", dev->chat_additional);
		printf("outside_number   : \"%s\"\n", dev->outside_number);
		printf("outside_delay    : \"%s\"\n", dev->outside_delay);
		printf("dialing_type     : \"%d\"\n", dev->dialing_type);
		printf("phy_config       : \"%d\"\n", dev->phy_config);
		printf("idle_timeout(dev): \"%d\"\n", dev->idle_timeout);
	    }
	}
    }
    printf("-----------------------\n");
}
#endif /* DEBUG */
