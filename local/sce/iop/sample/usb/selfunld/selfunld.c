/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *      USB Self-Unload Sample (Mouse)
 *
 *                          Version 0.10
 *                          Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                           selfunld.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *      0.10            July,5,2001     fukunaga   Initial
 *      0.11            Dec,1,2001      fukunaga   for Release2.4.3
 */


#include <stdio.h>
#include <kernel.h>
#include <memory.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>

#include <usb.h>
#include <usbd.h>

#include <usbmload.h>

#include "selfunld.h"

ModuleInfo Module = {"SCE_USB_SelfUnload_sample", 0x0100 };

/*********************************************************
                           Type 
**********************************************************/
typedef struct _unit {
	int dev_id;
        int number;
        int c_pipe;
        int d_pipe;
        int payload;
        int ifnum;
        int as;
        int tcount; /* �ڑ�����Ă���s�����f�[�^�]���̑��� */
  
        /* for interrupt transfer */
	u_char data[0];  
} UNIT;

typedef struct {
        int next_number;
	int n; /* the number of Usb mouse connecting */
	UNIT* con_unit[MAX_MOUSE_NUM];
} USBMOUSE_t;


/*********************************************************
                     Common Variables
**********************************************************/
static USBMOUSE_t USBMOUSE;

#define SSIZE 0x20

static int resident_flag;

#define NORMAL_MODE   0
#define AUTOLOAD_MODE 1
static int load_mode = NORMAL_MODE;

#define SELF_UNLOAD_OFF 0
#define SELF_UNLOAD_ON  1
static int self_unload = SELF_UNLOAD_OFF;

static int special_thid;


/*********************************************************
                         Macros
**********************************************************/
#define err(p, f, r)	if(r) printf("usbmouse%d: %s -> 0x%x\n", \
				(p)->number, (f), (r))


/*********************************************************
                         Prototype
**********************************************************/
static void special_thread();
static int usbmouse_probe(int dev_id);
static int usbmouse_attach(int dev_id);
static UNIT *unit_alloc(int dev_id, int payload, int ifnum, int as);
static void unit_free(UNIT *p);
static void set_config_done(int result, int count, void *arg);
static void set_interface_done(int result, int count, void *arg);
static void data_transfer(UNIT *unit);
static void data_transfer_done(int result, int count, void *arg);
static int usbmouse_detach(int dev_id);
static int stop_usbmouse(int argc, char* argv[]);


/*********************************************************
                      Program Start
**********************************************************/
static sceUsbdLddOps usbmouse_ops = {
	NULL, NULL,
	"usbmouse",
	usbmouse_probe,
	usbmouse_attach,
	usbmouse_detach,
};

#define BASE_priority  50


/* ---------------------------------------------
  Function Name	: start
  function     	: ���C���֐�
  Input Data	: none
  Output Data   : none
  Return Value	: REMOVABLE_RESIDENT_END(Unload�\�풓�I��),
                  NO_RESIDENT_END(��풓�I��)
----------------------------------------------*/
int start(int argc, char* argv[]){
	int r;
	struct ThreadParam param;
	int i,j;

	if (argc < 0) {
	  if( strcmp(argv[0],"other") == 0 ) {
	    /* ���̃��W���[���ɂ�� StopModule() ���ꂽ�P�[�X�B
	     * ���W���[���̍폜�O�ɕK�v�ȏI�������Ȃǂ��s���B
	     */
	    /* SPECIAL�X���b�h�̍폜 */
	    TerminateThread(special_thid);
	    DeleteThread(special_thid);
	    return stop_usbmouse(argc,argv);
	  }
	  if( strcmp(argv[0],"self") == 0 ) {
	    /* �����W���[���ɂ�� SelfStopModule() ���ꂽ�P�[�X�B
	     * ���W���[���̍폜�O�ɕK�v�ȏI�������Ȃǂ��s���B
	     * �������ASelfStopModule()/SelfUnloadModule() �����s����
	     * �X���b�h�����͍폜���Ȃ��B
	     */
	    return stop_usbmouse(argc,argv);
	  }	  
	}
	
	/* Init common variables */
	USBMOUSE.next_number = 0;
	USBMOUSE.n = 0;
	for(i=0;i<MAX_MOUSE_NUM;i++) {
	  USBMOUSE.con_unit[i] = NULL;
	}
	
#if 0  /*** for debug ***/
	for(i=0; i<argc; i++) {
	  printf("argv[%d]:%s\n",i,argv[i]);
	}
#endif
	
	load_mode = NORMAL_MODE;
	load_mode = SELF_UNLOAD_OFF;
	
	for(i=0; i<argc; i++) {
	  for(j=0; argv[i][j]!='\0'; j++) {
	    if (argv[i][j] == '=') { argv[i][j]='\0'; j++; break; }
	  }
	  if (strcmp(argv[i],"lmode") == 0) {
	    if (strcmp(argv[i]+j,"AUTOLOAD") == 0) 
	      { load_mode = AUTOLOAD_MODE; break; }
	  }
	  if (strcmp(argv[i],"selfunload") == 0) {
	    if (strcmp(argv[i]+j,"ON") == 0) 
	      { self_unload = SELF_UNLOAD_ON; break; }
	  }
	}
	switch(load_mode) 
	  {
	  case NORMAL_MODE:
	    resident_flag = 1; /* �K���풓����(�ʏ펞�̎g�p) */
	    break;
	  case AUTOLOAD_MODE:
	    printf("usbmouse : AUTOLOAD MODE\n");
	    resident_flag = 0; /* probe�֐����t���O�𗧂Ă��Ƃ��̂ݏ풓 */
	    break;
	  }
	
	if(0 != (r = sceUsbdRegisterLdd(&usbmouse_ops))) {
	  /* ������,usbmouse_probe�֐����Ă΂�� */
	  printf("usbmouse: sceUsbdRegisterLdd -> 0x%x\n", r);
	}
	
	if ( ! resident_flag ) {
	  sceUsbdUnregisterLdd(&usbmouse_ops);
	  return NO_RESIDENT_END;
	}
	
	/* Special thread */
	if (self_unload == SELF_UNLOAD_ON) {
	  param.attr         = TH_C;
	  param.entry        = special_thread;
	  param.initPriority = BASE_priority;
	  param.stackSize    = 0x400;
	  param.option       = 0;
	  special_thid = CreateThread(&param);
	  if (special_thid > 0) {
	    StartThread(special_thid,0);
	  } else {
	    sceUsbdUnregisterLdd(&usbmouse_ops);
	    return NO_RESIDENT_END;
	  }
	}
	
	return REMOVABLE_RESIDENT_END;
}


/* ---------------------------------------------
  Function Name	: special_thread
  function     	: ���ȏ��ŗp�X���b�h
  Input Data	: none
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void special_thread()
{
  int result;
  
  SleepThread();

  SelfStopModule(0, NULL, &result);
  SelfUnloadModule(); /* Unload & ExitDeleteThread */
  /* Never returns here */
}

/* ---------------------------------------------
  Function Name	: usbmouse_probe
  function     	: LDD���o�����֐�
  Input Data	: dev_id(�f�o�C�XID)
  Output Data   : none
  Return Value	: 0(���o�����f�o�C�X�͂���LDD�ɑΉ����Ȃ�)
                  1(���o�����f�o�C�X�͂���LDD�ɑΉ�)
----------------------------------------------*/
static int usbmouse_probe(int dev_id){
	UsbDeviceDescriptor *ddesc;
	UsbInterfaceDescriptor *idesc;

	printf("usbmouse : probe\n");
	
	if (USBMOUSE.n >= MAX_MOUSE_NUM) {
	  return(0);
	}
	
	if(NULL == (ddesc = sceUsbdScanStaticDescriptor(dev_id, NULL,
			USB_DESCRIPTOR_TYPE_DEVICE)))
		return(0);
	if(ddesc->bDeviceClass != 0 || ddesc->bNumConfigurations != 1)
		return(0);
	
	idesc = (UsbInterfaceDescriptor *)ddesc;
	if(NULL == (idesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
				       USB_DESCRIPTOR_TYPE_INTERFACE)))
	  { return(0); }
	if( (idesc->bInterfaceClass    != 3) ||
	    (idesc->bInterfaceSubClass != 1) || 
	    (idesc->bInterfaceProtocol != 2) )
	  { return(0); }

	printf("usbmouse : resident_flag is ON\n");
	resident_flag = 1;

	return(1);
}

/* ---------------------------------------------
  Function Name	: usbmouse_attach
  function     	: LDD�ڑ������֐�
                  (usbmouse_probe�̖߂�l��1�̂Ƃ��R�[�������)
  Input Data	: dev_id(�f�o�C�XID)
  Output Data   : none
  Return Value	: 0(�R���t�B�O���[�V��������I��)
                  -1(�R���t�B�O���[�V�������s)
----------------------------------------------*/
static int usbmouse_attach(int dev_id){
	UsbConfigurationDescriptor *cdesc;
	UsbInterfaceDescriptor *idesc;
	UsbEndpointDescriptor *edesc;
	UNIT *unit;
	int payload, r;
	u_char locs[7];

	if(NULL == (cdesc = sceUsbdScanStaticDescriptor(dev_id, NULL,
			USB_DESCRIPTOR_TYPE_CONFIGURATION)))
		return(-1);
	if(cdesc->bNumInterfaces != 1)
		return(-1);

	idesc = (UsbInterfaceDescriptor *)cdesc;
	if(NULL == (idesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
					  USB_DESCRIPTOR_TYPE_INTERFACE)))
	  { return(-1); }
	if( (idesc->bInterfaceClass    != 3) ||
	    (idesc->bInterfaceSubClass != 1) ||
	    (idesc->bInterfaceProtocol != 2) )
	  { return(-1); }

	if(idesc->bNumEndpoints != 1)
		return(-1);

	if(NULL == (edesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
			USB_DESCRIPTOR_TYPE_ENDPOINT)))
		return(-1);
	if((edesc->bEndpointAddress & USB_ENDPOINT_DIRECTION_BITS)
			!= USB_ENDPOINT_DIRECTION_IN)
		return(-1);
	if((edesc->bmAttribute & USB_ENDPOINT_TRANSFER_TYPE_BITS)
			!= USB_ENDPOINT_TRANSFER_TYPE_INTERRUPT)
		return(-1);
	payload = edesc->wMaxPacketSize0 | (edesc->wMaxPacketSize1 << 8);

	if(NULL == (unit = unit_alloc(dev_id, payload, idesc->bInterfaceNumber,
			idesc->bAlternateSetting)))
		return(-1);
	if(0 > (unit->c_pipe = sceUsbdOpenPipe(dev_id, NULL)))
		return(-1);
	if(0 > (unit->d_pipe = sceUsbdOpenPipe(dev_id, edesc)))
		return(unit_free(unit), -1);
	sceUsbdSetPrivateData(dev_id, unit);

	if(sceUsbd_NOERR != (r = sceUsbdSetConfiguration(unit->c_pipe,
			cdesc->bConfigurationValue, set_config_done, unit))){
		err(unit, "sceUsbdSetConfiguration", r);
		return(-1);
	}

	if(sceUsbd_NOERR != (r = sceUsbdGetDeviceLocation(dev_id, locs))){
		err(unit, "sceUsbdGetDeviceLocation", r);
		return(-1);
	}

	printf("usbmouse%d: attached (port=", unit->number);
	for(r = 0; r < 7 && locs[r] != 0; r++)
		printf("%s%d", ((r)? ",": ""), locs[r]);
	printf(")\n");

	/* update common data */
	USBMOUSE.n++;
	USBMOUSE.con_unit[unit->number] = unit;

	return(0);
}

/* ---------------------------------------------
  Function Name	: unit_alloc
  function     	: LDD�̈ˑ��f�[�^�̊m�ۂƍ쐬
  Input Data	: dev_id(�f�o�C�XID)
                  payload(1�t���[�����̍ő�p�P�b�g��), 
                  ifnum(�C���^�[�t�F�[�X�ԍ�), 
                  as(��֐ݒ�)
  Output Data   : none
  Return Value	: NULL(�������m�ۂɎ��s)
                  not NULL(�ˑ��f�[�^�̃|�C���^)
----------------------------------------------*/
static UNIT *unit_alloc(int dev_id, int payload, int ifnum, int as){
	UNIT *p;
	int oldstat;

	if (USBMOUSE.n >= MAX_MOUSE_NUM) return NULL;
	
	CpuSuspendIntr(&oldstat);
	p = AllocSysMemory(0, sizeof(UNIT) + payload, NULL);
	CpuResumeIntr(oldstat);
	if(p != NULL){
	        p->dev_id = dev_id;
		p->number = USBMOUSE.next_number;
	        while(1) {
		  ++USBMOUSE.next_number;
		  if (USBMOUSE.next_number >= MAX_MOUSE_NUM) {
		    USBMOUSE.next_number = 0;
		  }
		  if (USBMOUSE.con_unit[USBMOUSE.next_number] == NULL) {
		    break;
		  }
		}
		p->payload = payload;
		p->ifnum = ifnum;
		p->as = as;
		p->tcount = 0;
	}
	return(p);
}

/* ---------------------------------------------
  Function Name	: unit_free
  function     	: LDD�̈ˑ��f�[�^�̊J��
  Input Data	: p(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void unit_free(UNIT *p){
        int oldstat;
	
	CpuSuspendIntr(&oldstat);
	FreeSysMemory(p);
	CpuResumeIntr(oldstat);
}

/* ---------------------------------------------
  Function Name	: set_config_done
  function     	: ����]��(SetConfiguration�v��)�̃R�[���o�b�N�֐�
  Input Data	: result(����]���̌���Usbd_XXX), 
                  count(�]���ς݃f�[�^��), 
                  arg(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void set_config_done(int result, int count, void *arg){
	UNIT *unit = arg;
	int r;

	err(unit, "sceUsbdSetConfiguration", result);

	if (unit->as > 0) {
	  r = sceUsbdSetInterface(unit->c_pipe, unit->ifnum, unit->as,
				  set_interface_done, unit);
	  err(unit, "sceUsbdSetInterface", r);
	} else {
	  data_transfer(unit);  /* bAlternateSetting �� 0 �̂Ƃ�,
				   SET_INTERFACE �𑗐M���Ȃ� */
	}
}

/* ---------------------------------------------
  Function Name	: set_interface_done
  function     	: ����]��(SetInterface�v��)�̃R�[���o�b�N�֐�
  Input Data	: result(����]���̌���Usbd_XXX), 
                  count(�]���ς݃f�[�^��), 
                  arg(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void set_interface_done(int result, int count, void *arg){
	UNIT *unit = arg;

	err(unit, "sceUsbdSetInterface", result);
	data_transfer(unit);
}

/* ---------------------------------------------
  Function Name	: data_transfer
  function     	: �C���^���v�g�]���̗v��
  Input Data	: unit(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void data_transfer(UNIT *unit){
	int r;

	r = sceUsbdInterruptTransfer(unit->d_pipe,
		unit->data, unit->payload, data_transfer_done, unit);
	err(unit, "sceUsbdInterruptTransfer", r);
}

/* ---------------------------------------------
  Function Name	: data_transfer_done
  function     	: �C���^���v�g�]���̃R�[���o�b�N�֐�
  Input Data	: result(�C���^���v�g�]���̌���Usbd_XXX), 
                  count(�]���ς݃f�[�^��), 
                  arg(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void data_transfer_done(int result, int count, void *arg)
{
	UNIT *unit = arg;
	int i;
	
	err(unit, "sceUsbdInterruptTransfer", result);
	if(result == sceUsbd_NOERR){
		printf("usbmouse%d: count=%d data=(",
			 unit->number, ++unit->tcount);
		for(i = 0; i < count; i++)
			printf(" %02x", unit->data[i]);
		printf(" )\n");
	}
	
	data_transfer(unit);
}

/* ---------------------------------------------
  Function Name	: usbmouse_detach
  function     	: LDD�ؒf�����֐�
  Input Data	: dev_id(�f�o�C�XID)
  Output Data   : none
  Return Value	: 0(����I��)
                  -1(LDD�ˑ��f�[�^�Ȃ�)
----------------------------------------------*/
static int usbmouse_detach(int dev_id){
	UNIT *unit;

	if(NULL == (unit = sceUsbdGetPrivateData(dev_id)))
		return(-1);
	
	/* update common data */
	USBMOUSE.n--;
	USBMOUSE.con_unit[unit->number] = NULL;
	
	/* free */
	printf("usbmouse%d: detached\n", unit->number);
	unit_free(unit);
	
	if ((USBMOUSE.n == 0) && (self_unload == SELF_UNLOAD_ON)) {
	  WakeupThread(special_thid);  /* ���ȍ폜�̊J�n */
	}
	
	return(0);
}


/* ---------------------------------------------
  Function Name	: stop_usbmouse
  function     	: USBMOUSE��~����(�����̉��)
  Input Data	: argc,argv
  Output Data   : none
  Return Value	: NO_RESIDENT_END (��~����)
                  REMOVABLE_RESIDENT_END (��~���s)
----------------------------------------------*/
static int stop_usbmouse(int argc, char* argv[])
{
  int i;
  UNIT *unit;
  int oldstat;

  /* �p�C�v�̒�~ & �v���C�x�[�g�f�[�^�̉�� */
  for (i=0;i<MAX_MOUSE_NUM;i++) {
    if ((unit = USBMOUSE.con_unit[i]) != NULL) {
      sceUsbdClosePipe(unit->c_pipe);
      sceUsbdClosePipe(unit->d_pipe);
      CpuSuspendIntr(&oldstat);
      FreeSysMemory(unit);
      CpuResumeIntr(oldstat);
    }
  }
  
  /* LDD�o�^�̉��� */
  sceUsbdUnregisterLdd(&usbmouse_ops);

  return NO_RESIDENT_END;
}
