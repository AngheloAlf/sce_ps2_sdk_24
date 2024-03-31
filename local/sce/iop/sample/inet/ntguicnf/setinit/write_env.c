/* SCEI CONFIDENTIAL
 * "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *	Network GUI Setting Library Sample
 *		Set Configuration Initializer
 *
 *                          Version 1.2
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *	                 All Rights Reserved.
 *
 *                          write_env.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2001.06.01      tetsu      Beta Release
 *        1.2           2001.07.19      tetsu      First Ver. Release
 */
#include <sys/types.h>
#include <ctype.h>
#include <netcnf.h>
#include <ntguicnf.h>

#define MAX_STR_LEN	(256)

/*  --------------------------------------------------------------------
 *  関数プロトタイプ
 *  --------------------------------------------------------------------
 */
static int root_link(sceNetCnfEnv_t *, int);
static int put_net(sceNetCnfEnv_t *, sceNetGuiCnfEnvData_t *);
static int put_attach(sceNetCnfEnv_t *, sceNetGuiCnfEnvData_t *, int);
static int put_cmd(sceNetCnfEnv_t *, sceNetGuiCnfEnvData_t *);
static int put_gw(sceNetCnfEnv_t *, char *);
static int put_ns(sceNetCnfEnv_t *, char *);
static int check_address(char *);

int
write_env(sceNetGuiCnfEnvData_t *data, sceNetCnfEnv_t *e, void *mem_area, int mem_area_size)
{
    int r;

    /* sceNetGuiCnfEnvData の情報を sceNetCnfEnv に設定 */
    r = put_net(e, data);

    /* 使用した分 base をずらす */
    if( r >= 0 )
	{
	    e->mem_ptr = (void *)(((int)e->mem_ptr + 3) & ~3);
	    e->mem_base = e->mem_ptr;
	}

    return (r);
}

static int
root_link(sceNetCnfEnv_t *e, int type)
{
    // root の下の内容を e->ifc に持って来る
    if(type == 1) e->ifc = e->root->pair_head->ifc;
    if(type == 2) e->ifc = e->root->pair_head->dev;

    return (1);
}

static int
put_net(sceNetCnfEnv_t *e, sceNetGuiCnfEnvData_t *data)
{
    sceNetCnfPair_t *p;
    int r = 0;

    // sceNetCnfRoot を作成
    {
	if(NULL == (e->root = sceNetCnfAllocMem(e, sizeof(sceNetCnfRoot_t), 2)))
	    return(sceNETCNF_ALLOC_ERROR);

	e->root->version         = sceNetCnf_CURRENT_VERSION;
	e->root->redial_count    = -1;
	e->root->redial_interval = -1;
	e->root->dialing_type    = -1;
    }

    // sceNetCnfPair を作成
    {
	if(NULL == (p = sceNetCnfAllocMem(e, sizeof(sceNetCnfPair_t), 2)))
	    return(sceNETCNF_ALLOC_ERROR);
    }

    // sceNetCnfRoot -> sceNetCnfPair のリンクを作成
    {
	e->root->pair_head = e->root->pair_tail = p;
	p->forw = p->back = NULL;
    }

    // ifc の内容を設定
    r = put_attach(e, data, 1);
    if(r < 0) return (r);

    // dev の内容を設定
    r = put_attach(e, data, 2);
    if(r < 0) return (r);

    return(r);
}

static int
put_attach(sceNetCnfEnv_t *e, sceNetGuiCnfEnvData_t *data, int type)
{
    int r = 0;
    sceNetCnfPair_t *p;

    // 以下の 2 つのポインタは put_net で設定されていなければならない
    if(e->root == NULL)            return (sceNETCNF_NG);
    if(e->root->pair_head == NULL) return (sceNETCNF_NG);

    p = e->root->pair_head;

    switch(type){
    case 1:
	/* sceNetCnfInterface 領域を確保 */
	if(NULL == (p->ifc = sceNetCnfAllocMem(e, sizeof(sceNetCnfInterface_t), 2)))
	    return(sceNETCNF_ALLOC_ERROR);

	/* Initialize */
	sceNetCnfInitIFC(p->ifc);
	root_link(e, 1); // put_gw, put_ns で e->ifc を使用するために行う

	/* type */
	if(data->pppoe == 1)
	    {
		p->ifc->type = SCE_NETGUICNF_TYPE_PPP;
	    }
	else
	    {
		p->ifc->type = data->type;
	    }

	/* DHCP Server */
	p->ifc->dhcp = data->dhcp;

	/* DHCP ホスト名 */
	if(data->dhcp_host_name[0] != '\0'){
	    p->ifc->dhcp_host_name = data->dhcp_host_name;
	}

	/* IP Address */
	if(data->address[0] != '\0'){
	    p->ifc->address = data->address;
	}

	/* Subnet Mask */
	if(data->netmask[0] != '\0'){
	    p->ifc->netmask = data->netmask;
	}

	/* Default Gateway, Name Servers */
	r = put_cmd(e, data);

	/* アクセスポイント電話番号 1 */
	if(data->phone_numbers1[0] != '\0'){
	    p->ifc->phone_numbers[0] = data->phone_numbers1;
	}

	/* アクセスポイント電話番号 2 */
	if(data->phone_numbers2[0] != '\0'){
	    p->ifc->phone_numbers[1] = data->phone_numbers2;
	}

	/* アクセスポイント電話番号 3 */
	if(data->phone_numbers3[0] != '\0'){
	    p->ifc->phone_numbers[2] = data->phone_numbers3;
	}

	/* ユーザ ID */
	if(data->auth_name[0] != '\0'){
	    p->ifc->auth_name = data->auth_name;
	}

	/* パスワード */
	if(data->auth_key[0] != '\0'){
	    p->ifc->auth_key = data->auth_key;
	}

	/* peer_name */
	if(data->peer_name[0] != '\0'){
	    p->ifc->peer_name = data->peer_name;
	}

	/* プライマリ DNS 自動取得 */
	p->ifc->want.dns1_nego = data->dns1_nego;

	/* セカンダリ DNS 自動取得 */
	p->ifc->want.dns2_nego = data->dns2_nego;

	/* f_auth */
	p->ifc->allow.f_auth = data->f_auth;

	/* auth */
	p->ifc->allow.auth = data->auth;

	/* pppoe */
	if(data->pppoe == 1)
	    {
		p->ifc->pppoe = data->pppoe;
	    }
	else
	    {
		p->ifc->pppoe = -1;
	    }

	/* prc_nego */
	p->ifc->want.prc_nego = data->prc_nego;

	/* acc_nego */
	p->ifc->want.acc_nego = data->acc_nego;

	/* accm_nego */
	p->ifc->want.accm_nego = data->accm_nego;

	/* mtu */
	p->ifc->mtu = data->mtu;

	/* idle_timeout */
	if(p->ifc->pppoe == 1) p->ifc->idle_timeout = data->idle_timeout;
	break;

    case 2:
	/* sceNetCnfInterface 領域を確保 */
	if(NULL == (p->dev = sceNetCnfAllocMem(e, sizeof(sceNetCnfInterface_t), 2)))
	    return(sceNETCNF_ALLOC_ERROR);

	/* Initialize */
	sceNetCnfInitIFC(p->dev);
	root_link(e, 2); // put_gw, put_ns で e->ifc を使用するために行う

	/* type */
	p->dev->type = data->type;

	/* Ether,Nic 動作モード */
	p->dev->phy_config = data->phy_config;

	/* Vendor 名 */
	if(data->vendor[0] != '\0'){
	    p->dev->vendor = data->vendor;
	}

	/* Product 名 */
	if(data->product[0] != '\0'){
	    p->dev->product = data->product;
	}

	/* 追加チャットスクリプト */
	if(data->chat_additional[0] != '\0'){
	    p->dev->chat_additional = data->chat_additional;
	}

	/* 外線発信設定 */
	if(data->outside_number[0] != '\0'){
	    p->dev->outside_number = data->outside_number;
	}
	if(data->outside_delay[0] != '\0'){
	    p->dev->outside_delay = data->outside_delay;
	}

	/* ダイアルタイプ */
	p->dev->dialing_type = data->dialing_type;

	/* 回線切断設定 */
	if(data->pppoe != 1) p->dev->idle_timeout = data->idle_timeout;
	break;
    }

    return (r);
}

static int
put_cmd(sceNetCnfEnv_t *e, sceNetGuiCnfEnvData_t *data)
{
    int r;

    /* Initialize */
    r = 0;

    if( data->dhcp != 1 )
	{
	    /* Default Gateway */
	    if(data->gateway[0] != '\0' && check_address(data->gateway)){
		r = put_gw(e, data->gateway);
	    }else{
		r = put_gw(e, NULL);
	    }
	}

    /* Name Server */
    if(data->dns1_address[0] != '\0' && check_address(data->dns1_address)){
	r = put_ns(e, data->dns1_address);
	if(data->dns2_address[0] != '\0' && check_address(data->dns2_address)){
	    r = put_ns(e, data->dns2_address);
	}
    }

    return (r);
}

static int
put_gw(sceNetCnfEnv_t *e, char *gw)
{
    sceNetCnfCommand_t *p;
    sceNetCnfRoutingEntry_t *re;

    /* sceNetCnfCommand + sceNetCnfRoutingEntry 領域を確保 */
    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(sceNetCnfCommand_t) + sizeof(sceNetCnfRoutingEntry_t), 2)))
	return(sceNETCNF_ALLOC_ERROR);

    /* Initialize */
    p->code = sceNetCnf_CMD_ADD_ROUTING;
    re = (void *)(p + 1);

    /* Default Gateway を形式変換 */
    if(gw == NULL){
	if(0 > sceNetCnfName2Address(&re->dstaddr, NULL)) return(sceNETCNF_NG);
	if(0 > sceNetCnfName2Address(&re->gateway, NULL)) return(sceNETCNF_NG);
	if(0 > sceNetCnfName2Address(&re->genmask, NULL)) return(sceNETCNF_NG);
	re->flags = 0;
    }else{
	if(0 > sceNetCnfName2Address(&re->dstaddr, NULL)) return(sceNETCNF_NG);
	if(0 > sceNetCnfName2Address(&re->gateway, gw))   return(sceNETCNF_NG);
	if(0 > sceNetCnfName2Address(&re->genmask, NULL)) return(sceNETCNF_NG);
	re->flags |= sceNetCnfRoutingF_Gateway;
    }

    /* リンクを作成 */
    (((p->back = e->ifc->cmd_tail) != NULL) ? p->back->forw : e->ifc->cmd_head) = p;
    p->forw = NULL, e->ifc->cmd_tail = p;

    return (0);
}

static int
put_ns(sceNetCnfEnv_t *e, char *ns)
{
    sceNetCnfCommand_t *p;
    struct sceNetCnfAddress *ia;

    /* sceNetCnfCommand + sceNetCnfAddress 領域を確保 */
    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(sceNetCnfCommand_t) + sizeof(sceNetCnfAddress_t), 2)))
	return(sceNETCNF_ALLOC_ERROR);

    /* Initialize */
    p->code = sceNetCnf_CMD_ADD_NAMESERVER;
    ia = (void *)(p + 1);

    /* Primary Name Server を形式変換 */
    if(0 > sceNetCnfName2Address(ia, ns)) return(sceNETCNF_NG);

    /* リンクを作成 */
    (((p->back = e->ifc->cmd_tail) != NULL) ? p->back->forw : e->ifc->cmd_head) = p;
    p->forw = NULL, e->ifc->cmd_tail = p;

    return (0);
}

static int
check_address(char *str)
{
    int r;

    for(r = 0; *str != '\0'; str++){
	if(*str == '.') continue;
	if(*str != '0') r = 1;
    }

    return (r);
}
