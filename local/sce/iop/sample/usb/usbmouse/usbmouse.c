/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *      USB Mouse Class Driver
 *
 *                          Version 0.71
 *                          Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         usbmouse - usbmouse.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *      0.10            Jan, 2,2000     mka        Initial
 *      0.11            Jan,15,2000     hatto      Using SifRpc
 *      0.12            Apr,14,2000     mka        Using sceUsbdGetDeviceLocation()
 *      0.20            Apr,24,2000     fukunaga   Multi-MOUSE
 *      0.50            Nov,9,2000      fukunaga   for Release2.1
 *      0.60            Mar,22,2001     fukunaga   
 *      0.61            May,30,2001     fukunaga   Change start()
 *      0.70            July,12,2001    fukunaga   Unload
 *      0.71            Dec,1,2001      fukunaga   for Release 2.4.3
 */

#define AUTOLOAD

#include <stdio.h>
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

#include "usbmouse.h"

ModuleInfo Module = {"SCE_USB_MOUSE_SAMPLE", 0x0202 };

/*********************************************************
                           Type 
**********************************************************/
/* 一つのマウスを管理するための構造体 */
typedef struct _unit {
	int dev_id;  /* Device id */
        int number;  /* EEからのRPC要求を受けるためのINDEX (マウス番号) */
        int c_pipe;  /* Control pipe id */
        int d_pipe;  /* Data pipe id */
        int payload; /* Size of payload */
        int ifnum;   /* Interface number */
        int as;      /* Alternative setting number */
        int tcount; /* 接続されてから行ったデータ転送の総回数 */
  
        /* Ring buffer for RPC */
	int rp;    /* Read pointer */
        int wp;    /* Write pointer */
        int rblen; /* Buffer length */
        u_char ringbuf[RINGBUF_SIZE][MOUSE_DATA_LEN]; /* Ring buffer */
  
        /* Buffer for interrupt transfer */
	u_char data[0];  
} UNIT;

typedef struct {
        int next_number; /* 次に接続されたマウスに割り当てる接続番号 */
	int n;           /* 現在接続中のマウス数 */
	UNIT* con_unit[MAX_MOUSE_NUM]; /* UNIT構造体へのポインタの配列 */
        int thid_rpc;    /* RPCスレッドのID */
        sceSifQueueData qd;
	sceSifServeData sd;
} USBMOUSE_t;


/*********************************************************
                     Common Variables
**********************************************************/
static USBMOUSE_t USBMOUSE; /* このキーボードドライバのコントロールブロック */

/* RPCのためのバッファ */
#define SSIZE 0x20
static unsigned int buffer[SSIZE];

/* usbmload.irx でロードされたとき、常駐するかどうかを決めるフラグ */
#ifdef AUTOLOAD
static int resident_flag;

/* ロードモード(オプションによって決定) */
#define NORMAL_MODE   0
#define AUTOLOAD_MODE 1
static int load_mode = NORMAL_MODE;
#endif


/*********************************************************
                         Macros
**********************************************************/
#define err(p, f, r)	if(r) printf("usbmouse%d: %s -> 0x%x\n", \
				(p)->number, (f), (r))


/*********************************************************
                         Prototype
**********************************************************/
static void sifrpc_thread();
static void * sifrpc_server(unsigned int fno, void *data, int size);
static void * get_connect_info(unsigned int fno, void *data, int size);
static void * read_mouse(unsigned int fno, void *data, int size);
static void * get_device_location(unsigned int fno, void *data, int size);
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
	"usbmouse",      /* デバイス名 */
	usbmouse_probe,  /* デバイス検出処理関数(デバイス判定) */
	usbmouse_attach, /* デバイス接続処理関数(通信開始) */
	usbmouse_detach, /* デバイス切断処理関数 */
};

#define BASE_priority  50


/* ---------------------------------------------
  Function Name	: start
  function     	: メイン関数
  Input Data	: none
  Output Data   : none
  Return Value	: REMOVABLE_RESIDENT_END(Unload可能常駐終了),
                  NO_RESIDENT_END(非常駐終了)
----------------------------------------------*/
int start(int argc, char* argv[]){
	int r;
	struct ThreadParam param;
	int i;

#ifdef AUTOLOAD
	int j;
#endif

	if (argc < 0) {
	  /* Unload */
	  return stop_usbmouse(argc,argv);
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

#ifdef AUTOLOAD	
	load_mode = NORMAL_MODE;
	
	/* オプションの解析 */
	for(i=0; i<argc; i++) {
	  for(j=0; argv[i][j]!='\0'; j++) {
	    if (argv[i][j] == '=') { argv[i][j]='\0'; j++; break; }
	  }
	  if (strcmp(argv[i],"lmode") == 0) {
	    if (strcmp(argv[i]+j,"AUTOLOAD") == 0) 
	      { load_mode = AUTOLOAD_MODE; break; }
	  }
	}
	switch(load_mode) 
	  {
	  case NORMAL_MODE:
	    resident_flag = 1; /* 必ず常駐する(通常時の使用) */
	    break;
	  case AUTOLOAD_MODE:
	    printf("usbmouse : AUTOLOAD MODE\n");
	    resident_flag = 0; /* probe関数がフラグを立てたときのみ常駐 */
	    break;
	  }
#endif
	
	if(0 != (r = sceUsbdRegisterLdd(&usbmouse_ops))) {
	  /* ここで,usbmouse_probe関数が呼ばれる */
	  printf("usbmouse: sceUsbdRegisterLdd -> 0x%x\n", r);
	}
	
#ifdef AUTOLOAD
	if ( ! resident_flag ) {
	  /* usbmouse_probe関数がフラグを立てなかったので,常駐しない */
	  sceUsbdUnregisterLdd(&usbmouse_ops);
	  return NO_RESIDENT_END;
	}
#endif

	/* Init thread */
	param.attr         = TH_C;
	param.entry        = sifrpc_thread;
	param.initPriority = BASE_priority;
	param.stackSize    = 0x800;
	param.option       = 0;
	USBMOUSE.thid_rpc = CreateThread(&param);
	if (USBMOUSE.thid_rpc > 0) {
		StartThread(USBMOUSE.thid_rpc,0);
		r = REMOVABLE_RESIDENT_END;
	}else{
	        sceUsbdUnregisterLdd(&usbmouse_ops);
		r = NO_RESIDENT_END;
	}
	
	return r;
}

/* ---------------------------------------------
  Function Name	: sifrpc_thread
  function     	: EEからのRPCコールに応答するスレッド
  Input Data	: none
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void sifrpc_thread()
{
	sceSifInitRpc(0);
	sceSifSetRpcQueue(&USBMOUSE.qd, GetThreadId());
	sceSifRegisterRpc(&USBMOUSE.sd, SCE_RPC_USB_MOUSE, sifrpc_server,
			  (void *) &buffer[0],0,0,&USBMOUSE.qd);
	sceSifRpcLoop(&USBMOUSE.qd);
}

/* ---------------------------------------------
  Function Name	: sifrpc_server
  function     	: EEからRPCコールがあったときに
                  呼ばれる関数
  Input Data	: fno(関数番号),
                  data(EEから受信したデータ)
                  size(EEから受信したデータサイズ)
  Output Data   : data(EEへ送信するデータ)
  Return Value	: data(EEへ送信するデータ)
----------------------------------------------*/
static void * sifrpc_server(unsigned int fno, void *data, int size)
{
        switch(fno) 
	  {
	  case 0x01:
	    /* printf("Call get_connect_info()\n"); */
	    get_connect_info(fno,data,size);
	    break;
	  case 0x02:
	    /* printf("Call read_mouse()\n"); */
	    read_mouse(fno,data,size);
	    break;
	  case 0x03:
	    /* printf("Call get_device_location()\n"); */
	    get_device_location(fno,data,size);
	    break;
	  default:
	    printf("Illegal function No.%d from EE\n",fno);
	  }
	
	return data;
}

/* ---------------------------------------------
  Function Name	: get_connect_info (for RPC)
  function     	:                   
  Input Data	: fno(関数番号),
                  data(EEから受信したデータ)
                  size(EEから受信したデータサイズ)
  Output Data   : data(EEへ送信するデータ)
  Return Value	: data(EEへ送信するデータ)
----------------------------------------------*/
static void * get_connect_info(unsigned int fno, void *data, int size)
{
	u_char *p;
	int i;
	
	p = (unsigned char *) data;
	
	*p++= MAX_MOUSE_NUM;
	*p++= USBMOUSE.n;
	for(i=0;i<MAX_MOUSE_NUM;i++) {
	  if (USBMOUSE.con_unit[i] != NULL) {
	    *p++ = 1;
	  } else {
	    *p++ = 0;
	  }
	}
	
	return data;
}

/* ---------------------------------------------
  Function Name	: read_mouse (for RPC)
  function     	:                   
  Input Data	: fno(関数番号),
                  data(EEから受信したデータ)
                  size(EEから受信したデータサイズ)
  Output Data   : data(EEへ送信するデータ)
  Return Value	: data(EEへ送信するデータ)
----------------------------------------------*/
static void * read_mouse(unsigned int fno, void *data, int size)
{
        int oldstat;
	unsigned char *p;
	UNIT *unit;
	int i;
	u_char *mousebuf;
	
	p = (unsigned char *) data;
	if (p[0] > MAX_MOUSE_NUM) {
	  printf("Illegal index_no : %d\n",p[0]); /* debug */
	  p[0] = 0;  /* data length = 0 */
	  return data;
	}
	unit = USBMOUSE.con_unit[p[0]]; /* p[0]: mouse number */
	if (unit == NULL) {
	  printf("USB mouse %d is not connected!\n",p[0]);
	  p[0] = 0;  /* data length = 0 */
	  return data;
	}
	
	/* printf("wp:%d rp:%d len:%d pl:%d\n",
	   unit->wp,unit->rp,unit->rblen,unit->payload);
	   */ /* debug */
	
	/*----- get from ringbuffer -----*/
	CpuSuspendIntr(&oldstat);
	
	if (unit->rblen > 0) {
	  mousebuf = &(unit->ringbuf[unit->rp][0]);
	  *p++ = mousebuf[0]; /* len */
	  for(i=0;i<unit->payload;i++) {
	    *p++ = mousebuf[i+1];
	  }
	  if (++unit->rp >= RINGBUF_SIZE) { unit->rp = 0; }
	  unit->rblen--;
	} else {
	  *p++ = 0;           /* len */
	  for(i=0;i<unit->payload;i++) { *p++ = 0; }
	}

	CpuResumeIntr(oldstat);
	
	return data;
}

/* ---------------------------------------------
  Function Name	: get_device_location(for RPC)
  function     	: デバイスの位置情報を取得
  Input Data	: fno(関数番号),
                  data(EEから受信したデータ)
                  size(EEから受信したデータサイズ)
  Output Data   : data(EEへ送信するデータ)
  Return Value	: data(EEへ送信するデータ)
----------------------------------------------*/
static void * get_device_location(unsigned int fno, void *data, int size)
{
	u_char *p;
	u_char locs[7];
	int i;
	int dev_id;
	int r;
	
	p = (unsigned char *) data;
	
	if (p[0] >= MAX_MOUSE_NUM) {
	  printf("Illegal USB mouse number! : %d\n",p[0]);
	  for(i=0;i<8;i++) { *p++ = 0; }
	  return data;
	}
	
	if (USBMOUSE.con_unit[p[0]] == NULL) {
	  printf("USB mouse %d is not connected!\n",p[0]);
	  for(i=0;i<8;i++) { *p++ = 0; }
	  return data;
	}
	
	dev_id = USBMOUSE.con_unit[p[0]]->dev_id;
	
	if(sceUsbd_NOERR != (r = sceUsbdGetDeviceLocation(dev_id, locs))){
		err(USBMOUSE.con_unit[p[0]], "sceUsbdGetDeviceLocation", r);
		for(i=0;i<8;i++) { *p++ = 0; }
		return data;
	}
	
	for(i=0;i<7;i++) { *p++ = locs[i]; }
	
	return data;
}

/* ---------------------------------------------
  Function Name	: usbmouse_probe
  function     	: LDD検出処理関数
  Input Data	: dev_id(デバイスID)
  Output Data   : none
  Return Value	: 0(検出したデバイスはこのLDDに対応しない)
                  1(検出したデバイスはこのLDDに対応)
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

#ifdef AUTOLOAD
	printf("usbmouse : resident_flag is ON\n");
	resident_flag = 1;
#endif

	return(1);
}

/* ---------------------------------------------
  Function Name	: usbmouse_attach
  function     	: LDD接続処理関数
                  (usbmouse_probeの戻り値が1のときコールされる)
  Input Data	: dev_id(デバイスID)
  Output Data   : none
  Return Value	: 0(コンフィグレーション正常終了)
                  -1(コンフィグレーション失敗)
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
  function     	: LDDの依存データの確保と作成
  Input Data	: dev_id(デバイスID)
                  payload(1フレーム中の最大パケット長), 
                  ifnum(インターフェース番号), 
                  as(代替設定)
  Output Data   : none
  Return Value	: NULL(メモリ確保に失敗)
                  not NULL(依存データのポインタ)
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
		p->rp = 0;
		p->wp = 0;
		p->rblen = 0;
	}
	return(p);
}

/* ---------------------------------------------
  Function Name	: unit_free
  function     	: LDDの依存データの開放
  Input Data	: p(LDD依存データのポインタ)
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
  function     	: 制御転送(SetConfiguration要求)のコールバック関数
  Input Data	: result(制御転送の結果Usbd_XXX), 
                  count(転送済みデータ数), 
                  arg(LDD依存データのポインタ)
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
	  data_transfer(unit);  /* bAlternateSetting が 0 のとき,
				   SET_INTERFACE を送信しない */
	}
}

/* ---------------------------------------------
  Function Name	: set_interface_done
  function     	: 制御転送(SetInterface要求)のコールバック関数
  Input Data	: result(制御転送の結果Usbd_XXX), 
                  count(転送済みデータ数), 
                  arg(LDD依存データのポインタ)
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
  function     	: インタラプト転送の要求
  Input Data	: unit(LDD依存データのポインタ)
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
  function     	: インタラプト転送のコールバック関数
  Input Data	: result(インタラプト転送の結果Usbd_XXX), 
                  count(転送済みデータ数), 
                  arg(LDD依存データのポインタ)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void data_transfer_done(int result, int count, void *arg)
{
	int oldstat;
	UNIT *unit = arg;
	int i;
	u_char *mousebuf;
	
	err(unit, "sceUsbdInterruptTransfer", result);
	if(result == sceUsbd_NOERR){
		printf("usbmouse%d: count=%d data=(",
			 unit->number, ++unit->tcount);
		for(i = 0; i < count; i++)
			printf(" %02x", unit->data[i]);
		printf(" )\n");
	}
	
	/*----- put ringbuffer (for sifrpc) -----*/
	/* printf("wp:%d rp:%d len:%d\n",unit->wp,unit->rp,unit->rblen); */ /* debug */
	
	CpuSuspendIntr(&oldstat);
	
	if (unit->rblen < RINGBUF_SIZE) {
	  mousebuf = &(unit->ringbuf[unit->wp][0]);
	  mousebuf[0] = (u_char)(count & 0xff);
	  for(i=0; i<count; i++) {
	    mousebuf[i+1] = unit->data[i];
	  }
	  if (++unit->wp >= RINGBUF_SIZE) { unit->wp = 0; }
	  unit->rblen++;
	}
	
	CpuResumeIntr(oldstat);
	
	data_transfer(unit);
}

/* ---------------------------------------------
  Function Name	: usbmouse_detach
  function     	: LDD切断処理関数
  Input Data	: dev_id(デバイスID)
  Output Data   : none
  Return Value	: 0(正常終了)
                  -1(LDD依存データなし)
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
	
	return(0);
}


/* ---------------------------------------------
  Function Name	: stop_usbmouse
  function     	: USBMOUSE停止処理(資源の解放)
  Input Data	: argc,argv
  Output Data   : none
  Return Value	: NO_RESIDENT_END (停止成功)
                  REMOVABLE_RESIDENT_END (停止失敗)
----------------------------------------------*/
static int stop_usbmouse(int argc, char* argv[])
{
  int i;
  UNIT *unit;
  int oldstat;

  /* パイプの停止 & プライベートデータの解放 */
  for (i=0;i<MAX_MOUSE_NUM;i++) {
    if ((unit = USBMOUSE.con_unit[i]) != NULL) {
      sceUsbdClosePipe(unit->c_pipe);
      sceUsbdClosePipe(unit->d_pipe);
      CpuSuspendIntr(&oldstat);
      FreeSysMemory(unit);
      CpuResumeIntr(oldstat);
    }
  }
  
  /* LDD登録の解除 */
  sceUsbdUnregisterLdd(&usbmouse_ops);

  TerminateThread(USBMOUSE.thid_rpc);
  DeleteThread(USBMOUSE.thid_rpc);

  sceSifRemoveRpc(&USBMOUSE.sd,&USBMOUSE.qd);
  sceSifRemoveRpcQueue(&USBMOUSE.qd);

  return NO_RESIDENT_END;
}
