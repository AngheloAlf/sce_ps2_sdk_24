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
 *                          write_env.c
 *
 *        Version      Date            Design    Log
 *  --------------------------------------------------------------------
 *        1.1          2000.12.22      tetsu     Initial
 *        1.2          2001.01.31      tetsu     Change Final Flow
 *        1.3          2001.03.11      tetsu     Change for HDD
 *        1.4          2001.03.16      tetsu     Change for PPPoE
 *        1.5          2001.07.25      tetsu     On Memory PPPoE Bug Fix
 */
#include "ifnetcnf.h"

enum { type_eth = 1,
       type_ppp = 2,
       type_nic = 3 };

static int root_link(sceNetCnfEnv_t *, int);
static int put_net(sceNetCnfEnv_t *, sceNetCnfEnvData_t *);
static int put_attach(sceNetCnfEnv_t *, sceNetCnfEnvData_t *, int);
static int put_cmd(sceNetCnfEnv_t *, sceNetCnfEnvData_t *);
static int put_gw(sceNetCnfEnv_t *, char *);
static int put_ns(sceNetCnfEnv_t *, char *);
static int check_address(char *);

/****************************/
/* sceNetCnfEnv に書き込む */
/****************************/
int
write_env(int data, int type, char *fname, int no_decode)
{
    sceNetCnfEnv_t *e = &env;
    int r = 0;

    // sceNetCnfEnv を初期化
    bzero(e, sizeof(sceNetCnfEnv_t));

    // 設定管理ファイル名の設定
    e->dir_name  = fname;

    // メモリプールの初期化
    e->mem_last  = (e->mem_base = e->mem_ptr = mem_area) + sizeof(mem_area);

    // 符号化設定
    e->f_no_decode = no_decode;

    // リクエストモードの設定
    switch(type){
    case 0:
	e->req = sceNetCnf_REQ_NET;
	break;
    case 1:
    case 2:
	e->req = sceNetCnf_REQ_ATTACH;
	break;
    }

    // sceNetCnfEnvData の情報を sceNetCnfEnv に設定
    switch(type){
    case 0:
	r = put_net(e, (sceNetCnfEnvData_t *)data);
	break;
    case 1:
    case 2:
	r = put_attach(e, (sceNetCnfEnvData_t *)data, type);
	break;
    }

    // 使用した分 base をずらす
    e->mem_ptr = (void *)(((int)e->mem_ptr + 3) & ~3);
    e->mem_base = e->mem_ptr;

#ifdef DEBUG
    // dump_env(e);
#endif /* DEBUG */

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
put_net(sceNetCnfEnv_t *e, sceNetCnfEnvData_t *data)
{
    char display_name[MAX_LEN];
    sceNetCnfPair_t *p;
    int r = 0;

    // sceNetCnfRoot を作成
    {
	if(NULL == (e->root = sceNetCnfAllocMem(e, sizeof(sceNetCnfRoot_t), 2))) return(sceNETCNF_NG);
	e->root->version         = sceNetCnf_CURRENT_VERSION;
	e->root->redial_count    = -1;
	e->root->redial_interval = -1;
	e->root->dialing_type    = -1;
    }

    // sceNetCnfPair を作成
    {
	if(NULL == (p = sceNetCnfAllocMem(e, sizeof(sceNetCnfPair_t), 2)))	return(sceNETCNF_NG);

	// 表示名称の設定
	sprintf(display_name, "%s + %s", data->attach_ifc, data->attach_dev);
	if(NULL == (p->display_name = sceNetCnfAllocMem(e, strlen(display_name) + 1, 0))) return(sceNETCNF_NG);
	strcpy(p->display_name, display_name);

	// 接続設定ファイル名(system name) を設定
	if(NULL == (p->attach_ifc = sceNetCnfAllocMem(e, strlen(data->attach_ifc) + 1, 0))) return(sceNETCNF_NG);
	strcpy(p->attach_ifc, data->attach_ifc);

	// モデム設定ファイル名(system name) を設定
	if(NULL == (p->attach_dev = sceNetCnfAllocMem(e, strlen(data->attach_dev) + 1, 0))) return(sceNETCNF_NG);
	strcpy(p->attach_dev, data->attach_dev);
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
put_attach(sceNetCnfEnv_t *e, sceNetCnfEnvData_t *data, int type)
{
    int i, j, r = 0;
    char outside_number[MAX_LEN];
    char outside_delay[MAX_LEN];
    sceNetCnfPair_t *p;

    // sceNetCnfRoot を作成
    if(e->root == NULL)
	{
	    if(NULL == (e->root = sceNetCnfAllocMem(e, sizeof(sceNetCnfRoot_t), 2))) return(sceNETCNF_NG);
	    e->root->version         = sceNetCnf_CURRENT_VERSION;
	    e->root->redial_count    = -1;
	    e->root->redial_interval = -1;
	    e->root->dialing_type    = -1;
	}

    // sceNetCnfPair を作成
    if(e->root->pair_head == NULL)
	{
	    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(sceNetCnfPair_t), 2))) return(sceNETCNF_NG);

	    // sceNetCnfRoot -> sceNetCnfPair のリンクを作成
	    {
		e->root->pair_head = e->root->pair_tail = p;
		p->forw = p->back = NULL;
	    }
	}
    else
	{
	    // sceNetCnfPair は既に存在
	    p = e->root->pair_head;
	}

    switch(type){
    case 1:
	// sceNetCnfInterface 領域を確保
	if(NULL == (p->ifc = sceNetCnfAllocMem(e, sizeof(sceNetCnfInterface_t), 2))) return(sceNETCNF_NG);
	sceNetCnfInitIFC(p->ifc);
	root_link(e, 1);

	/* type */
	if(data->pppoe == 1)
	    {
		e->ifc->type = type_ppp;
	    }
	else
	    {
		e->ifc->type = data->type;
	    }

	/* DHCP Server */
	e->ifc->dhcp = data->dhcp;

	/* DHCP ホスト名 */
	if(data->dhcp_host_name[0] != '\0'){
	    if(NULL == (e->ifc->dhcp_host_name = sceNetCnfAllocMem(e, strlen(data->dhcp_host_name) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->dhcp_host_name, data->dhcp_host_name);
	}

	/* IP Address */
	if(data->address[0] != '\0'){
	    if(NULL == (e->ifc->address = sceNetCnfAllocMem(e, strlen(data->address) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->address, data->address);
	}

	/* Subnet Mask */
	if(data->netmask[0] != '\0'){
	    if(NULL == (e->ifc->netmask = sceNetCnfAllocMem(e, strlen(data->netmask) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->netmask, data->netmask);
	}

	/* Default Gateway, Name Servers */
	r = put_cmd(e, data);

	/* アクセスポイント電話番号 1 */
	if(data->phone_numbers1[0] != '\0'){
	    if(NULL == (e->ifc->phone_numbers[0] = sceNetCnfAllocMem(e, strlen(data->phone_numbers1) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->phone_numbers[0], data->phone_numbers1);
	}

	/* アクセスポイント電話番号 2 */
	if(data->phone_numbers2[0] != '\0'){
	    if(NULL == (e->ifc->phone_numbers[1] = sceNetCnfAllocMem(e, strlen(data->phone_numbers2) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->phone_numbers[1], data->phone_numbers2);
	}

	/* アクセスポイント電話番号 3 */
	if(data->phone_numbers3[0] != '\0'){
	    if(NULL == (e->ifc->phone_numbers[2] = sceNetCnfAllocMem(e, strlen(data->phone_numbers3) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->phone_numbers[2], data->phone_numbers3);
	}

	/* ユーザ ID */
	if(data->auth_name[0] != '\0'){
	    if(NULL == (e->ifc->auth_name = sceNetCnfAllocMem(e, strlen(data->auth_name) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->auth_name, data->auth_name);
	}

	/* パスワード */
	if(data->auth_key[0] != '\0'){
	    if(NULL == (e->ifc->auth_key = sceNetCnfAllocMem(e, strlen(data->auth_key) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->auth_key, data->auth_key);
	}

	/************************/
	/* 以下, デフォルト情報 */
	/************************/
	if(e->ifc->type == sceNetCnf_IFC_TYPE_PPP){
	    if(NULL == (e->ifc->peer_name = sceNetCnfAllocMem(e, strlen("*") + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->peer_name, "*");
	    if(data->dns1_address[0] == '\0'){
		e->ifc->want.dns1_nego = 1;
		e->ifc->want.dns2_nego = 1;
	    }else{
		if(check_address(data->dns1_address) == 0){
		    e->ifc->want.dns1_nego = 1;
		    e->ifc->want.dns2_nego = 1;
		}
	    }
	    e->ifc->allow.f_auth   = 1;
	    e->ifc->allow.auth     = 4;
	    put_gw(e, NULL);
	    if(data->pppoe == 1){
		e->ifc->pppoe          = 1;
		e->ifc->want.prc_nego  = 0;
		e->ifc->want.acc_nego  = 0;
		e->ifc->want.accm_nego = 0;
		e->ifc->mtu            = 1454;
		e->ifc->idle_timeout   = 0;
	    }
	}
	break;

    case 2:
	// sceNetCnfInterface 領域を確保
	if(NULL == (p->dev = sceNetCnfAllocMem(e, sizeof(sceNetCnfInterface_t), 2))) return(sceNETCNF_NG);
	sceNetCnfInitIFC(p->dev);
	root_link(e, 2);

	/* type */
	e->ifc->type = data->type;

	/* Ether,Nic 動作モード */
	if(e->ifc->type != sceNetCnf_IFC_TYPE_PPP){
	    e->ifc->phy_config = data->phy_config;
	}

	/* Vendor 名 */
	if(data->vendor[0] != '\0'){
	    if(NULL == (e->ifc->vendor = sceNetCnfAllocMem(e, strlen(data->vendor) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->vendor, data->vendor);
	}

	/* Product 名 */
	if(data->product[0] != '\0'){
	    if(NULL == (e->ifc->product = sceNetCnfAllocMem(e, strlen(data->product) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->product, data->product);
	}

	/* 追加チャットスクリプト */
	if(data->chat_additional[0] != '\0'){
	    if(NULL == (e->ifc->chat_additional = sceNetCnfAllocMem(e, strlen(data->chat_additional) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->chat_additional, data->chat_additional);
	}

	/* 外線発信設定 */
	i = 0;
	if(data->outside_set[i] != '\0'){
	    for( ; isdigit(data->outside_set[i]) != 0; i++){
		outside_number[i] = data->outside_set[i];
	    }
	}
	outside_number[i] = '\0';
	j = 0;
	for( ; data->outside_set[i] != '\0'; i++, j++){
	    outside_delay[j] = data->outside_set[i];
	}
	outside_delay[j] = '\0';
	if(outside_number[0] != '\0'){
	    if(NULL == (e->ifc->outside_number = sceNetCnfAllocMem(e, strlen(outside_number) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->outside_number, outside_number);
	}
	if(outside_delay[0] != '\0'){
	    if(NULL == (e->ifc->outside_delay = sceNetCnfAllocMem(e, strlen(outside_delay) + 1, 0))) return(sceNETCNF_NG);
	    strcpy(e->ifc->outside_delay, outside_delay);
	}

	/* ダイアルタイプ */
	if(0 <= data->dialing_type && data->dialing_type <= 2){
	    e->ifc->dialing_type = data->dialing_type;
	}else{
	    e->ifc->dialing_type = -1;
	}

	/* 回線切断設定(分 → 秒) */
	if(data->pppoe != 1) e->ifc->idle_timeout = data->idle_timeout * 60;
	break;
    }

    return (r);
}

static int
put_cmd(sceNetCnfEnv_t *e, sceNetCnfEnvData_t *data)
{
    int r;

    /* Initialize */
    r = 0;

    /* Default Gateway */
    if(data->gateway[0] != '\0' && check_address(data->gateway)){
	r = put_gw(e, data->gateway);
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
    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(*p) + sizeof(*re), 2))) return(sceNETCNF_NG);

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
    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(*p) + sizeof(*ia), 2))) return(sceNETCNF_NG);

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
