/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *      Inet Setting Application Sample
 *
 *                          Version 1.6
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                          ifnetcnf.c
 *
 *        Version       Date             Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2000.12.22       tetsu      Initial
 *        1.2           2001.01.31       tetsu      Change Final Flow
 *        1.3           2001.03.11       tetsu      Change for HDD
 *        1.4           2001.03.16       tetsu      Change for PPPoE
 *        1.5           2001.07.26       tetsu      event bug fix
 *        1.6           2001.09.27       tetsu      Add redial_count
 */
#include "ifnetcnf.h"

ModuleInfo Module = {"Inet_Setting_Application_Sample", 0x0106};

/* �C�x���g�t���O��� */
struct EventFlagParam event_param;
int evf_id;

/* for CpuSuspendIntr(), CpuResumeIntr() */
static int oldstat;

/* Rpc_NetCnf �֐��o�b�t�@ */
static u_int buf0 [SSIZE / 4];							/* for ifnetcnf_interface */
static u_int buf1 [SSIZE / 4];							/* for ifnetcnf_status */

/* NetCnf Information */
sceNetCnfEnv_t env;
u_char mem_area[0x1000];
static sceNetCnfEnvData_t env_data __attribute__((aligned(64)));		/* EE <-> IOP �ł��Ƃ肷��ݒ�t�@�C�����e */

/* Network Interface */
static int dev_count;								/* Network Interface ID �̐� */
static int dev_id[MAX_LEN];							/* Network Interface ID �̃��X�g */
static sceInetInterfaceList_t dev_list[MAX_LEN] __attribute__((aligned(64)));	/* Network Interface ID + vendor,product �̃��X�g */

static int dev_count2;								/* Network Interface ID �̐� */
static int dev_id2[MAX_LEN];							/* Network Interface ID �̃��X�g */
static sceInetInterfaceList_t dev_list2[MAX_LEN] __attribute__((aligned(64)));	/* Network Interface ID + vendor,product �̃��X�g */

/* Event Handler */
#define HANDLER_RESULT_NUM_MAX (10)
static sceInetCtlEventHandlers_t eh;
static int handler_result_num;
static int handler_result[HANDLER_RESULT_NUM_MAX];
static int handler_id     = -1;

/********************/
/* �֐��v���g�^�C�v */
/********************/
void usb_autoload_setup(void);
void ifnetcnf_interface(void);
void ifnetcnf_status(void);
void *netcnf_status(u_int, void *, int);
void event_handler(int, int);
void *netcnf_interface(u_int, void *, int);
void send_ee(u_int, u_int, u_int, u_int);
int get_if_list(int, int, char *, char *, char *, int, int, int, int *);
int check_device(char *, int, int, sceNetCnfList_t *, sceNetCnfList2_t *, int, int);
int get_another_name(char *, char *, char *, int);
void init_data(sceNetCnfEnvData_t *data);
char *make_path(char *, char *);
void init_env(void);
int get_hwaddr(int, u_char *, int);

/*********/
/* START */
/*********/
int
start(int argc, char *argv[])
{
  struct ThreadParam param0, param1, param2;
  int thid0, thid1, thid2;
  int i;

  /* �X���b�h�̏����� */
  param0.attr		= TH_C;
  param0.entry		= ifnetcnf_status;
  param0.initPriority	= USER_LOWEST_PRIORITY - 1;
  param0.stackSize	= 0x400 * 2;
  param0.option		= 0;

  param1.attr		= TH_C;
  param1.entry		= ifnetcnf_interface;
  param1.initPriority	= USER_LOWEST_PRIORITY - 2;
  param1.stackSize	= 0x400 * 10;
  param1.option		= 0;

  param2.attr		= TH_C;
  param2.entry		= usb_autoload_setup;
  param2.initPriority	= USER_LOWEST_PRIORITY - 3;
  param2.stackSize	= 0x400;
  param2.option		= 0;

  /* �X���b�h�̍쐬 */
  thid0 = CreateThread(&param0);
  thid1 = CreateThread(&param1);
  thid2 = CreateThread(&param2);

  /* �C�x���g�t���O�̍쐬 */
  event_param.attr        = EA_SINGLE;
  event_param.initPattern = 0;
  event_param.option      = 0;
  evf_id = CreateEventFlag(&event_param);

  /* �C�x���g�n���h����o�^ */
  eh.func = event_handler;
  sceInetCtlRegisterEventHandler(&eh);

  // �C�x���g�n���h���Ŏg�p����ϐ��̏�����
  handler_result_num = -1;
  for(i = 0; i < HANDLER_RESULT_NUM_MAX; i++){
    handler_result[i] = -1;
  }

  /* �X���b�h���X�^�[�g */
  if( thid0 > 0 && thid1 > 0 && thid2 > 0){
    StartThread(thid2, 0);
    StartThread(thid1, 0);
    StartThread(thid0, 0);
    return RESIDENT_END;
  }else{
    return NO_RESIDENT_END;
  }
}

/**************************/
/* USB �I�[�g���[�_�̐ݒ� */
/**************************/
void
usb_autoload_setup(void)
{
  /* �J�e�S����o�^ */
  sceUsbmlActivateCategory("Ether");
  sceUsbmlActivateCategory("Modem");

  /* �I�[�g���[�h������ */
  sceUsbmlEnable();
}

/*****************************************/
/* IOP ���� Server �Ƃ��� RPC �֌W��ݒ� */
/*****************************************/
void
ifnetcnf_interface(void)
{
  sceSifQueueData qd;
  sceSifServeData sd;

  /* RPC Initialize */
  sceSifInitRpc(0);

  /* Set RPC Queue */
  sceSifSetRpcQueue(&qd, GetThreadId());

  /* Register Function */
  sceSifRegisterRpc(&sd, NETCNF_INTERFACE, netcnf_interface, (void *)&buf0[0], 0, 0, &qd);

  /* Goto Wait Loop */
  sceSifRpcLoop(&qd);
}

void
ifnetcnf_status(void)
{
  sceSifQueueData qd;
  sceSifServeData sd;

  /* RPC Initialize */
  sceSifInitRpc(0);

  /* Set RPC Queue */
  sceSifSetRpcQueue(&qd, GetThreadId());

  /* Register Function */
  sceSifRegisterRpc(&sd, NETCNF_STATUS, netcnf_status, (void *)&buf1[0], 0, 0, &qd);

  /* Goto Wait Loop */
  sceSifRpcLoop(&qd);
}

/******************/
/* �R�}���h���` */
/******************/
void *
netcnf_status(u_int fno, void *data, int size)
{
  sceNetCnf_t *p;
  u_long result;

  /* Initialize */
  p = (sceNetCnf_t *)data;

  switch(fno){
  case WAIT_EVENT:
    /**************************************/
    /* < �w�肵�� ID �̃C�x���g��҂� >   */
    /*     p->data    : �C���^�t�F�[�X ID */
    /**************************************/

    /* ������ */
    if(p->data != -1) handler_id = p->data;

    if(handler_result_num < 0)
	{
#ifdef DEBUG
	    printf("[WAIT_EVENT] Wait...\n");
#endif /* DEBUG */
	    /* �C�x���g���N����܂ő҂� */
	    WaitEventFlag(evf_id, 0x1, EW_OR | EW_CLEAR, &result);
	}
    break;
  }

  /* �Ԃ�l��ݒ� */
  if(handler_result_num != -1)
      {
	  p->data = handler_result[handler_result_num];
	  handler_result_num--;
      }
  else
      {
	  p->data = -1;
      }
#ifdef DEBUG
  switch(p->data){
  case (-1):			printf("[WAIT_EVENT] Wake up(-1)\n");		break;
  case sceINETCTL_IEV_Attach:	printf("[WAIT_EVENT] Wake up(Attach)\n");	break;
  case sceINETCTL_IEV_Detach:	printf("[WAIT_EVENT] Wake up(Detach)\n");	break;
  case sceINETCTL_IEV_Start:	printf("[WAIT_EVENT] Wake up(Start)\n");	break;
  case sceINETCTL_IEV_Stop:	printf("[WAIT_EVENT] Wake up(Stop)\n");		break;
  case sceINETCTL_IEV_Error:	printf("[WAIT_EVENT] Wake up(Error)\n");	break;
  case sceINETCTL_IEV_Conf:	printf("[WAIT_EVENT] Wake up(Conf)\n");		break;
  case sceINETCTL_IEV_NoConf:	printf("[WAIT_EVENT] Wake up(NoConf)\n");	break;
  }
#endif /* DEBUG */

  return (data);
}

void
event_handler(int id, int type)
{
    static int pstate;

    if(handler_id == -1) goto TYPE_CHECK;
    if(handler_id == id){
 TYPE_CHECK:
	// Start, Stop, Error �̂݃C�x���g���L������
	// ������Attach, Detach, Conf, NoConf���d�Ȃ����� Attach �����A�D�悳���
	if(type == sceINETCTL_IEV_Start || type == sceINETCTL_IEV_Stop || type == sceINETCTL_IEV_Error)
	    {
		if(0 < ++handler_result_num)
		    {
			int i;
			for(i = handler_result_num - 1; 0 <= i; i--)
			    {
				handler_result[i + 1] = handler_result[i];
			    }
		    }
	    }
	else
	    {
		// Attach, Detach, Conf, NoConf �͐ڑ����� I/F �̂��̂�
		// �ڑ��@��̐ڑ����ƍ�������̂ŗ����͂Ƃ�Ȃ�
		handler_result_num = 0;
	    }
	switch(type){
	case sceINETCTL_IEV_Attach:	handler_result[0] = IEV_Attach;	break;
	case sceINETCTL_IEV_Detach:	handler_result[0] = IEV_Detach;	break;
	case sceINETCTL_IEV_Start:	handler_result[0] = IEV_Start;	break;
	case sceINETCTL_IEV_Stop:	handler_result[0] = IEV_Stop;	break;
	case sceINETCTL_IEV_Error:
	    {
		// ���_�C�A�������ǂ����̊m�F
		if(sceNETCNF_NG != sceInetCtlGetState(id, &pstate))
		    {
			switch(pstate)
			    {
			    case sceINETCTL_S_STOPPED:
				handler_result[0] = IEV_Error;
				break;
			    default:
				handler_result[0] = IEV_Redial;
				break;
			    }
		    }
		else
		    {
			handler_result[0] = IEV_Error;
		    }
	    }
	    break;
	case sceINETCTL_IEV_Conf:	handler_result[0] = IEV_Conf;	break;
	case sceINETCTL_IEV_NoConf:	handler_result[0] = IEV_NoConf;	break;
	}
    }
    SetEventFlag(evf_id, 0x1);
}

void *
netcnf_interface(u_int fno, void *data, int size)
{
  static u_char hwaddr[16];
  static char disp_name[MAX_LEN];
  static char disp_ifc_name[MAX_LEN];
  static char disp_dev_name[MAX_LEN];
  static sceNetCnfEnv_t *e = &env;
  static sceNetCnfList_t *list;
  static sceNetCnfList2_t *list2;
  sceNetCnf_t *p;
  int i, ret_val, ret_val2, ret_val3, redial_count;
  static int error_device;
  char *sp;

  /* Initialize */
  p            = (sceNetCnf_t *)data;
  ret_val      = 0;
  ret_val2     = 0;
  ret_val3     = 0;
  error_device = NO_SELECT;

  switch(fno){
  case GET_COUNT:
    /******************************************************/
    /* < �w�肵����ނ̃t�@�C�������擾 >                 */
    /*     p->type       : �t�@�C���̎��                 */
    /*     p->device_stat: �f�o�C�X�̏��                 */
    /*     p->dir_name   : �ݒ�Ǘ��t�@�C���̃p�X��(MC)   */
    /*     p->dir_name2  : �ݒ�Ǘ��t�@�C���̃p�X��(HDD)  */
    /*     p->dir_name3  : �ݒ�Ǘ��t�@�C���̃p�X��(HOST) */
    /******************************************************/

    /* �w�肵����ނ̃t�@�C�������擾 */
    while(p->device_stat){
      /* device check */
      if(p->device_stat & DEVICE_STAT_MC_ON){
	p->device_stat &= DEVICE_STAT_MC_OFF;
	ret_val2 = sceNetCnfGetCount(p->dir_name, p->type);
	if(0 <= ret_val2){
	  ret_val += ret_val2;
	}else{
	  error_device = SELECT_MC;
	  ret_val = ret_val2;
	  break;
	}
      }else{
	if(p->device_stat & DEVICE_STAT_HDD_ON){
	  p->device_stat &= DEVICE_STAT_HDD_OFF;
	  ret_val2 = sceNetCnfGetCount(p->dir_name2, p->type);
	  if(0 <= ret_val2){
	    ret_val += ret_val2;
	  }else{
	    error_device = SELECT_HDD;
	    ret_val = ret_val2;
	    break;
	  }
	}else{
	  if(p->device_stat & DEVICE_STAT_HOST_ON){
	    p->device_stat &= DEVICE_STAT_HOST_OFF;
	    ret_val2 = sceNetCnfGetCount(p->dir_name3, p->type);
	    if(0 <= ret_val2){
	      ret_val += ret_val2;
	    }else{
	      error_device = SELECT_HOST;
	      ret_val = ret_val2;
	      break;
	    }
	  }
	}
      }
    }
    break;

  case GET_LIST:
    /******************************************************/
    /* < �w�肵����ނ̃t�@�C�����X�g���擾 >             */
    /*     p->dst_addr   : EE �� destination address      */
    /*     p->size       : �擾����t�@�C����             */
    /*     p->type       : �t�@�C���̎��                 */
    /*     p->device_stat: �f�o�C�X�̏��                 */
    /*     p->dir_name   : �ݒ�Ǘ��t�@�C���̃p�X��(MC)   */
    /*     p->dir_name2  : �ݒ�Ǘ��t�@�C���̃p�X��(HDD)  */
    /*     p->dir_name3  : �ݒ�Ǘ��t�@�C���̃p�X��(HOST) */
    /******************************************************/

    /* �t�@�C�����X�g�̃��������m�� */
    CpuSuspendIntr(&oldstat);
    list  = (sceNetCnfList_t *)AllocSysMemory(0, (u_long)(p->size * sizeof(sceNetCnfList_t)), NULL);
    if(list == NULL){
      CpuResumeIntr(oldstat);
      ret_val = sceNETCNF_ALLOC_ERROR;
      break;
    }
    list2 = (sceNetCnfList2_t *)AllocSysMemory(0, (u_long)(p->size * sizeof(sceNetCnfList2_t)), NULL);
    if(list2 == NULL){
      if(list) FreeSysMemory((void *)list);
      CpuResumeIntr(oldstat);
      ret_val = sceNETCNF_ALLOC_ERROR;
      break;
    }
    CpuResumeIntr(oldstat);

    ret_val = 0;
    while(p->device_stat){
      if(p->device_stat & DEVICE_STAT_MC_ON){
	p->device_stat &= DEVICE_STAT_MC_OFF;
	/* �t�@�C�����X�g���擾 */
	ret_val2 = sceNetCnfGetList(p->dir_name, p->type, (list + ret_val));
	if(ret_val2 < 0){
	  ret_val      = ret_val2;
	  error_device = SELECT_MC;
	  CpuSuspendIntr(&oldstat);
	  FreeSysMemory((void *)list); 
	  FreeSysMemory((void *)list2); 
	  CpuResumeIntr(oldstat);
	  break;
	}else{
	  /* �f�o�C�X���`�F�b�N(inet.cnf �̏ꍇ�� ifc.cnf �̑��݂��`�F�b�N) */
	  ret_val3 = check_device(p->dir_name, ret_val2, p->type, (list + ret_val), (list2 + ret_val), p->no_decode, SELECT_MC);
	  if(ret_val3 < 0){
	    ret_val      = ret_val3;
	    error_device = SELECT_MC;
	    CpuSuspendIntr(&oldstat);
	    FreeSysMemory((void *)list); 
	    FreeSysMemory((void *)list2); 
	    CpuResumeIntr(oldstat);
	    break;
	  }

	  /* ret_val update */
	  ret_val += ret_val2;
	}
      }else{
	if(p->device_stat & DEVICE_STAT_HDD_ON){
	  p->device_stat &= DEVICE_STAT_HDD_OFF;
	  /* �t�@�C�����X�g���擾 */
	  ret_val2 = sceNetCnfGetList(p->dir_name2, p->type, (list + ret_val));
	  if(ret_val2 < 0){
	    ret_val      = ret_val2;
	    error_device = SELECT_HDD;
	    CpuSuspendIntr(&oldstat);
	    FreeSysMemory((void *)list); 
	    FreeSysMemory((void *)list2); 
	    CpuResumeIntr(oldstat);
	    break;
	  }else{
	    /* �f�o�C�X���`�F�b�N(inet.cnf �̏ꍇ�� ifc.cnf �̑��݂��`�F�b�N) */
	    ret_val3 = check_device(p->dir_name2, ret_val2, p->type, (list + ret_val), (list2 + ret_val), p->no_decode2, SELECT_HDD);
	    if(ret_val3 < 0){
	      ret_val      = ret_val3;
	      error_device = SELECT_HDD;
	      CpuSuspendIntr(&oldstat);
	      FreeSysMemory((void *)list); 
	      FreeSysMemory((void *)list2); 
	      CpuResumeIntr(oldstat);
	      break;
	    }

	    /* ret_val update */
	    ret_val += ret_val2;
	  }
	}else{
	  if(p->device_stat & DEVICE_STAT_HOST_ON){
	    p->device_stat &= DEVICE_STAT_HOST_OFF;
	    /* �t�@�C�����X�g���擾 */
	    ret_val2 = sceNetCnfGetList(p->dir_name3, p->type, (list + ret_val));
	    if(ret_val2 < 0){
	      ret_val      = ret_val2;
	      error_device = SELECT_HOST;
	      CpuSuspendIntr(&oldstat);
	      FreeSysMemory((void *)list); 
	      FreeSysMemory((void *)list2); 
	      CpuResumeIntr(oldstat);
	      break;
	    }else{
	      /* �f�o�C�X���`�F�b�N(inet.cnf �̏ꍇ�� ifc.cnf �̑��݂��`�F�b�N) */
	      ret_val3 = check_device(p->dir_name3, ret_val2, p->type, (list + ret_val), (list2 + ret_val), p->no_decode3, SELECT_HOST);
	      if(ret_val3 < 0){
		ret_val      = ret_val3;
		error_device = SELECT_HOST;
		CpuSuspendIntr(&oldstat);
		FreeSysMemory((void *)list); 
		FreeSysMemory((void *)list2); 
		CpuResumeIntr(oldstat);
		break;
	      }

	      /* ret_val update */
	      ret_val += ret_val2;
	    }
	  }
	}
      }
    }
    if(ret_val < 0) break;
#ifdef DEBUG
//    list_dump(list2, p->size);
#endif /* DEBUG */

    /* EE �փt�@�C�����X�g��]�� */
    send_ee((u_int)list2, (u_int)p->dst_addr, (u_int)(p->size * sizeof(sceNetCnfList2_t)), (u_int)0);

    /* �t�@�C�����X�g�̃���������� */
    CpuSuspendIntr(&oldstat);
    FreeSysMemory((void *)list); 
    FreeSysMemory((void *)list2); 
    CpuResumeIntr(oldstat);
    break;

  case LOAD_ENTRY:
    /**********************************************/
    /* < �w�肵���t�@�C����ǂݍ��� >             */
    /*     p->dst_addr: EE �� destination address */
    /*     p->type    : �t�@�C���̎��            */
    /*     p->usr_name: �擾����t�@�C����        */
    /*     p->dir_name: �ݒ�Ǘ��t�@�C���̃p�X��  */
    /**********************************************/

    /* sceNetCnfEnvData �������� */
    init_data(&env_data);

    /* �w�肵���t�@�C����ǂݍ��� */
    init_env();
    ret_val = read_env((int)&env_data, p->type, p->usr_name, p->dir_name, p->no_decode);
    if(ret_val < 0){
      printf("[LOAD_ENTRY] read_env() error(%d)\n", ret_val);
      break;
    }

    /* EE �փt�@�C�����e��]�� */
    send_ee((u_int)&env_data, (u_int)p->dst_addr, (u_int)sizeof(sceNetCnfEnvData_t), (u_int)0);
    break;

  case ADD_ENTRY:
    /****************************************************/
    /* < �ݒ�Ǘ��t�@�C���ɒǉ����A�t�@�C���ɏ����o�� > */
    /*     p->src_addr: source address                  */
    /*     p->type    : �t�@�C���̎��                  */
    /*     p->usr_name: �ǉ�����ݒ薼                  */
    /*     p->dir_name: �ݒ�Ǘ��t�@�C���̃p�X��        */
    /****************************************************/

#ifdef DEBUG
    dump_data((sceNetCnfEnvData_t *)p->src_addr);
#endif /* DEBUG */
    /* sceNetCnfEnv �փf�[�^���������� */
    ret_val = write_env(p->src_addr, p->type, p->dir_name, p->no_decode);
    if(ret_val < 0){
      printf("[ADD_ENTRY] write_env() error(%d)\n", ret_val);
      break;
    }

    /* �ݒ�Ǘ��t�@�C���ɒǉ����A�t�@�C���ɏ����o�� */
    ret_val = sceNetCnfAddEntry(p->dir_name, p->type, p->usr_name, &env);
    if(ret_val < 0){
      printf("[ADD_ENTRY] sceNetCnfAddEntry() error(%d)\n", ret_val);
    }
    break;

  case EDIT_ENTRY:
    /****************************************************/
    /* < �ݒ�Ǘ��t�@�C����ҏW���A�t�@�C���ɏ����o�� > */
    /*     p->src_addr : source address                 */
    /*     p->type     : �t�@�C���̎��                 */
    /*     p->usr_name : �ҏW����ݒ薼                 */
    /*     p->usr_name2: �V�����ݒ薼                   */
    /*     p->dir_name : �ݒ�Ǘ��t�@�C���̃p�X��       */
    /****************************************************/

    /* �����̃f�[�^��ǂݍ��� */
    init_env();
    env.f_no_decode = p->no_decode;
    if(0 > (ret_val = sceNetCnfLoadEntry(p->dir_name, p->type, p->usr_name, &env))){
      printf("[EDIT_ENTRY] %d = sceNetCnfLoadEntry(%s, %d, %s, 0x%08x)\n"
	     , ret_val, p->dir_name, p->type, p->usr_name, (int)(&env));
      break;
    }
    env.mem_ptr = (void *)(((int)env.mem_ptr + 3) & ~3);
    env.mem_base = env.mem_ptr;

    /* sceNetCnfEnv ���X�V���� */
    ret_val = update_env(p->src_addr, p->type);
    if(ret_val < 0){
      printf("[EDIT_ENTRY] update_env() error(%d)\n", ret_val);
      break;
    }

    /* �ݒ�Ǘ��t�@�C����ҏW���A�t�@�C���ɏ����o�� */
    ret_val = sceNetCnfEditEntry(p->dir_name, p->type, p->usr_name, p->usr_name2, &env);
    if(ret_val < 0){
      printf("[EDIT_ENTRY] sceNetCnfEditEntry() error(%d)\n", ret_val);
    }
    break;

  case DELETE_ENTRY:
    /************************************************************/
    /* < �ݒ�Ǘ��t�@�C������G���g�����폜���A�t�@�C�����폜 > */
    /*     p->type    : �t�@�C���̎��                          */
    /*     p->usr_name: �폜����ݒ薼                          */
    /*     p->dir_name: �ݒ�Ǘ��t�@�C���̃p�X��                */
    /************************************************************/

    /* �ݒ�Ǘ��t�@�C������G���g�����폜���A�t�@�C�����폜 */
    ret_val = sceNetCnfDeleteEntry(p->dir_name, p->type, p->usr_name);
    if(ret_val < 0){
      printf("[DELETE_ENTRY] sceNetCnfDeleteEntry() error(%d)\n", ret_val);
    }
    break;

   case DELETE_ALL:
    /*********************************************/
    /* < �w�肳�ꂽ�f�o�C�X��̐ݒ��S�č폜 >  */
    /*     p->dir_name: �ݒ�Ǘ��t�@�C���̃p�X�� */
    /*********************************************/

    /* �f�o�C�X�����쐬 */
    sp = p->dir_name;
    while(*sp != ':') sp++;
    *(sp + 1) = '\0';

    /* �w�肳�ꂽ�f�o�C�X��̐ݒ��S�č폜 */
    ret_val = sceNetCnfDeleteAll(p->dir_name);
    if(ret_val < 0){
      printf("[DELETE_ALL] sceNetCnfDeleteAll() error(%d)\n", ret_val);
    }
    break;

 case SET_CNF:
    /***********************************************************************************/
    /* < netcnf.irx ���Q�Ƃ���ݒ�f�[�^��u�������A�ݒ�Ǘ��t�@�C������׊����A�ڑ� > */
    /*     p->data       : �t���O                                                      */
    /*     p->src_addr   : source address                                              */
    /*     p->no_decode  : no_decode �t���O                                            */
    /*     p->no_decode2 : no_decode �t���O                                            */
    /*     p->usr_name   : �擾����t�@�C����                                          */
    /*     p->usr_name2  : �擾����t�@�C����                                          */
    /*     p->dir_name   : �ݒ�Ǘ��t�@�C���̃p�X��                                    */
    /*     p->dir_name2  : �ݒ�Ǘ��t�@�C���̃p�X��                                    */
    /***********************************************************************************/

#ifdef DEBUG
    if(p->data < 3) dump_data((sceNetCnfEnvData_t *)p->src_addr);
#endif /* DEBUG */

    switch(p->data){
      /* �S�Ẵf�[�^����������ɂ���ꍇ */
    case 0:
      ret_val = write_env(p->src_addr, (int)0, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] write_env() error(%d)\n", p->data, ret_val);
      }

      strcpy(disp_name, "ifc_mem + dev_mem");
      e->root->pair_head->display_name = disp_name;
      break;

      /* dev �f�[�^����������Aifc �f�[�^���t�@�C���ɂ���ꍇ */
    case 1:
      ret_val = write_env(p->src_addr, (int)2, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] write_env() error(%d)\n", p->data, ret_val);
	break;
      }
      printf("%p\n", e->root->pair_head->dev);

      ret_val = read_env((int)&env_data, (int)1, p->usr_name, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] read_env() error(%d)\n", p->data, ret_val);
      }

      strcpy(disp_ifc_name, "");
      get_another_name(disp_ifc_name, p->usr_name, p->dir_name, 1);
      strcpy(disp_name, disp_ifc_name);
      strcat(disp_name, " + dev_mem");
      e->root->pair_head->display_name = disp_name;
      break;

      /* ifc �f�[�^����������Adev �f�[�^���t�@�C���ɂ���ꍇ */
    case 2:
      ret_val = write_env(p->src_addr, (int)1, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] write_env() error(%d)\n", p->data, ret_val);
	break;
      }

      ret_val = read_env((int)&env_data, (int)2, p->usr_name, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] read_env() error(%d)\n", p->data, ret_val);
      }

      strcpy(disp_dev_name, "");
      get_another_name(disp_dev_name, p->usr_name, p->dir_name, 2);
      strcpy(disp_name, "ifc_mem + ");
      strcat(disp_name, disp_dev_name);
      e->root->pair_head->display_name = disp_name;
      break;

      /* net �t�@�C�����w�肹���� ifc, dev ���w�肷��ꍇ */
    case 3:
      init_env();
      ret_val = read_env((int)&env_data, (int)1, p->usr_name, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] read_env() error(%d)\n", p->data, ret_val);
	break;
      }

      ret_val = read_env((int)&env_data, (int)2, p->usr_name2, p->dir_name2, p->no_decode2);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] read_env() error(%d)\n", p->data, ret_val);
      }

      strcpy(disp_ifc_name, "");
      strcpy(disp_dev_name, "");
      get_another_name(disp_ifc_name, p->usr_name, p->dir_name, 1);
      get_another_name(disp_dev_name, p->usr_name2, p->dir_name2, 2);
      strcpy(disp_name, disp_ifc_name);
      strcat(disp_name, " + ");
      strcat(disp_name, disp_dev_name);
      e->root->pair_head->display_name = disp_name;
      break;

      /* inet �t�@�C�����w�肷��ꍇ */
    case 4:
#ifdef HOST_READ_ONLY
      if(strncmp(p->dir_name, "host", 4) != 0){
	ret_val = sceNetCnfSetLatestEntry(p->dir_name, (int)0, p->usr_name);
	if(ret_val < 0){
	  printf("[SET_CNF] [%d] sceNetCnfSetLatestEntry() error(%d)\n", p->data, ret_val);
	  break;
	}
      }
#else /* HOST_READ_ONLY */
      ret_val = sceNetCnfSetLatestEntry(p->dir_name, (int)0, p->usr_name);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] sceNetCnfSetLatestEntry() error(%d)\n", p->data, ret_val);
	break;
      }
#endif /* HOST_READ_ONLY */

      init_env();
      ret_val = read_env((int)&env_data, (int)0, p->usr_name, p->dir_name, p->no_decode);
      if(ret_val < 0){
	printf("[SET_CNF] [%d] read_env() error(%d)\n", p->data, ret_val);
      }
      break;
    }

    // redial_count ��ݒ肷��
    for(i = 0, redial_count = 0; i < sceNetCnf_MAX_PHONE_NUMBERS; i++){
	if(NULL == (e->root->pair_head->ifc->phone_numbers[i])) continue;
	switch(i){
	case 0: redial_count++; break;
	case 1: redial_count++; break;
	case 2: redial_count++; break;
	}
    }
    e->root->pair_head->ifc->redial_count = redial_count - 1;

#ifdef DEBUG
    dump_env(e);
#endif /* DEBUG */

    /* Auto Mode �ɐݒ� */
    sceInetCtlSetAutoMode(1);

    /* netcnf.irx ���Q�Ƃ���ݒ�f�[�^��u�������� */
    ret_val = sceInetCtlSetConfiguration(e);
    if(ret_val < 0){
      printf("[SET_CNF] sceInetCtlSetConfiguration() error(%d)\n", ret_val);
    }
    break;

  case DOWN:
    /****************/
    /* < �ڑ����� > */
    /****************/

    /* No Auto Mode �ɐݒ� */
    sceInetCtlSetAutoMode(0);

    /* �ڑ����� */
    ret_val = sceInetCtlDownInterface(0);
    if(ret_val < 0){
      printf("[DOWN] sceInetCtlDownInterface() error(%d)\n", ret_val);
    }
    break;

  case ALLOC_AREA:
    /*********************************/
    /* < Alloc ���� >                */
    /*     p->size    : �������T�C�Y */
    /*********************************/

    /* Alloc ���� */
    CpuSuspendIntr(&oldstat);
    ret_val = (u_int)AllocSysMemory(0, (u_long)(p->size), NULL);
    CpuResumeIntr(oldstat);
    if(ret_val < 0){
      printf("[ALLOC_AREA] AllocSysMemory() error(%d)\n", ret_val);
    }
    break;

  case FREE_AREA:
    /***********************************/
    /* < Free ���� >                   */
    /*     p->data    : �������A�h���X */
    /***********************************/

    /* Free ���� */
    CpuSuspendIntr(&oldstat);
    ret_val = FreeSysMemory((void *)(p->data)); 
    CpuResumeIntr(oldstat);
    if(ret_val < 0){
      printf("[FREE_AREA] FreeSysMemory() error(%d)\n", ret_val);
    }
    break;

  case GET_IF_COUNT:
    /******************************************************/
    /* < Interface List�� ���擾���� >                    */
    /*     p->data       : Mode                           */
    /*     p->device_stat: �f�o�C�X�̏��                 */
    /*     p->dir_name   : �ݒ�Ǘ��t�@�C���̃p�X��(MC)   */
    /*     p->dir_name2  : �ݒ�Ǘ��t�@�C���̃p�X��(HDD)  */
    /*     p->dir_name3  : �ݒ�Ǘ��t�@�C���̃p�X��(HOST) */
    /*     p->no_decode  : no_decode �t���O(MC)           */
    /*     p->no_decode2 : no_decode �t���O(HDD)          */
    /*     p->no_decode3 : no_decode �t���O(HOST)         */
    /******************************************************/

    /* ID ���X�g�� ID �̐����擾 */
    ret_val = get_if_list(p->data, p->device_stat, p->dir_name, p->dir_name2, p->dir_name3
			  , p->no_decode, p->no_decode2, p->no_decode3, &error_device);
    if(ret_val < 0){
      printf("[GET_IF_COUNT] get_if_list() device(%d), error(%d)\n", error_device, ret_val);
      break;
    }
    break;

  case GET_IF_LIST:
    /******************************************************/
    /* < Interface List ���擾���� >                      */
    /*     p->data     : Mode                             */
    /*     p->dst_addr : Destination Address              */
    /*     p->size     : Size                             */
    /*     p->device_stat: �f�o�C�X�̏��                 */
    /*     p->dir_name: �ݒ�Ǘ��t�@�C���̃p�X��          */
    /*     p->dir_name2  : �ݒ�Ǘ��t�@�C���̃p�X��(HDD)  */
    /*     p->dir_name3  : �ݒ�Ǘ��t�@�C���̃p�X��(HOST) */
    /*     p->no_decode  : no_decode �t���O(MC)           */
    /*     p->no_decode2 : no_decode �t���O(HDD)          */
    /*     p->no_decode3 : no_decode �t���O(HOST)         */
    /******************************************************/

    /* �C���^�t�F�[�X���X�g���擾 */
    ret_val = get_if_list(p->data, p->device_stat, p->dir_name, p->dir_name2, p->dir_name3
			  , p->no_decode, p->no_decode2, p->no_decode3, &error_device);
    if(ret_val < 0){
      printf("[GET_IF_LIST] get_if_list() device(%d), error(%d)\n", error_device, ret_val);
      break;
    }

    /* EE �փt�@�C�����X�g��]�� */
    send_ee((u_int)dev_list, (u_int)p->dst_addr, (u_int)(p->size * sizeof(sceInetInterfaceList_t)), (u_int)0);
    break;

  case GET_HWADDR:
    /**********************************************/
    /* <�n�[�h�E�F�A�A�h���X���擾����>           */
    /*     p->data: �l�b�g���[�N�C���^�t�F�[�X ID */
    /**********************************************/

    ret_val = get_hwaddr(p->data, hwaddr, sizeof(hwaddr));
    for(i = 0; i < 6; i++){
      p->hwaddr[i] = hwaddr[i];
    }
    break;

  case COPY_ENTRY:
    /**************************************************/
    /* < �w�肳�ꂽ�G���g����ʃf�o�C�X�� copy ���� > */
    /*     p->type       : �t�@�C���̎��             */
    /*     p->no_decode  : no_decode �t���O           */
    /*     p->no_decode2 : no_decode �t���O           */
    /*     p->usr_name   : �擾����t�@�C����         */
    /*     p->usr_name2  : �擾����t�@�C����         */
    /*     p->dir_name   : �ݒ�Ǘ��t�@�C���̃p�X��   */
    /*     p->dir_name2  : �ݒ�Ǘ��t�@�C���̃p�X��   */
    /**************************************************/
    /* �w�肵���t�@�C����ǂݍ��� */
    init_env();
    env.f_no_decode = p->no_decode;
    if(0 > (ret_val = sceNetCnfLoadEntry(p->dir_name, p->type, p->usr_name, &env))){
      printf("[COPY_ENTRY] %d = sceNetCnfLoadEntry(%s, %d, %s, 0x%08x)\n"
	     , ret_val, p->dir_name, p->type, p->usr_name, (int)(&env));
      break;
    }
    env.mem_ptr = (void *)(((int)env.mem_ptr + 3) & ~3);
    env.mem_base = env.mem_ptr;


    /* �ݒ�Ǘ��t�@�C���ɒǉ����A�t�@�C���ɏ����o�� */
    env.f_no_decode = p->no_decode2;
    ret_val = sceNetCnfAddEntry(p->dir_name2, p->type, p->usr_name2, &env);
    if(ret_val < 0){
      printf("[COPY_ENTRY] sceNetCnfAddEntry() error(%d)\n", ret_val);
    }
    break;
  }

  /* �Ԃ�l��ݒ� */
  p->data        = ret_val;
  p->device_stat = error_device;

  return (data);
}

/*********************************/
/* SIF DMA �� IOP ���� EE �֓]�� */
/*********************************/
void
send_ee(u_int data, u_int addr, u_int size, u_int mode)
{
  sceSifDmaData fdma;
  int id;

  fdma.data = data;
  fdma.addr = addr;
  fdma.size = (size & 0xfffffff0) + ((size % 0x10) ? 0x10 : 0);
  fdma.mode = mode;
  FlushDcache();
  CpuSuspendIntr(&oldstat);
  id = sceSifSetDma(&fdma, 1);
  CpuResumeIntr(oldstat);
  while(0 <= sceSifDmaStat(id));
}

/******************************/
/* �C���^�t�F�[�X���X�g���擾 */
/******************************/
int
get_if_list(int mode, int device_stat, char *dir_name, char *dir_name2, char *dir_name3, int no_decode, int no_decode2, int no_decode3, int *error_device)
{
  int i, j, k, flag, r, r2, r3, count, count2, device_stat_org;
  static sceNetCnfList_t *list;
  static sceNetCnfList2_t *list2;

  /* Initialize */
  device_stat_org = device_stat;
  r               = 0;
  count           = 0;
  if(error_device) *error_device = NO_SELECT;

  switch(mode){
  case 0:
    /***********************************************************************************/
    /* ���ݒ�̑��݂��Ȃ�,�����I�ɐڑ�����Ă���l�b�g���[�N�C���^�t�F�[�X���X�g���쐬 */
    /***********************************************************************************/

    /* �S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g�̐��� ID ���擾 */
    dev_count = sceInetGetInterfaceList(dev_id, MAX_LEN);
    if(dev_count < 0) return (dev_count);

    /* �S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g���擾 */
    for(i = 0; i < dev_count; i++){
      dev_list[i].select_device = NO_SELECT;
      dev_list[i].id            = dev_id[i];
      dev_list[i].mode          = mode;
      dev_list[i].flag          = 0;
      dev_list[i].type          = 0;
      r = sceInetInterfaceControl(dev_id[i], (int)sceInetCC_GetFlags, &dev_list[i].type, (int)sizeof(int));
      dev_list[i].type &= (sceInetDevF_NIC | sceInetDevF_ARP | sceInetDevF_PPP);
      if(r < 0) return (r);
      r = sceInetInterfaceControl(dev_id[i], (int)sceInetCC_GetVendorName, dev_list[i].vendor, (int)MAX_LEN);
      if(r < 0) return (r);
      r = sceInetInterfaceControl(dev_id[i], (int)sceInetCC_GetDeviceName, dev_list[i].product, (int)MAX_LEN);
      if(r < 0) return (r);
    }

    /* ���ݒ萔���擾 */
    count       = 0;
    device_stat = device_stat_org;
    while(device_stat){
      if(device_stat & DEVICE_STAT_MC_ON){
	device_stat &= DEVICE_STAT_MC_OFF;
	count2 = sceNetCnfGetCount(dir_name, 2);
	if(count2 < 0){
	  if(error_device) *error_device = SELECT_MC;
	  return (count2);
	}else{
	  count += count2;
	}
      }else{
	if(device_stat & DEVICE_STAT_HDD_ON){
	  device_stat &= DEVICE_STAT_HDD_OFF;
	  count2 = sceNetCnfGetCount(dir_name2, 2);
	  if(count2 < 0){
	    if(error_device) *error_device = SELECT_HDD;
	    return (count2);
	  }else{
	    count += count2;
	  }
	}else{
	  if(device_stat & DEVICE_STAT_HOST_ON){
	    device_stat &= DEVICE_STAT_HOST_OFF;
	    count2 = sceNetCnfGetCount(dir_name3, 2);
	    if(count2 < 0){
	      if(error_device) *error_device = SELECT_HOST;
	      return (count2);
	    }else{
	      count += count2;
	    }
	  }
	}
      }
    }

    if(count){
      /* �t�@�C�����X�g�̃��������m�� */
      CpuSuspendIntr(&oldstat);
      list  = (sceNetCnfList_t *)AllocSysMemory(0, (u_long)(count * sizeof(sceNetCnfList_t)), NULL);
      if(list == NULL){
	CpuResumeIntr(oldstat);
	return(sceNETCNF_ALLOC_ERROR);
      }
      list2 = (sceNetCnfList2_t *)AllocSysMemory(0, (u_long)(count * sizeof(sceNetCnfList2_t)), NULL);
      if(list2 == NULL){
	if(list) FreeSysMemory((void *)list);
	CpuResumeIntr(oldstat);
	return(sceNETCNF_ALLOC_ERROR);
      }
      CpuResumeIntr(oldstat);

      /* �t�@�C�����X�g���擾 */
      r           = 0;
      device_stat = device_stat_org;
      while(device_stat){
	if(device_stat & DEVICE_STAT_MC_ON){
	  device_stat &= DEVICE_STAT_MC_OFF;
	  r2 = sceNetCnfGetList(dir_name, 2, (list + r));
	  if(r2 < 0){
	    if(error_device) *error_device = SELECT_MC;
	    CpuSuspendIntr(&oldstat);
	    FreeSysMemory((void *)list); 
	    FreeSysMemory((void *)list2); 
	    CpuResumeIntr(oldstat);
	    return (r2);
	  }else{
	    r3 = check_device(dir_name, r2, 2, (list + r), (list2 + r), no_decode, SELECT_MC);
	    if(r3 < 0){
	      if(error_device) *error_device = SELECT_MC;
	      CpuSuspendIntr(&oldstat);
	      FreeSysMemory((void *)list); 
	      FreeSysMemory((void *)list2); 
	      CpuResumeIntr(oldstat);
	      return (r3);
	    }
	    r += r2;
	  }
	}else{
	  if(device_stat & DEVICE_STAT_HDD_ON){
	    device_stat &= DEVICE_STAT_HDD_OFF;
	    r2 = sceNetCnfGetList(dir_name2, 2, (list + r));
	    if(r2 < 0){
	      if(error_device) *error_device = SELECT_HDD;
	      CpuSuspendIntr(&oldstat);
	      FreeSysMemory((void *)list); 
	      FreeSysMemory((void *)list2); 
	      CpuResumeIntr(oldstat);
	      return (r2);
	    }else{
	      r3 = check_device(dir_name2, r2, 2, (list + r), (list2 + r), no_decode2, SELECT_HDD);
	      if(r3 < 0){
		if(error_device) *error_device = SELECT_HDD;
		CpuSuspendIntr(&oldstat);
		FreeSysMemory((void *)list); 
		FreeSysMemory((void *)list2); 
		CpuResumeIntr(oldstat);
		return (r3);
	      }
	      r += r2;
	    }
	  }else{
	    if(device_stat & DEVICE_STAT_HOST_ON){
	      device_stat &= DEVICE_STAT_HOST_OFF;
	      r2 = sceNetCnfGetList(dir_name3, 2, (list + r));
	      if(r2 < 0){
		if(error_device) *error_device = SELECT_HOST;
		CpuSuspendIntr(&oldstat);
		FreeSysMemory((void *)list); 
		FreeSysMemory((void *)list2); 
		CpuResumeIntr(oldstat);
		return (r2);
	      }else{
		r3 = check_device(dir_name3, r2, 2, (list + r), (list2 + r), no_decode3, SELECT_HOST);
		if(r3 < 0){
		  if(error_device) *error_device = SELECT_HOST;
		  CpuSuspendIntr(&oldstat);
		  FreeSysMemory((void *)list); 
		  FreeSysMemory((void *)list2); 
		  CpuResumeIntr(oldstat);
		  return (r3);
		}
		r += r2;
	      }
	    }
	  }
	}
      }

      /* vendor, product, type �� check */
      for(i = 0; i < count; i++){
	init_data(&env_data);
	init_env();
	switch((list2 + i)->select_device){
	case SELECT_MC:
	  r = read_env((int)&env_data, 2, (list2 + i)->usr_name, dir_name, no_decode);
	  if(r < 0){
	    if(error_device) *error_device = SELECT_MC;
	    return (r);
	  }
	  break;
	case SELECT_HDD:
	  r = read_env((int)&env_data, 2, (list2 + i)->usr_name, dir_name2, no_decode2);
	  if(r < 0){
	    if(error_device) *error_device = SELECT_HDD;
	    return (r);
	  }
	  break;
	case SELECT_HOST:
	  r = read_env((int)&env_data, 2, (list2 + i)->usr_name, dir_name3, no_decode3);
	  if(r < 0){
	    if(error_device) *error_device = SELECT_HOST;
	    return (r);
	  }
	  break;
	}
	if(env_data.vendor[0] != '\0' && env_data.product[0] != '\0'){
	  dev_list[dev_count].id   = -1;                         /* �l�b�g���[�N�C���^�t�F�[�X ID */
	  dev_list[dev_count].type = 0;                          /* �l�b�g���[�N�C���^�t�F�[�X�^�C�v */
	  switch(env_data.type){
	  case 3: /* nic */
	    dev_list[dev_count].type |= sceInetDevF_NIC;
	  case 1: /* eth */
	    dev_list[dev_count].type |= sceInetDevF_ARP;
	    break;
	  case 2: /* ppp */
	    dev_list[dev_count].type |= sceInetDevF_PPP;
	    break;
	  }
	  dev_list[dev_count].mode = mode;                       /* ���X�g���[�h(1: ���ݒ�I/F+���ݒ�I/F, 0: ���ݒ�I/F) */
	  dev_list[dev_count].flag = 1;                          /* �ݒ�σt���O(1: �ݒ��, 0: ���ݒ�) */
	  strcpy(dev_list[dev_count].vendor, env_data.vendor);   /* �x���_�� */
	  strcpy(dev_list[dev_count].product, env_data.product); /* �v���_�N�g�� */
	}else{
	  continue;
	}

	for(j = 0; j < dev_count; ){
	  if(strcmp(dev_list[j].vendor, dev_list[dev_count].vendor) == 0 &&
	     strcmp(dev_list[j].product, dev_list[dev_count].product) == 0){
	    /* ���ݒ肪���݂���Ȃ炻�̃��X�g�͍폜 */
	    for(k = j; k < dev_count; k++){
	      dev_list[k].id   = dev_list[k + 1].id;
	      dev_list[k].type = dev_list[k + 1].type;
	      dev_list[k].mode = dev_list[k + 1].mode;
	      dev_list[k].flag = dev_list[k + 1].flag;
	      strcpy(dev_list[k].vendor, dev_list[k + 1].vendor);
	      strcpy(dev_list[k].product, dev_list[k + 1].product);
	    }
	    dev_count--;
	  }else{
	    j++;
	  }
	}
      }

      /* �t�@�C�����X�g�̃���������� */
      CpuSuspendIntr(&oldstat);
      FreeSysMemory((void *)list); 
      FreeSysMemory((void *)list2); 
      CpuResumeIntr(oldstat);
    }
    break;

  case 1:
    /*****************************************************************************************/
    /* ���ݒ� + �ݒ�̑��݂��Ȃ������I�ɐڑ�����Ă���l�b�g���[�N�C���^�t�F�[�X���X�g���쐬 */
    /*****************************************************************************************/

    /* ���ݒ萔���擾 */
    count       = 0;
    device_stat = device_stat_org;
    while(device_stat){
      if(device_stat & DEVICE_STAT_MC_ON){
	device_stat &= DEVICE_STAT_MC_OFF;
	count2 = sceNetCnfGetCount(dir_name, 2);
	if(count2 < 0){
	  if(error_device) *error_device = SELECT_MC;
	  return (count2);
	}else{
	  count += count2;
	}
      }else{
	if(device_stat & DEVICE_STAT_HDD_ON){
	  device_stat &= DEVICE_STAT_HDD_OFF;
	  count2 = sceNetCnfGetCount(dir_name2, 2);
	  if(count2 < 0){
	    if(error_device) *error_device = SELECT_HDD;
	    return (count2);
	  }else{
	    count += count2;
	  }
	}else{
	  if(device_stat & DEVICE_STAT_HOST_ON){
	    device_stat &= DEVICE_STAT_HOST_OFF;
	    count2 = sceNetCnfGetCount(dir_name3, 2);
	    if(count2 < 0){
	      if(error_device) *error_device = SELECT_HOST;
	      return (count2);
	    }else{
	      count += count2;
	    }
	  }
	}
      }
    }

    if(count){
      /* �t�@�C�����X�g�̃��������m�� */
      CpuSuspendIntr(&oldstat);
      list  = (sceNetCnfList_t *)AllocSysMemory(0, (u_long)(count * sizeof(sceNetCnfList_t)), NULL);
      if(list == NULL){
	CpuResumeIntr(oldstat);
	return(sceNETCNF_ALLOC_ERROR);
      }
      list2 = (sceNetCnfList2_t *)AllocSysMemory(0, (u_long)(count * sizeof(sceNetCnfList2_t)), NULL);
      if(list2 == NULL){
	if(list) FreeSysMemory((void *)list);
	CpuResumeIntr(oldstat);
	return(sceNETCNF_ALLOC_ERROR);
      }
      CpuResumeIntr(oldstat);

      /* �t�@�C�����X�g���擾 */
      r           = 0;
      device_stat = device_stat_org;
      while(device_stat){
	if(device_stat & DEVICE_STAT_MC_ON){
	  device_stat &= DEVICE_STAT_MC_OFF;
	  r2 = sceNetCnfGetList(dir_name, 2, (list + r));
	  if(r2 < 0){
	    if(error_device) *error_device = SELECT_MC;
	    CpuSuspendIntr(&oldstat);
	    FreeSysMemory((void *)list); 
	    FreeSysMemory((void *)list2); 
	    CpuResumeIntr(oldstat);
	    return (r2);
	  }else{
	    r3 = check_device(dir_name, r2, 2, (list + r), (list2 + r), no_decode, SELECT_MC);
	    if(r3 < 0){
	      if(error_device) *error_device = SELECT_MC;
	      CpuSuspendIntr(&oldstat);
	      FreeSysMemory((void *)list); 
	      FreeSysMemory((void *)list2); 
	      CpuResumeIntr(oldstat);
	      return (r3);
	    }
	    r += r2;
	  }
	}else{
	  if(device_stat & DEVICE_STAT_HDD_ON){
	    device_stat &= DEVICE_STAT_HDD_OFF;
	    r2 = sceNetCnfGetList(dir_name2, 2, (list + r));
	    if(r2 < 0){
	      if(error_device) *error_device = SELECT_HDD;
	      CpuSuspendIntr(&oldstat);
	      FreeSysMemory((void *)list); 
	      FreeSysMemory((void *)list2); 
	      CpuResumeIntr(oldstat);
	      return (r2);
	    }else{
	      r3 = check_device(dir_name2, r2, 2, (list + r), (list2 + r), no_decode2, SELECT_HDD);
	      if(r3 < 0){
		if(error_device) *error_device = SELECT_HDD;
		CpuSuspendIntr(&oldstat);
		FreeSysMemory((void *)list); 
		FreeSysMemory((void *)list2); 
		CpuResumeIntr(oldstat);
		return (r3);
	      }
	      r += r2;
	    }
	  }else{
	    if(device_stat & DEVICE_STAT_HOST_ON){
	      device_stat &= DEVICE_STAT_HOST_OFF;
	      r2 = sceNetCnfGetList(dir_name3, 2, (list + r));
	      if(r2 < 0){
		if(error_device) *error_device = SELECT_HOST;
		CpuSuspendIntr(&oldstat);
		FreeSysMemory((void *)list); 
		FreeSysMemory((void *)list2); 
		CpuResumeIntr(oldstat);
		return (r2);
	      }else{
		r3 = check_device(dir_name3, r2, 2, (list + r), (list2 + r), no_decode3, SELECT_HOST);
		if(r3 < 0){
		  if(error_device) *error_device = SELECT_HOST;
		  CpuSuspendIntr(&oldstat);
		  FreeSysMemory((void *)list); 
		  FreeSysMemory((void *)list2); 
		  CpuResumeIntr(oldstat);
		  return (r3);
		}
		r += r2;
	      }
	    }
	  }
	}
      }

      /* vendor, product �� if_list �� copy */
      for(i = 0, j = 0, flag = 0; i < count; i++){
	init_data(&env_data);
	init_env();
	switch((list2 + i)->select_device){
	case SELECT_MC:
	  r = read_env((int)&env_data, 2, (list2 + i)->usr_name, dir_name, no_decode);
	  if(r < 0){
	    if(error_device) *error_device = SELECT_MC;
	    return (r);
	  }
	  break;
	case SELECT_HDD:
	  r = read_env((int)&env_data, 2, (list2 + i)->usr_name, dir_name2, no_decode2);
	  if(r < 0){
	    if(error_device) *error_device = SELECT_HDD;
	    return (r);
	  }
	  break;
	case SELECT_HOST:
	  r = read_env((int)&env_data, 2, (list2 + i)->usr_name, dir_name3, no_decode3);
	  if(r < 0){
	    if(error_device) *error_device = SELECT_HOST;
	    return (r);
	  }
	  break;
	}
	if(env_data.vendor[0] != '\0' && env_data.product[0] != '\0'){
	  dev_list[j].select_device = (list2 + i)->select_device; /* �I������Ă���L���f�o�C�X */
	  dev_list[j].id            = -1;                         /* �l�b�g���[�N�C���^�t�F�[�X ID */
	  dev_list[j].type          = 0;                          /* �l�b�g���[�N�C���^�t�F�[�X�^�C�v */
	  switch(env_data.type){
	  case 3: /* nic */
	    dev_list[j].type |= sceInetDevF_NIC;
	  case 1: /* eth */
	    dev_list[j].type |= sceInetDevF_ARP;
	    break;
	  case 2: /* ppp */
	    dev_list[j].type |= sceInetDevF_PPP;
	    break;
	  }
	  dev_list[j].mode = mode;                                /* ���X�g���[�h(1: ���ݒ�I/F+���ݒ�I/F, 0: ���ݒ�I/F) */
	  dev_list[j].flag = 1;                                   /* �ݒ�σt���O(1: �ݒ��, 0: ���ݒ�) */
	  strcpy(dev_list[j].vendor, env_data.vendor);            /* �x���_�� */
	  strcpy(dev_list[j].product, env_data.product);          /* �v���_�N�g�� */
	  j++;
	}else{
	  flag++;
	}
      }
      if(flag) count -= flag;

      /* �t�@�C�����X�g�̃���������� */
      CpuSuspendIntr(&oldstat);
      FreeSysMemory((void *)list); 
      FreeSysMemory((void *)list2); 
      CpuResumeIntr(oldstat);
    }

    /* �S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g�̐��� ID ���擾 */
    dev_count = sceInetGetInterfaceList(dev_id, MAX_LEN);
    if(dev_count < 0) return (dev_count);

    /* �S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g�Ɗ��ݒ���`�F�b�N */
    for(i = 0; i < dev_count; i++){
      dev_list[count].select_device = NO_SELECT;
      dev_list[count].id            = dev_id[i];
      dev_list[count].mode          = mode;
      dev_list[count].flag          = 0;
      dev_list[count].type          = 0;
      r = sceInetInterfaceControl(dev_id[i], (int)sceInetCC_GetFlags, &dev_list[count].type, (int)sizeof(int));
      dev_list[count].type &= (sceInetDevF_NIC | sceInetDevF_ARP | sceInetDevF_PPP);
      if(r < 0) return (r);
      r = sceInetInterfaceControl(dev_id[i], (int)sceInetCC_GetVendorName, dev_list[count].vendor, (int)MAX_LEN);
      if(r < 0) return (r);
      r = sceInetInterfaceControl(dev_id[i], (int)sceInetCC_GetDeviceName, dev_list[count].product, (int)MAX_LEN);
      if(r < 0) return (r);

      /* flag, vendor, product �����S��v���Ȃ��ꍇ���X�g��ǉ� */
      for(j = 0, flag = 0; j < count; j++){
	if(strcmp(dev_list[j].vendor, dev_list[count].vendor) == 0 &&
	   strcmp(dev_list[j].product, dev_list[count].product) == 0 &&
	   dev_list[j].flag == 1){
	  dev_list[j].id = dev_list[count].id;
	  flag = 1;
	}
      }
      if(flag == 0) count++;
    }

    /* count �� dev_count �ɔ��f������ */
    dev_count = count;
    break;

  case 2:
    /*******************************************************************************************/
    /* �ݒ�̑��݂Ɋւ�炸,�����I�ɐڑ�����Ă���S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g���쐬 */
    /*******************************************************************************************/

    /* �S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g�̐��� ID ���擾 */
    dev_count2 = sceInetGetInterfaceList(dev_id2, MAX_LEN);
    if(dev_count2 < 0) return (dev_count2);

    /* �S�Ẵl�b�g���[�N�C���^�t�F�[�X���X�g���擾 */
    for(i = 0; i < dev_count2; i++){
      dev_list2[i].select_device = NO_SELECT;
      dev_list2[i].id            = dev_id2[i];
      dev_list2[i].mode          = mode;
      dev_list2[i].flag          = 2;
      dev_list2[i].type          = 0;
      r = sceInetInterfaceControl(dev_id2[i], (int)sceInetCC_GetFlags, &dev_list2[i].type, (int)sizeof(int));
      dev_list2[i].type &= (sceInetDevF_NIC | sceInetDevF_ARP | sceInetDevF_PPP);
      if(r < 0) return (r);
      r = sceInetInterfaceControl(dev_id2[i], (int)sceInetCC_GetVendorName, dev_list2[i].vendor, (int)MAX_LEN);
      if(r < 0) return (r);
      r = sceInetInterfaceControl(dev_id2[i], (int)sceInetCC_GetDeviceName, dev_list2[i].product, (int)MAX_LEN);
      if(r < 0) return (r);
    }
    break;
  }

  return (( ( mode != 2 ) ? dev_count : dev_count2 ));
}

/******************************************/
/* ���f���ݒ�t�@�C���̃f�o�C�X���`�F�b�N */
/******************************************/
int
check_device(char *fname, int size, int type, sceNetCnfList_t *list, sceNetCnfList2_t *list2, int no_decode, int select_device)
{
  int i, j, fd, flag, r = 0;
  char usr_name[MAX_LEN];

  if(type != 1){
    /* vendor �� product ���擾 */
    get_if_list(2, -1, NULL, NULL, NULL, -1, -1, -1, NULL);
  }

  for(i = 0; i < size; i++){
    /* initialize */
    (list2 + i)->select_device = select_device;
    (list2 + i)->flag          = 1;
    (list2 + i)->type          = (list + i)->type;
    (list2 + i)->stat          = (list + i)->stat;
    strcpy((list2 + i)->sys_name, (list + i)->sys_name);
    strcpy((list2 + i)->usr_name, (list + i)->usr_name);

    /* vendor, product ���擾 */
    switch(type){
    case 0:
      init_data(&env_data);
      init_env();
      r = read_env((int)&env_data, (int)type, (list + i)->usr_name, fname, no_decode);
      if(r < 0) return (r);

      /* ifc �t�@�C���̑��݃`�F�b�N */
      if(0 > (fd = open(make_path(fname, env_data.attach_ifc), O_RDONLY))){
	(list2 + i)->flag = 0;
      }else{
	close(fd);

	strcpy(usr_name, "");
	r = get_another_name(env_data.attach_ifc, usr_name, fname, 1);
	if(r < 0) return (r);

	init_env();
	r = read_env((int)&env_data, (int)1, usr_name, fname, no_decode);
	if(r < 0) return (r);
      }

      /* dev �t�@�C���̑��݃`�F�b�N */
      if(0 > (fd = open(make_path(fname, env_data.attach_dev), O_RDONLY))){
	(list2 + i)->flag = 0;
      }else{
	close(fd);

	strcpy(usr_name, "");
	r = get_another_name(env_data.attach_dev, usr_name, fname, 2);
	if(r < 0) return (r);

	init_env();
	r = read_env((int)&env_data, (int)2, usr_name, fname, no_decode);
	if(r < 0) return (r);
      }
      break;

    case 1:
      init_data(&env_data);
      init_env();
      r = read_env((int)&env_data, (int)type, (list + i)->usr_name, fname, no_decode);
      if(r < 0) return (r);
      break;

    case 2:
      init_data(&env_data);
      init_env();
      r = read_env((int)&env_data, (int)type, (list + i)->usr_name, fname, no_decode);
      if(r < 0) return (r);
      break;
    }

    if(type != 1 && (list2 + i)->flag){
      /* vendor, product �����݂��邩�ǂ����`�F�b�N */
      flag = 0;
      if(env_data.vendor[0] == '\0'){
	flag++;
      }else{
	for(j = 0; j < dev_count2; j++){
	  if(strcmp(env_data.vendor, dev_list2[j].vendor) == 0){
	    flag++;
	    break;
	  }
	}
      }
      if(env_data.product[0] == '\0'){
	flag++;
      }else{
	for(j = 0; j < dev_count2; j++){
	  if(strcmp(env_data.product, dev_list2[j].product) == 0){
	    flag++;
	    break;
	  }
	}
      }
      if(flag != 2) (list2 + i)->flag = 0;
    }
  }

  return (r);
}

/*****************************************************/
/* usr_name(sys_name) ���� sys_name(usr_name) ���擾 */
/*****************************************************/
int
get_another_name(char *sys_name, char *usr_name, char *fname, int type)
{
  int ret_val, i;
  sceNetCnfList_t *p;

  /* �擾�����ނ̃t�@�C���̐����擾 */
  ret_val = sceNetCnfGetCount(fname, type);
  if(ret_val < 0) return (ret_val);

  /* �t�@�C�����X�g�̃��������m�� */
  CpuSuspendIntr(&oldstat);
  p  = (sceNetCnfList_t *)AllocSysMemory(0, (u_long)(ret_val * sizeof(sceNetCnfList_t)), NULL);
  if(p == NULL){
    CpuResumeIntr(oldstat);
    ret_val = sceNETCNF_ALLOC_ERROR;
    return (ret_val);
  }
  CpuResumeIntr(oldstat);

  /* �t�@�C�����X�g���擾 */
  ret_val = sceNetCnfGetList(fname, type, p);
  if(ret_val < 0){
    CpuSuspendIntr(&oldstat);
    FreeSysMemory((void *)p); 
    CpuResumeIntr(oldstat);
    return (ret_val);
  }

  /* usr_name(sys_name) ���� sys_name(usr_name) ���擾 */
  for(i = 0; i < ret_val; i++){
    if(strcmp(usr_name, (p + i)->usr_name) == 0){
      strcpy(sys_name, (p + i)->sys_name);
      break;
    }
    if(strcmp(sys_name, (p + i)->sys_name) == 0){
      strcpy(usr_name, (p + i)->usr_name);
      break;
    }
  }

  /* �t�@�C�����X�g�̃���������� */
  CpuSuspendIntr(&oldstat);
  FreeSysMemory((void *)p); 
  CpuResumeIntr(oldstat);

  return (ret_val);
}

/******************************/
/* sceNetCnfEnvData �̏����� */
/******************************/
void
init_data(sceNetCnfEnvData_t *data)
{
  strcpy(data->attach_ifc, "");
  strcpy(data->attach_dev, "");
  strcpy(data->address, "");
  strcpy(data->netmask, "");
  strcpy(data->gateway, "");
  strcpy(data->dns1_address, "");
  strcpy(data->dns2_address, "");
  strcpy(data->phone_numbers1, "");
  strcpy(data->phone_numbers2, "");
  strcpy(data->phone_numbers3, "");
  strcpy(data->auth_name, "");
  strcpy(data->auth_key, "");
  strcpy(data->vendor, "");
  strcpy(data->product, "");
  strcpy(data->chat_additional, "");
  strcpy(data->outside_set, "");
  strcpy(data->dhcp_host_name, "");
  data->dialing_type = 0;
  data->idle_timeout = 10;
  data->type = 0;
  data->phy_config = 0;
  data->dhcp = 0;
  data->pppoe = 0;
}

/*******************/
/* PATH ���쐬���� */
/*******************/
char *
make_path(char *dir_name, char *arg_fname)
{
  int n, flag;
  char *p;
  static char tmp[MAX_LEN];

  p    = arg_fname;
  flag = 1;
  while(*p != '\0'){
    if(*p == ':') flag = 0;
    p++;
  }

  if(flag){
    strcpy(tmp, dir_name);
    n = strlen(tmp);

    n--;
    while(tmp[n] != '/' && tmp[n] != '\\'){
      n--;
    }
    tmp[n + 1] = '\0';
  }else{
    strcpy(tmp, "");
  }

  return (strcat(tmp, arg_fname));
}

/*****************************/
/* sceNetCnfEnv ������������ */
/*****************************/
void
init_env(void)
{
  static sceNetCnfEnv_t *e = &env;

  bzero(e, sizeof(*e));
  e->mem_last  = (e->mem_base = e->mem_ptr = mem_area) + sizeof(mem_area);
}

/**********************************/
/* �n�[�h�E�F�A�A�h���X���擾���� */
/**********************************/
int
get_hwaddr(int id, u_char *hwaddr, int size)
{
  int r;

  r = sceInetInterfaceControl(id, sceInetCC_GetHWaddr, hwaddr, size);
#ifdef DEBUG
  printf("HWaddr:");
  printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
	 hwaddr[0], hwaddr[1], hwaddr[2],
	 hwaddr[3], hwaddr[4], hwaddr[5]);
#endif /* DEBUG */

  return (r);
}

#ifdef DEBUG
/**********************/
/* �f�[�^���_���v���� */
/**********************/
void
dump_data(sceNetCnfEnvData_t *data)
{
  printf("attach_ifc       : %s\n", data->attach_ifc);
  printf("attach_dev       : %s\n", data->attach_dev);
  printf("address          : %s\n", data->address);
  printf("netmask          : %s\n", data->netmask);
  printf("gateway          : %s\n", data->gateway);
  printf("dns1_address     : %s\n", data->dns1_address);
  printf("dns2_address     : %s\n", data->dns2_address);
  printf("phone_numbers1   : %s\n", data->phone_numbers1);
  printf("phone_numbers2   : %s\n", data->phone_numbers2);
  printf("phone_numbers3   : %s\n", data->phone_numbers3);
  printf("auth_name        : %s\n", data->auth_name);
  printf("auth_key         : %s\n", data->auth_key);
  printf("vendor           : %s\n", data->vendor);
  printf("product          : %s\n", data->product);
  printf("chat_additional  : %s\n", data->chat_additional);
  printf("outside_set      : %s\n", data->outside_set);
  printf("dhcp_host_name   : %s\n", data->dhcp_host_name);
  printf("dialing_type     : %d\n", data->dialing_type);
  printf("idle_timeout     : %d\n", data->idle_timeout);
  printf("type             : %d\n", data->type);
  printf("phy_config       : %d\n", data->phy_config);
  printf("dhcp             : %d\n", data->dhcp);
  printf("pppoe            : %d\n", data->pppoe);
}

/*****************************/
/* sceNetCnfEnv ���_���v���� */
/*****************************/
void
dump_env(sceNetCnfEnv_t *e)
{
    int i, dns_no;
    char str_tmp[MAX_LEN];
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

/*******************************/
/* sceNetCnfList2 ���_���v���� */
/*******************************/
void
list_dump(sceNetCnfList2_t *list, int max)
{
  int i;

  for(i = 0; i < max; i++){
    printf("-----\n");
    printf("flag     : %d\n", (list + i)->flag);
    printf("type     : %d\n", (list + i)->type);
    printf("stat     : %d\n", (list + i)->stat);
    printf("sys_name : %s\n", (list + i)->sys_name);
    printf("usr_name : %s\n", (list + i)->usr_name);
  }
}
#endif /* DEBUG */
