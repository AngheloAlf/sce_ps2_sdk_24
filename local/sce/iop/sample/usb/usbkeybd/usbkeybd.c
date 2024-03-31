/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *      USB Keyboard Class Driver (for IOP)
 *
 *                          Version 0.62
 *                          Shift-JIS
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                              usbkeybd.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *      0.10            Apr,13,2000     fukunaga   USB Keyboard sample
 *      0.50            Nov,9,2000      fukunaga   for Release2.1
 *      0.60            Jan,15,2001     fukunaga   SET_INTERFACE
 *      0.61            May,30,2001     fukunaga   Change start()
 *      0.62            Dec,1,2001      fukunaga   for Release2.4.3
 *
 */


#define AUTOLOAD

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <memory.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>

#include <usb.h>
#include <usbd.h>

#ifdef AUTOLOAD
#include <usbmload.h>
#endif

#include "usbkeybd.h"

ModuleInfo Module = {"SCE_USB_KEYBOARD_SAMPLE", 0x0201 };


/*********************************************************
                           Type 
**********************************************************/
/* ��̃L�[�{�[�h���Ǘ����邽�߂̍\���� */
typedef struct _unit {
  int    dev_id;  /* Device id */
  int    number;  /* EE�����RPC�v�����󂯂邽�߂�INDEX (�L�[�{�[�h�ԍ�) */
  int    c_pipe;  /* Control pipe id */
  int    d_pipe;  /* Data pipe id */
  int    payload; /* Size of payload */
  int    ifnum;   /* Interface number */
  int    as;      /* Alternative setting number */
  int    count;   /* �ڑ��� */
  u_char ledptn;  /* LED�̓_���p�^�[��  */
  u_char old_ledbtn; /* �O���LED�_���p�^�[�� */
  
  /* Ring buffer */
  int rp;    /* Read pointer */
  int wp;    /* Write pointer */
  int rblen; /* Buffer length */
  u_char ringbuf[RINGBUF_SIZE][KEY_DATA_LEN+2]; /* Ring buffer */
  
  u_char data[0];  /* Buffer for interrupt transfer */
} UNIT;

/* ���̃L�[�{�[�h�h���C�o�̃R���g���[���u���b�N */
typedef struct {
  int next_number; /* ���ɐڑ����ꂽ�L�[�{�[�h�Ɋ��蓖�Ă�ڑ��ԍ� */
  int n;           /* ���ݐڑ����̃L�[�{�[�h�� */
  UNIT* con_unit[MAX_KEYBD_NUM]; /* UNIT�\���̂ւ̃|�C���^�̔z�� */
} USBKEYBD_t;

/*********************************************************
                     Common Variables
**********************************************************/
static USBKEYBD_t USBKEYBD; /* ���̃L�[�{�[�h�h���C�o�̃R���g���[���u���b�N */

/* RPC�̂��߂̃o�b�t�@ */
#define SSIZE 0x20
static unsigned int buffer [SSIZE/sizeof(u_int)]; 

/* usbmload.irx �Ń��[�h���ꂽ�Ƃ��A�풓���邩�ǂ��������߂�t���O */
#ifdef AUTOLOAD
static int resident_flag;  

/* ���[�h���[�h(�I�v�V�����ɂ���Č���) */
#define NORMAL_MODE   0
#define AUTOLOAD_MODE 1
#define TESTLOAD_MODE 2
static int load_mode;
#endif

/*********************************************************
                         Macros
**********************************************************/
#define err(p, f, r)	if(r) printf("usbkeybd%d: %s -> 0x%x\n", \
				(p)->number, (f), (r))

/*********************************************************
                         Prototype
**********************************************************/
static void sifrpc_thread();
static void * sifrpc_server(unsigned int fno, void *data, int size);
static void * get_connect_info(unsigned int fno, void *data, int size);
static void * read_keyboard(unsigned int fno, void *data, int size);
static void * get_device_location(unsigned int fno, void *data, int size);
static int usbkeybd_probe(int dev_id);
static int usbkeybd_attach(int dev_id);
static UNIT *unit_alloc(int dev_id, int payload, int ifnum, int as);
static void unit_free(UNIT *p);
static void set_config_done(int result, int count, void *arg);
static void set_interface_done(int result, int count, void *arg);
static void set_idle_request(UNIT *unit);
static void set_idle_request_done(int result, int count, void *arg);
static void data_transfer(UNIT *unit);
static void data_transfer_done(int result, int count, void *arg);
static void led_transfer_done(int result, int count, void *arg);
static int usbkeybd_detach(int dev_id);

/*********************************************************
                      Program Start
**********************************************************/

static sceUsbdLddOps usbkeybd_ops = {
	NULL, NULL,
	"usbkeybd",      /* �f�o�C�X�� */
	usbkeybd_probe,  /* �f�o�C�X���o�����֐�(�f�o�C�X����) */
	usbkeybd_attach, /* �f�o�C�X�ڑ������֐�(�ʐM�J�n) */
	usbkeybd_detach, /* �f�o�C�X�ؒf�����֐� */
};

#define BASE_priority  50

/* ---------------------------------------------
  Function Name	: start
  function     	: ���C���֐�
  Input Data	: none
  Output Data   : none
  Return Value	: RESIDENT_END(�풓�I��),
                  NO_RESIDENT_END(��풓�I��)
----------------------------------------------*/
int start(int argc, char *argv[]){
	int r;
	struct ThreadParam param;
	int th;
	int i,j;

	/* Init common variables */
	USBKEYBD.next_number = 0;
	USBKEYBD.n = 0;
	for(i=0;i<MAX_KEYBD_NUM;i++) {
	  USBKEYBD.con_unit[i] = NULL;
	}
	
#ifdef AUTOLOAD
	load_mode = NORMAL_MODE;
	
	/* �I�v�V�����̉�� */
	for(i=0; i<argc; i++) {
	  for(j=0; argv[i][j]!='\0'; j++) {
	    if (argv[i][j] == '=') { argv[i][j]='\0'; j++; break; }
	  }
	  if (strcmp(argv[i],"lmode") != 0) { continue; }
	  
	  if (strcmp(argv[i]+j,"AUTOLOAD") == 0) 
	    { load_mode = AUTOLOAD_MODE; break; }
	  if (strcmp(argv[i]+j,"TESTLOAD") == 0) 
	    { load_mode = TESTLOAD_MODE; break; }
	}
	switch(load_mode) 
	  {
	  case NORMAL_MODE:
	    resident_flag = 1; /* �K���풓����(�ʏ펞�̎g�p) */
	    break;
	  case AUTOLOAD_MODE:
	    printf("usbkeybd : AUTOLOAD MODE\n");
	    resident_flag = 0; /* probe�֐����t���O�𗧂Ă��Ƃ��̂ݏ풓 */
	    break;
	  case TESTLOAD_MODE:
	    printf("usbkeybd : TESTLOAD MODE\n");
	    resident_flag = 0; /* �t���O�Ɋ֌W�Ȃ��풓���Ȃ� */
	    break;
	  }
#endif
	
	/* register ldd */
	if(0 != (r = sceUsbdRegisterLdd(&usbkeybd_ops))) {
	  /* ������,usbkeybd_probe�֐����Ă΂�� */
	  printf("usbkeybd: sceUsbdRegisterLdd -> 0x%x\n", r);
	}
	
#ifdef AUTOLOAD
	if (load_mode == TESTLOAD_MODE) {
	  /* TESTLOAD �Ȃ̂�,resident_flag�Ɋ֌W�Ȃ��풓���Ȃ� */
	  sceUsbdUnregisterLdd(&usbkeybd_ops);
	  if (resident_flag == 1) {
	    return (NO_RESIDENT_END | TESTLOAD_OK);
	  } else {
	    return (NO_RESIDENT_END | TESTLOAD_NG);	    
	  }
	}
	
	if ( ! resident_flag ) {
	  /* usbkeybd_probe�֐����t���O�𗧂ĂȂ������̂�,�풓���Ȃ� */
	  sceUsbdUnregisterLdd(&usbkeybd_ops);
	  return NO_RESIDENT_END;
	}
#endif
	
	/* Init thread */
	param.attr         = TH_C;
	param.entry        = sifrpc_thread;
	param.initPriority = BASE_priority;
	param.stackSize    = 0x1000;
	param.option       = 0;
	th = CreateThread(&param);
	if (th > 0) {
		StartThread(th,0);
		r = RESIDENT_END;
	}else{
	        sceUsbdUnregisterLdd(&usbkeybd_ops);
		r = NO_RESIDENT_END;
	}
	
	return r;
}

/* ---------------------------------------------
  Function Name	: sifrpc_thread
  function     	: EE�����RPC�R�[���ɉ�������X���b�h
  Input Data	: none
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void sifrpc_thread()
{
	sceSifQueueData qd;
	sceSifServeData sd;
	
	sceSifInitRpc(0);
	sceSifSetRpcQueue(&qd, GetThreadId());
	sceSifRegisterRpc(&sd, SCE_RPC_USB_KEYBD, sifrpc_server,
			  (void *) &buffer[0],0,0,&qd);
	sceSifRpcLoop(&qd);
}

/* ---------------------------------------------
  Function Name	: sifrpc_server
  function     	: EE����RPC�R�[�����������Ƃ���
                  �Ă΂��֐�
  Input Data	: fno(�֐��ԍ�),
                  data(EE�����M�����f�[�^)
                  size(EE�����M�����f�[�^�T�C�Y)
  Output Data   : data(EE�֑��M����f�[�^)
  Return Value	: data(EE�֑��M����f�[�^)
----------------------------------------------*/
static void * sifrpc_server(unsigned int fno, void *data, int size)
{
        switch(fno) 
	  {
	  case 0x01:
	    //printf("Call get_connect_info()\n");
	    get_connect_info(fno,data,size);
	    break;
	  case 0x02:
	    //printf("Call read_keyboard()\n");
	    read_keyboard(fno,data,size);
	    break;
	  case 0x03:
	    //printf("Call get_device_location()\n");
	    get_device_location(fno,data,size);
	    break;
	  default:
	    printf("Illegal function No.%d from EE\n",fno);
	  }
	
	return data;
}

/* ---------------------------------------------
  Function Name	: get_connect_info(for RPC)
  function     	: �L�[�{�[�h�̐ڑ����̎擾
  Input Data	: fno(�֐��ԍ�),
                  data(EE�����M�����f�[�^)
                  size(EE�����M�����f�[�^�T�C�Y)
  Output Data   : data(EE�֑��M����f�[�^)
  Return Value	: data(EE�֑��M����f�[�^)
----------------------------------------------*/
static void * get_connect_info(unsigned int fno, void *data, int size)
{
	u_char *p;
	int i;
	
	p = (unsigned char *) data;
	
	*p++= MAX_KEYBD_NUM;
	*p++= USBKEYBD.n;
	for(i=0;i<MAX_KEYBD_NUM;i++) {
	  if (USBKEYBD.con_unit[i] != NULL) {
	    *p++ = 1;
	  } else {
	    *p++ = 0;
	  }
	}
	
	return data;
}

/* ---------------------------------------------
  Function Name	: read_keyboard
  function     	: �L�[�f�[�^�̎擾(for RPC)
  Input Data	: fno(�֐��ԍ�)
                  data(EE�����M�����f�[�^)
                  size(EE�����M�����f�[�^�T�C�Y)
  Output Data   : data(EE�֑��M����f�[�^)
  Return Value	: data(EE�֑��M����f�[�^)
----------------------------------------------*/
static void * read_keyboard(unsigned int fno, void *data, int size)
{
        int oldstat;
	u_char *p;
	UNIT *unit;
	int i;
	u_char *keybuf;
	
	p = (unsigned char *) data;
	if (p[0] > MAX_KEYBD_NUM) {
	  printf("Illegal index_no : %d\n",p[0]); //debug
	  p[0] = 0;  /* data length = 0 */
	  return data;
	}
	unit = USBKEYBD.con_unit[p[0]]; /* p[0]: keyboard number */
	if (unit == NULL) {
	  printf("USB keyboard %d is not connected!\n",p[0]);
	  p[0] = 0;  /* data length = 0 */
	  return data;
	}
	
#if 0
	printf("wp:%d rp:%d len:%d pl:%d\n",
	       unit->wp,unit->rp,unit->rblen,unit->payload); /* debug */
#endif
	
	/*----- get from ringbuffer -----*/
	CpuSuspendIntr(&oldstat);
	
	if (unit->rblen > 0) {
	  keybuf = &(unit->ringbuf[unit->rp][0]);
	  *p++ = keybuf[0]; /* ledptn */
	  *p++ = keybuf[1]; /* len */
	  for(i=0;i<unit->payload;i++) {
	    *p++ = keybuf[i+2];
	  }
	  if (++unit->rp >= RINGBUF_SIZE) { unit->rp = 0; }
	  unit->rblen--;
	} else {
	  *p++ = unit->ledptn; /* ledptn */
	  *p++ = 0;            /* len */
	  for(i=0;i<unit->payload;i++) { *p++ = 0; }
	}
	
	CpuResumeIntr(oldstat);
	
	return data;
}

/* ---------------------------------------------
  Function Name	: get_device_location(for RPC)
  function     	: �f�o�C�X�̈ʒu�����擾
  Input Data	: fno(�֐��ԍ�),
                  data(EE�����M�����f�[�^)
                  size(EE�����M�����f�[�^�T�C�Y)
  Output Data   : data(EE�֑��M����f�[�^)
  Return Value	: data(EE�֑��M����f�[�^)
----------------------------------------------*/
static void * get_device_location(unsigned int fno, void *data, int size)
{
	u_char *p;
	u_char locs[7];
	int i;
	int dev_id;
	int r;
	
	p = (unsigned char *) data;
	
	if (p[0] >= MAX_KEYBD_NUM) {
	  printf("Illegal USB keyboard number! : %d\n",p[0]);
	  for(i=0;i<8;i++) { *p++ = 0; }
	  return data;
	}
	
	if (USBKEYBD.con_unit[p[0]] == NULL) {
	  printf("USB keyboard %d is not connected!\n",p[0]);
	  for(i=0;i<8;i++) { *p++ = 0; }
	  return data;
	}
	
	dev_id = USBKEYBD.con_unit[p[0]]->dev_id;
	
	if(sceUsbd_NOERR != (r = sceUsbdGetDeviceLocation(dev_id, locs))){
		err(USBKEYBD.con_unit[p[0]], "sceUsbdGetDeviceLocation", r);
		for(i=0;i<8;i++) { *p++ = 0; }
		return data;
	}
	
	for(i=0;i<7;i++) { *p++ = locs[i]; }
	
	return data;
}



/* ---------------------------------------------
  Function Name	: usbkeybd_probe
  function     	: LDD���o�����֐�
  Input Data	: dev_id(�f�o�C�XID)
  Output Data   : none
  Return Value	: 0(���o�����f�o�C�X�͂���LDD�ɑΉ����Ȃ�)
                  1(���o�����f�o�C�X�͂���LDD�ɑΉ�)
----------------------------------------------*/
static int usbkeybd_probe(int dev_id){
	UsbDeviceDescriptor *ddesc;
	UsbInterfaceDescriptor *idesc;
	
	if (USBKEYBD.n >= MAX_KEYBD_NUM) {
	  return(0);
	}

	if(NULL == (ddesc = sceUsbdScanStaticDescriptor(dev_id, NULL,
			USB_DESCRIPTOR_TYPE_DEVICE)))
		return(0);
	if(ddesc->bDeviceClass != 0)  /* 0:HID Device Class */ 
		return(0);
	//printf("ddesc->bNumConfigurations:%d\n",ddesc->bNumConfigurations);
	if(ddesc->bNumConfigurations != 1)
		return(0);

	idesc = (UsbInterfaceDescriptor *)ddesc;
	if(NULL == (idesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
					  USB_DESCRIPTOR_TYPE_INTERFACE)))
	  { return(0); }
	if( (idesc->bInterfaceClass    != 3) ||  /* 3:HID Interface Class*/
	    (idesc->bInterfaceSubClass != 1) ||  /* 1:Boot Interface subclass */
	    (idesc->bInterfaceProtocol != 1) )   /* 1:Keyboard */
	  { return(0); }

#ifdef AUTOLOAD
	printf("usbkeybd : resident_flag is ON\n");
	resident_flag = 1;
	if (load_mode == TESTLOAD_MODE) return(0);
#endif
	return(1);
}

/* ---------------------------------------------
  Function Name	: usbkeybd_attach
  function     	: LDD�ڑ������֐�
                  (usbkeybd_probe�̖߂�l��1�̂Ƃ��R�[�������)
  Input Data	: dev_id(�f�o�C�XID)
  Output Data   : none
  Return Value	: 0(�R���t�B�O���[�V��������I��)
                  -1(�R���t�B�O���[�V�������s)
----------------------------------------------*/
static int usbkeybd_attach(int dev_id){
	UsbConfigurationDescriptor *cdesc;
	UsbInterfaceDescriptor *idesc;
	UsbEndpointDescriptor *edesc;
	UNIT *unit;
	int payload, r;
	u_char locs[7];
	
	if(NULL == (cdesc = sceUsbdScanStaticDescriptor(dev_id, NULL,
			USB_DESCRIPTOR_TYPE_CONFIGURATION)))
		return(-1);
	
        idesc = (UsbInterfaceDescriptor *)cdesc;
	if(NULL == (idesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
					  USB_DESCRIPTOR_TYPE_INTERFACE)))
	  { return(-1); }
	if( (idesc->bInterfaceClass    != 3) ||  /* 3:HID Interface Class*/
	    (idesc->bInterfaceSubClass != 1) ||  /* 1:Boot Interface subclass */
	    (idesc->bInterfaceProtocol != 1) )   /* 1:Keyboard */
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
	if(payload > KEY_DATA_LEN) { return -1;	}

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
	
	printf("dev_id:%d\n",dev_id);
	printf("usbkeybd%d: attached (port=", unit->number);
	for(r = 0; r < 7 && locs[r] != 0; r++)
		printf("%s%d", ((r)? ",": ""), locs[r]);
	printf(")\n");
	
	// update common data
	USBKEYBD.n++;
	USBKEYBD.con_unit[unit->number] = unit;
	
	return(0);
}

/* ---------------------------------------------
  Function Name	: unit_alloc
  function     	: LDD�̈ˑ��f�[�^�̊m�ۂƍ쐬
  Input Data	: payload(1�t���[�����̍ő�p�P�b�g��), 
                  ifnum(�C���^�[�t�F�[�X�ԍ�), 
                  as(��֐ݒ�)
  Output Data   : none
  Return Value	: NULL(�������m�ۂɎ��s)
                  not NULL(�ˑ��f�[�^�̃|�C���^)
----------------------------------------------*/
static UNIT *unit_alloc(int dev_id, int payload, int ifnum, int as){
	UNIT *p;
	int oldstat;
	
	if (USBKEYBD.n >= MAX_KEYBD_NUM) return NULL;
	
	CpuSuspendIntr(&oldstat);
	p = AllocSysMemory(0, sizeof(UNIT) + payload, NULL);
	CpuResumeIntr(oldstat);
	if(p != NULL){
	        p->dev_id = dev_id;
		p->number = USBKEYBD.next_number;
	        while(1) {
		  ++USBKEYBD.next_number;
		  if (USBKEYBD.next_number >= MAX_KEYBD_NUM) {
		    USBKEYBD.next_number = 0;
		  }
		  if (USBKEYBD.con_unit[USBKEYBD.next_number] == NULL) {
		    break;
		  }
		}
		p->payload = payload;
		p->ifnum = ifnum;
		p->as = as;
		p->count = 0;
		p->ledptn = 0;
		p->old_ledbtn = 0;
		p->rp = 0;
		p->wp = 0;
		p->rblen = 0;
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
	  /* bAlternateSetting �� 0 �̂Ƃ�,SET_INTERFACE �𑗐M���Ȃ� */
	  set_idle_request(unit);
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
	set_idle_request(unit);
}

/* ---------------------------------------------
  Function Name	: set_idle_request
  function     	: SET_IDLE�v���𑗐M
  Input Data	: unit(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void set_idle_request(UNIT *unit){
  int r;
  
  r = sceUsbdControlTransfer(unit->c_pipe,  /* Pipe ID */
			     0x21,          /* bmRequestType */
			     0x0A,          /* bRequest = Set_Idle_Request */
			     0x0000,        /* Duration = 0
					       Report ID   = 0x0000 */
			     unit->ifnum,   /* Interface */
			     0,             /* Report Length = 0byte */
			     (void*)NULL,
			     set_idle_request_done, /* Callback */
			     unit);
  
  err(unit, "set_idle_request", r);
}

/* ---------------------------------------------
  Function Name	: set_idle_request_done
  function     	: ����]��(SetIdle�v��)�̃R�[���o�b�N�֐�
  Input Data	: result(����]���̌���Usbd_XXX), 
                  count(�]���ς݃f�[�^��), 
                  arg(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void set_idle_request_done(int result, int count, void *arg){
	UNIT *unit = arg;

	err(unit, "set_idle_request_done", result);
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
        int oldstat;
	UNIT *unit = arg;
	int i;
	int r;
	u_char *keybuf;
	u_char ledbtn = 0;
	
	err(unit, "data_transfer_done:sceUsbdInterruptTransfer", result);
	if(result == sceUsbd_NOERR){
		printf("usbkeybd%d: count=%d led=%02X data=(",
			 unit->number, ++unit->count, unit->ledptn);
		for(i = 0; i < count; i++)
			printf(" %02x", unit->data[i]);
		printf(" )\n");
	} else {
	  data_transfer(unit);
	  return;
	}
	
	for(i=2; i < count; i++) {
	  switch(unit->data[i])
	    {
	    case 0x53: /* NUM LOCK */
	      ledbtn |= 1<<0;
	      break;
	    case 0x39: /* CAPS LOCK */
#if (CAPS_LED_TYPE == MAC)
	      ledbtn |= 1<<1;
#endif
#if (CAPS_LED_TYPE == WIN)
	      if (unit->data[0] & 0x22) {
		ledbtn |= 1<<1;
	      }
#endif
	      break;
	    case 0x47: /* SCROLL LOCK */
	      ledbtn |= 1<<2;
	      break;
	    case 0xE3: /* COMPOSE */
	      ledbtn |= 1<<3;
	      break;
	    case 0x90: /* KANA */
	      ledbtn |= 1<<4;
	      break;
	    }
	}
	
	if (unit->old_ledbtn != ledbtn) {  /* LED�Ɋ֌W����{�^���ɕω������邩 */

	  unit->old_ledbtn = ledbtn;
	  unit->ledptn ^= ledbtn;   /* �{�^���������ꂽLED�𔽓]���� */
	  
	  r = sceUsbdControlTransfer(unit->c_pipe,  /* Pipe ID */
				     0x21,          /* bmRequestType */
				     0x09,          /* bRequest = SET_REPORT */
				     0x02*256+0x00, /* Report type = 0x02(Output)
						       Report ID   = 0x00 */
				     unit->ifnum,   /* Interface */
				     1,             /* Report Length = 1byte */
				     &unit->ledptn, /* LED DATA */
				     led_transfer_done, /* Callback */
				     unit);
	  if(sceUsbd_NOERR != r){
	    err(unit, "sceUsbdControlTransfer", r);
	  }
	} else {
	  data_transfer(unit);
	}
	
	/*----- put ringbuffer (for sifrpc) -----*/
	
	/* printf("wp:%d rp:%d len:%d\n",unit->wp,unit->rp,unit->rblen); */ /*debug*/
	
	CpuSuspendIntr(&oldstat);
	
	if (unit->rblen < RINGBUF_SIZE) {
	  keybuf = &(unit->ringbuf[unit->wp][0]);
	  keybuf[0] = unit->ledptn;
	  keybuf[1] = (u_char)(count & 0xff);
	  for(i=0;i<count;i++) {
	    keybuf[i+2] = unit->data[i];
	  }
	  if (++unit->wp >= RINGBUF_SIZE) { unit->wp = 0; }
	  unit->rblen++;
	}
	
	CpuResumeIntr(oldstat);
}

/* ---------------------------------------------
  Function Name	: led_transfer_done
  function     	: LED�f�[�^�]���̃R�[���o�b�N�֐�
  Input Data	: result(����]���̌���Usbd_XXX), 
                  count(�]���ς݃f�[�^��), 
                  arg(LDD�ˑ��f�[�^�̃|�C���^)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void led_transfer_done(int result, int count, void *arg){
	UNIT *unit = arg;
	
	err(unit, "led_transfer_done", result);
	
	data_transfer(unit);
}

/* ---------------------------------------------
  Function Name	: usbkeybd_detach
  function     	: LDD�ؒf�����֐�
  Input Data	: dev_id(�f�o�C�XID)
  Output Data   : none
  Return Value	: 0(����I��)
                  -1(LDD�ˑ��f�[�^�Ȃ�)
----------------------------------------------*/
static int usbkeybd_detach(int dev_id){
	UNIT *unit;
	
	if(NULL == (unit = sceUsbdGetPrivateData(dev_id)))
		return(-1);
	
	// update common data
	USBKEYBD.n--;
	USBKEYBD.con_unit[unit->number] = NULL;
	
	// free
	printf("usbkeybd%d: detached\n", unit->number);
	unit_free(unit);
	
	return(0);
}

