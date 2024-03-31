/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *      Inet Setting Application Sample
 *
 *                          Version 1.5
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                          read_env.c
 *
 *        Version      Date            Design    Log
 *  --------------------------------------------------------------------
 *        1.1          2000.12.22      tetsu     Initial
 *        1.2          2001.01.31      tetsu     Change Final Flow
 *        1.3          2001.03.11      tetsu     Change for HDD
 *        1.4          2001.03.16      tetsu     Change for PPPoE
 *        1.5          2001.07.25      tetsu     link bug fix
 */
#include "ifnetcnf.h"

static int ns_count;

static void get_net(sceNetCnfEnvData_t *, sceNetCnfRoot_t *);
static void get_attach(sceNetCnfEnvData_t *, sceNetCnfInterface_t *, int);
static void get_cmd(sceNetCnfEnvData_t *, sceNetCnfCommand_t *, int *n);

/******************************/
/* sceNetCnfEnv から読み込む */
/******************************/
int
read_env(int data, int type, char *usr_name, char *fname, int no_decode)
{
    sceNetCnfEnv_t *e = &env;
    sceNetCnfPair_t *p;
    int r;

    /* Initialize */
    ns_count       = 0;
    e->f_no_decode = no_decode;

    /* sceNetCnfEnv にデータを読み込む */
    if(0 > (r = sceNetCnfLoadEntry(fname, type, usr_name, e))){
	printf("[read_env] %d = sceNetCnfLoadEntry(%s, %d, %s, 0x%08x)\n"
	       , r, fname, type, usr_name, (int)e);
	return (r);
    }
    e->mem_ptr = (void *)(((int)e->mem_ptr + 3) & ~3);
    e->mem_base = e->mem_ptr;

    /* sceNetCnfEnvData にデータを読み込む */
    switch(type){
    case 0:
	/* NET_CNF を読み込む */
	get_net((sceNetCnfEnvData_t *)data, e->root);
	break;
    case 1:
    case 2:
	/* ATTACH_CNF を読み込む */
	get_attach((sceNetCnfEnvData_t *)data, e->ifc, type);

	/* e->ifc の内容を root の下に持って来る */
	if(e->root != NULL){
	    if(e->root->pair_head != NULL){
		if(type == 1) e->root->pair_head->ifc = e->ifc;
		if(type == 2) e->root->pair_head->dev = e->ifc;
	    }else{
		goto MAKE_PAIR;
	    }
	}else{
	    /* sceNetCnfRoot 領域を確保 */
	    if(NULL == (e->root = sceNetCnfAllocMem(e, sizeof(*e->root), 2))) return(sceNETCNF_NG);

	    /* Initialize */
	    e->root->version         = sceNetCnf_CURRENT_VERSION;
	    e->root->redial_count    = -1;
	    e->root->redial_interval = -1;
	    e->root->dialing_type    = -1;

    MAKE_PAIR:
	    /* sceNetCnfPair 領域を確保 */
	    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(*p), 2)))	return(sceNETCNF_NG);

	    if(type == 1) p->ifc = e->ifc;
	    if(type == 2) p->dev = e->ifc;

	    /* リンクを作成 */
	    e->root->pair_head = e->root->pair_tail = p;
	    p->forw = p->back = NULL;
	}
	break;
    }

    return (r);
}

static void
get_net(sceNetCnfEnvData_t *data, sceNetCnfRoot_t *p)
{
    sceNetCnfPair_t *pair;

    /* 一番最後の pair の設定が有効 */
    for(pair = p->pair_head; pair != NULL; pair = pair->forw){
	/* 設定ファイル名を取得(system name) */
	strcpy(data->attach_ifc, (char *)pair->attach_ifc);
	strcpy(data->attach_dev, (char *)pair->attach_dev);

	/* 設定内容を取得 */
	if(pair->ifc){
	    get_attach((sceNetCnfEnvData_t *)data, pair->ifc, 1);
	}else{
	    printf("[get_net] not ifc.cnf\n");
	}
	if(pair->dev){
	    get_attach((sceNetCnfEnvData_t *)data, pair->dev, 2);
	}else{
	    printf("[get_net] not dev.cnf\n");
	}
    }
}

static void
get_attach(sceNetCnfEnvData_t *data, sceNetCnfInterface_t *p, int type)
{
    int i;
    char *str;
    sceNetCnfCommand_t *cmd;

    switch(type){
    case 1:
	/* type */
	data->type = p->type;

	/* DHCP Server */
	data->dhcp = p->dhcp;

	/* DHCP ホスト名 */
	if(p->dhcp_host_name != NULL) strcpy(data->dhcp_host_name, p->dhcp_host_name);

	/* IP Address */
	if(p->address != NULL) strcpy(data->address, p->address);

	/* Subnet Mask */
	if(p->netmask != NULL) strcpy(data->netmask, p->netmask);

	/* Default Gateway(最後のものが有効), Name Servers(最初の２つが有効) */
	for(i = 0, cmd = p->cmd_head; cmd != NULL; cmd = cmd->forw){
	    get_cmd(data, cmd, &ns_count);
	}

	/* アクセスポイント電話番号(最初から３番目までの電話番号が有効) */
	for(i = 0; i < sceNetCnf_MAX_PHONE_NUMBERS; i++){
	    if(NULL == (str = p->phone_numbers[i])) continue;
	    switch(i){
	    case 0:
		strcpy(data->phone_numbers1, str);
		break;
	    case 1:
		strcpy(data->phone_numbers2, str);
		break;
	    case 2:
		strcpy(data->phone_numbers3, str);
		break;
	    }
	}

	/* ユーザ ID */
	if(p->auth_name != NULL) strcpy(data->auth_name, p->auth_name);

	/* パスワード */
	if(p->auth_key != NULL) strcpy(data->auth_key, p->auth_key);

	/* PPPoE */
	if(p->pppoe == 1){
	    data->pppoe = 1;
	}else{
	    data->pppoe = 0;
	}
	break;

    case 2:
	/* type */
	data->type = p->type;

	/* Ether,Nic 動作モード */
	data->phy_config = p->phy_config;

	/* Vendor 名 */
	if(p->vendor != NULL) strcpy(data->vendor, p->vendor);

	/* Product 名 */
	if(p->product != NULL) strcpy(data->product, p->product);

	/* 追加チャットスクリプト */
	if(p->chat_additional != NULL) strcpy(data->chat_additional, p->chat_additional);

	/* 外線発信設定 */
	if(p->outside_number != NULL) strcpy(data->outside_set, p->outside_number);
	if(p->outside_delay != NULL) strcat(data->outside_set, p->outside_delay);

	/* ダイアルタイプ(0 - 2) */
	if(0 <= p->dialing_type){
	    data->dialing_type = p->dialing_type;
	}else{
	    data->dialing_type = 3;
	}

	/* 回線切断設定(秒 → 分) */
	if(0 > p->idle_timeout){
	    data->idle_timeout = 1; /* 90/60 == 1 */
	}else{
	    data->idle_timeout = p->idle_timeout / 60;
	}
	break;
    }
}

static void
get_cmd(sceNetCnfEnvData_t *data, sceNetCnfCommand_t *p, int *ns_count)
{
    sceNetCnfRoutingEntry_t *re;

    switch(p->code){
    case sceNetCnf_CMD_ADD_NAMESERVER:
	switch(*ns_count){
	case 0: /* Primary Name Server */
	    sceNetCnfAddress2String(data->dns1_address, sizeof(data->dns1_address), (void *)(p + 1));
	    *ns_count = *ns_count + 1;
	    break;
	case 1: /* Secondary Name Server */
	    sceNetCnfAddress2String(data->dns2_address, sizeof(data->dns2_address), (void *)(p + 1));
	    *ns_count = *ns_count + 1;
	    break;
	}
	break;
    case sceNetCnf_CMD_ADD_ROUTING:
	re = (void *)(p + 1);
	sceNetCnfAddress2String(data->gateway, sizeof(data->gateway), &re->gateway); /* Default Gateway */
	break;
    }
}
