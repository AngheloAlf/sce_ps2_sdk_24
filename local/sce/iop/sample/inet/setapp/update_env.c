/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *      Inet Setting Application Sample
 *
 *                          Version 1.4
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                          update_env.c
 *
 *        Version       Date             Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2000.12.22       tetsu      Initial
 *        1.2           2001.01.31       tetsu      Change Final Flow
 *        1.3           2001.03.11       tetsu      Change for HDD
 *        1.4           2001.03.11       tetsu      Change for PPPoE
 */
#include "ifnetcnf.h"

typedef struct route{
  sceNetCnfCommand_t route_cmd;
  sceNetCnfRoutingEntry_t routing;
} route_t;

static route_t gateway __attribute__((aligned(64))); 

typedef struct nameserver{
  sceNetCnfCommand_t route_cmd;
  sceNetCnfAddress_t routing;
} nameserver_t;

static nameserver_t dns1 __attribute__((aligned(64)));
static nameserver_t dns2 __attribute__((aligned(64)));

static char peer_name[1];

static int root_link(sceNetCnfEnv_t *, int);
static int put_net(sceNetCnfEnv_t *, sceNetCnfEnvData_t *);
static int put_attach(sceNetCnfEnv_t *, sceNetCnfEnvData_t *, int);
static int put_cmd(sceNetCnfEnv_t *, sceNetCnfEnvData_t *);
static int put_gw(sceNetCnfEnv_t *, char *);
static int put_ns(sceNetCnfEnv_t *, char *, int);
static int check_address(char *);

/***************************/
/* sceNetCnfEnv を更新する */
/***************************/
int
update_env(int data, int type)
{
  sceNetCnfEnv_t *e = &env;
  int r;

  /* sceNetCnfEnvData の情報を sceNetCnfEnv に設定 */
  switch(type){
  case 0:
    r = put_net(e, (sceNetCnfEnvData_t *)data);
    break;
  case 1:
  case 2:
    r = put_attach(e, (sceNetCnfEnvData_t *)data, type);
    r = root_link(e, type);
    break;
  }

  /* 使用した分 base をずらす */
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
  sceNetCnfPair_t *p;

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
    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(*p), 2)))	      return(sceNETCNF_NG);

    if(type == 1) p->ifc = e->ifc;
    if(type == 2) p->dev = e->ifc;

    /* リンクを作成 */
    (((p->back = e->root->pair_tail) != NULL) ? p->back->forw : e->root->pair_head) = p;
    p->forw = NULL, e->root->pair_tail = p;
  }

  return (1);
}

static int
put_net(sceNetCnfEnv_t *e, sceNetCnfEnvData_t *data)
{
  static char display_name[MAX_LEN];
  sceNetCnfPair_t *p;
  int r = 0;

  /* sceNetCnfRoot 領域を確保 */
  if(e->root == NULL){
    if(NULL == (e->root = sceNetCnfAllocMem(e, sizeof(*e->root), 2))) return(sceNETCNF_NG);
  }

  /* Initialize */
  e->root->version         = sceNetCnf_CURRENT_VERSION;
  e->root->redial_count    = -1;
  e->root->redial_interval = -1;
  e->root->dialing_type    = -1;

    /* sceNetCnfPair 領域を確保 */
  if(e->root->pair_head == NULL){
    if(NULL == (p = sceNetCnfAllocMem(e, sizeof(*p), 2)))	return(sceNETCNF_NG);

    /* リンクを作成 */
    (((p->back = e->root->pair_tail) != NULL) ? p->back->forw : e->root->pair_head) = p;
    p->forw = NULL, e->root->pair_tail = p;
  }else{
    p = e->root->pair_head;
  }

  /* 表示名称の設定 */
  strcpy(display_name, data->attach_ifc);
  strcat(display_name, " + ");
  strcat(display_name, data->attach_dev);
  if(p->display_name == NULL){
    if(NULL == (p->display_name = sceNetCnfAllocMem(e, strlen(display_name) + 1, 0))) return(sceNETCNF_NG);
  }else{
    p->display_name = display_name;
  }

  /* 接続設定ファイル名(system name) を設定 */
  if(p->attach_ifc == NULL){
    if(NULL == (p->attach_ifc = sceNetCnfAllocMem(e, strlen(data->attach_ifc) + 1, 0))) return(sceNETCNF_NG);
  }else{
    p->attach_ifc = data->attach_ifc;
  }

  /* モデム設定ファイル名(system name) を設定 */
  if(p->attach_dev == NULL){
    if(NULL == (p->attach_dev = sceNetCnfAllocMem(e, strlen(data->attach_dev) + 1, 0))) return(sceNETCNF_NG);
  }else{
    p->attach_dev = data->attach_dev;
  }

  /* ifc の内容を設定 */
  r = put_attach(e, data, 1);
  if(r < 0) return (r);
  root_link(e, 1);
  if(r < 0) return (r);

  /* dev の内容を設定 */
  r = put_attach(e, data, 2);
  if(r < 0) return (r);
  root_link(e, 2);
  if(r < 0) return (r);

  return(r);
}

static int
put_attach(sceNetCnfEnv_t *e, sceNetCnfEnvData_t *data, int type)
{
  int i, j, r;
  static char outside_number[MAX_LEN];
  static char outside_delay[MAX_LEN];

  /* Initialize */
  r = 0;

  /* sceNetCnfInterface 領域を確保 */
  if(e->ifc == NULL){
    if(NULL == (e->ifc = sceNetCnfAllocMem(e, sizeof(*(e->ifc)), 2))) return(sceNETCNF_NG);
    sceNetCnfInitIFC(e->ifc);
  }

  switch(type){
  case 1:
    /* type */
    e->ifc->type = data->type;

    /* DHCP Server */
    e->ifc->dhcp = data->dhcp;

    /* DHCP ホスト名 */
    if(data->dhcp_host_name[0] != '\0'){
      e->ifc->dhcp_host_name = data->dhcp_host_name;
    }else{
      e->ifc->dhcp_host_name = NULL;
    }

    /* IP Address */
    if(data->address[0] != '\0'){
      e->ifc->address = data->address;
    }else{
      e->ifc->address = NULL;
    }

    /* Subnet Mask */
    if(data->netmask[0] != '\0'){
      e->ifc->netmask = data->netmask;
    }else{
      e->ifc->netmask = NULL;
    }

    /* Default Gateway, Name Servers */
    r = put_cmd(e, data);

    /* アクセスポイント電話番号 1 */
    if(data->phone_numbers1[0] != '\0'){
      e->ifc->phone_numbers[0] = data->phone_numbers1;
    }else{
      e->ifc->phone_numbers[0] = NULL;
    }

    /* アクセスポイント電話番号 2 */
    if(data->phone_numbers2[0] != '\0'){
      e->ifc->phone_numbers[1] = data->phone_numbers2;
    }else{
      e->ifc->phone_numbers[1] = NULL;
    }

    /* アクセスポイント電話番号 3 */
    if(data->phone_numbers3[0] != '\0'){
      e->ifc->phone_numbers[2] = data->phone_numbers3;
    }else{
      e->ifc->phone_numbers[2] = NULL;
    }

    /* ユーザ ID */
    if(data->auth_name[0] != '\0'){
      e->ifc->auth_name = data->auth_name;
    }else{
      e->ifc->auth_name = NULL;
    }

    /* パスワード */
    if(data->auth_key[0] != '\0'){
      e->ifc->auth_key = data->auth_key;
    }else{
      e->ifc->auth_key = NULL;
    }

    /************************/
    /* 以下, デフォルト情報 */
    /************************/
    if(e->ifc->type == sceNetCnf_IFC_TYPE_PPP){
      e->ifc->peer_name = peer_name;
      strcpy(e->ifc->peer_name, "*");
      if(data->dns1_address[0] == '\0'){
	e->ifc->want.dns1_nego = 1;
	e->ifc->want.dns2_nego = 1;
      }else{
	if(check_address(data->dns1_address) == 0){
	  e->ifc->want.dns1_nego = 1;
	  e->ifc->want.dns2_nego = 1;
	}else{
	  e->ifc->want.dns1_nego = 0xff;
	  e->ifc->want.dns2_nego = 0xff;
	}
      }
      e->ifc->allow.f_auth = 1;
      e->ifc->allow.auth   = 4;
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
    /* type */
    e->ifc->type = data->type;

    /* Ether,Nic 動作モード */
    if(e->ifc->type != sceNetCnf_IFC_TYPE_PPP){
      e->ifc->phy_config = data->phy_config;
    }

    /* Vendor 名 */
    if(data->vendor[0] != '\0'){
      e->ifc->vendor = data->vendor;
    }else{
      e->ifc->vendor = NULL;
    }

    /* Product 名 */
    if(data->product[0] != '\0'){
      e->ifc->product = data->product;
    }else{
      e->ifc->product = NULL;
    }

    /* 追加チャットスクリプト */
    if(data->chat_additional[0] != '\0'){
      e->ifc->chat_additional = data->chat_additional;
    }else{
      e->ifc->chat_additional = NULL;
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
      e->ifc->outside_number = outside_number;
    }else{
      e->ifc->outside_number = NULL;
    }
    if(outside_delay[0] != '\0'){
      e->ifc->outside_delay = outside_delay;
    }else{
      e->ifc->outside_delay = NULL;
    }

    /* ダイアルタイプ */
    if(0 <= data->dialing_type && data->dialing_type <= 2){
      e->ifc->dialing_type = data->dialing_type;
    }else{
      e->ifc->dialing_type = -1;
    }

    /* 回線切断設定(分 → 秒) */
    e->ifc->idle_timeout = data->idle_timeout * 60;
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
  e->ifc->cmd_head = NULL;
  e->ifc->cmd_tail = NULL;

  /* Default Gateway */
  if(data->gateway[0] != '\0' && check_address(data->gateway)){
    r = put_gw(e, data->gateway);
  }

  /* Name Server */
  if(data->dns1_address[0] != '\0' && check_address(data->dns1_address)){
    r = put_ns(e, data->dns1_address, 1);
    if(data->dns2_address[0] != '\0' && check_address(data->dns2_address)){
      r = put_ns(e, data->dns2_address, 2);
    }
  }

  return (r);
}

static int
put_gw(sceNetCnfEnv_t *e, char *gw)
{
  sceNetCnfCommand_t *p;
  sceNetCnfRoutingEntry_t *re;

  /* sceNetCnfCommand + sceNetCnfRoutingEntry 領域を取得 */
  p = (sceNetCnfCommand_t *)&gateway;
  p->code = sceNetCnf_CMD_ADD_ROUTING;

  /* リンクを作成 */
  (((p->back = e->ifc->cmd_tail) != NULL) ? p->back->forw : e->ifc->cmd_head) = p;
  p->forw = NULL, e->ifc->cmd_tail = p;

  /* Initialize */
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

  return (0);
}

static int
put_ns(sceNetCnfEnv_t *e, char *ns, int ns_count)
{
  sceNetCnfCommand_t *p;
  struct sceNetCnfAddress *ia;

  /* sceNetCnfCommand + sceNetCnfAddress 領域を取得 */
  switch(ns_count){
  case 1:
    p = (sceNetCnfCommand_t *)&dns1;
    break;
  case 2:
    p = (sceNetCnfCommand_t *)&dns2;
    break;
  }
  p->code = sceNetCnf_CMD_ADD_NAMESERVER;

  /* リンクを作成 */
  (((p->back = e->ifc->cmd_tail) != NULL) ? p->back->forw : e->ifc->cmd_head) = p;
  p->forw = NULL, e->ifc->cmd_tail = p;

  /* Initialize */
  ia = (void *)(p + 1);
  p->code = sceNetCnf_CMD_ADD_NAMESERVER;

  /* Primary Name Server を形式変換 */
  if(0 > sceNetCnfName2Address(ia, ns)) return(sceNETCNF_NG);

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
