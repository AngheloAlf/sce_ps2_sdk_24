/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *      USB load function sample for usbmload.irx
 *
 *                          Version 0.40
 *                          Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                             usbloadf.c
 *
 *        Version       Date             Design     Log
 *  --------------------------------------------------------------------
 *        0.10          July,23,2000     fukunaga   Initial
 *        0.11          August,17,2000   fukunaga   
 *        0.20          Sep,29,2000      fukunaga   
 *        0.30          Nov,9,2000       fukunaga
 *        0.40          July,12,2001     fukunaga   Unload
 */

#include <stdio.h>
#include <kernel.h>
#include <sys/file.h>
#include <memory.h>

#include <libcdvd.h>
#include <usb.h>
#include <usbd.h>
#include <usbmload.h>

ModuleInfo Module = {"USB_load_function_sample", 0x0101 };

/*********************************************************
                           Type 
**********************************************************/


/*********************************************************
                         Macros
**********************************************************/
#define BASE_priority   120
#define RESIDENT_END_MASK  0x03

#define ARGLOAD 0  /* usbmload.irx の引数で設定ファイルを読む */
#define APILOAD 1  /* sceUsbmlLoadConffile() で設定ファイルを読む */
#define APIREGI 2  /* 設定ファイルを使わずに sceUsbmlRegisterDevice()で登録 */

#define HOST 0
#define CD   1


/*********************************************************
                     Common Variables
**********************************************************/
static int Load = ARGLOAD;  /* 設定のロード方法を選択する変数 */
static int Driver = HOST;   /* ドライバの置いている場所を選択する変数 */
static int Conffile = HOST; /* 設定ファイルの置いている場所を選択する変数 */
static int thid;            /* このモジュールの設定を行うスレッドのID */


/*********************************************************
                         Prototype
**********************************************************/
static int stop_usbloadf(int argc, char* argv[]);
static void setup_usbmload(int arg);
static void loadfunc(sceUsbmlPopDevinfo pop_devinfo);
static int sjis2euc(char *string);


/*********************************************************
                      Program Start
**********************************************************/

/* ---------------------------------------------
  Function Name	: start
  function     	: メイン関数
  Input Data	: none
  Output Data   : none
  Return Value	: REMOVABLE_RESIDENT_END (Unload可能常駐終了)
                  NO_RESIDENT_END (非常駐終了)
----------------------------------------------*/
int start(int argc,char * argv[])
{
        struct ThreadParam param;
	int i,j;
	
	if (argc < 0) { 
	  /* Unload 処理 */
	  return stop_usbloadf(argc,argv); 
	}
	
        /* 引数の解析 */
	for(i=0; i<argc; i++) {
	  for(j=0; argv[i][j]!='\0'; j++) {
	    if (argv[i][j] == '=') { argv[i][j]='\0'; j++; break; }
	  }
	  if (strcmp(argv[i],"load") == 0) {
	    if (strcmp(argv[i]+j,"ARGLOAD") == 0) { Load = ARGLOAD; }
	    if (strcmp(argv[i]+j,"APILOAD") == 0) { Load = APILOAD; }
	    if (strcmp(argv[i]+j,"APIREGI") == 0) { Load = APIREGI; }
	  }
	  if (strcmp(argv[i],"conffile") == 0) {
	    if (strcmp(argv[i]+j,"HOST") == 0) { Conffile = HOST; }
	    if (strcmp(argv[i]+j,"CD") == 0) { Conffile = CD; }
	  }
	  if (strcmp(argv[i],"driver") == 0) {
	    if (strcmp(argv[i]+j,"HOST") == 0) { Driver = HOST; }
	    if (strcmp(argv[i]+j,"CD") == 0) { Driver = CD; }
	  }
	}
	
        /* --- initialize thread --- */
        param.attr         = TH_C;
        param.entry        = setup_usbmload;
        param.initPriority = BASE_priority;
        param.stackSize    = 0x800;
        param.option       = 0;
        thid = CreateThread(&param);
        if( thid > 0 ) {
                StartThread(thid,0);
                return REMOVABLE_RESIDENT_END; /* 削除可能な常駐 */
        } else {
                return NO_RESIDENT_END;
        }
}

/* ---------------------------------------------
  Function Name	: stop_usbloadf
  function     	: Unloadのための停止処理(資源の解放)
  Input Data	: argc,argv
  Output Data   : none
  Return Value	: NO_RESIDENT_END (停止成功)
                  REMOVABLE_RESIDENT_END (停止失敗)
----------------------------------------------*/
static int stop_usbloadf(int argc, char* argv[])
{
  sceUsbmlDisable();
  sceUsbmlUnregisterLoadFunc();
  
  return NO_RESIDENT_END;
}

/* ---------------------------------------------
  Function Name	: setup_usbmload
  function     	: USBMLOADの設定をする関数
  Input Data	: none
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
void setup_usbmload(int arg)
{
        USBDEV_t device;
	struct ThreadInfo th_info;
	
	ReferThreadStatus(0,&th_info);
	printf("Thread priority:%d\n",th_info.currentPriority);
	
        sceCdInit(SCECdINIT);
	sceCdMmode(SCECdCD);  /* メディアによって変えること */
	
	sceUsbmlChangeThreadPriority(100);
	
	if (sceUsbmlRegisterLoadFunc(loadfunc) == USBML_NG) {
	  ExitThread();
	}
	//sceUsbmlUnregisterLoadFunc();
	
	switch(Load) 
	  {
	  case ARGLOAD:
	    break;
	  case APILOAD:
	    /* ***** API で設定ファイルを読ませる *****/
	    switch(Conffile)
	      {
	      case HOST:
		switch(Driver)
		  {
		  case HOST:
		    sceUsbmlLoadConffile("host1:/usr/local/sce/conf/usb/usbdrvho.cnf");
		    break;
		  case CD:
		    sceUsbmlLoadConffile("host1:/usr/local/sce/conf/usb/usbdrvcd.cnf");
		    break;
		  }		    
		break;
	      case CD:
		switch(Driver)
		  {
		  case HOST:
		    sceUsbmlLoadConffile("cdrom0:\\USBDRVHO.CNF;1");
		    break;
		  case CD:
		    sceUsbmlLoadConffile("cdrom0:\\USBDRVCD.CNF;1");
		    break;
		  }
		break;
	      }
	      break;
	  case APIREGI:
	    /* ***** API でドライバを登録 *****/
	    device.vendor   = WILDCARD_INT;
	    device.product  = WILDCARD_INT;
	    device.release  = WILDCARD_INT;
	    device.class    = 3;
	    device.subclass = 1;
	    device.protocol = 2;
	    device.category = "Mouse";
	    switch(Driver)
	      {
	      case HOST:
		device.dispname = "標準 USB マウス(HOST)";
		device.path     = "host1:/usr/local/sce/iop/sample/usb/usbmouse/usbmouse.irx";
		break;
	      case CD:
		device.dispname = "標準 USB マウス(CD)";
		device.path     = "cdrom0:\\USBMOUSE.IRX;1";
		break;
	      }
	    device.argc     = 0;
	    device.activate_flag = CAT_INACTIVE;
	    device.modid = -1;
	    sceUsbmlRegisterDevice(&device);
	    
	    device.vendor   = WILDCARD_INT;
	    device.product  = WILDCARD_INT;
	    device.release  = WILDCARD_INT;
	    device.class    = 3;
	    device.subclass = 1;
	    device.protocol = 1;
	    device.category = "Keyboard";
	    switch(Driver)
	      {
	      case HOST:
		device.dispname = "標準 USB キーボード(HOST)";
		device.path     = "host1:/usr/local/sce/iop/sample/usb/usbkeybd/usbkeybd.irx";
		break;
	      case CD:
		device.dispname = "標準 USB キーボード(CD)";
		device.path     = "cdrom0:\\USBKEYBD.IRX;1";
		break;
	      }
	    device.argc     = 0;
	    device.activate_flag = CAT_INACTIVE;
	    device.modid = -1;
	    sceUsbmlRegisterDevice(&device);
	    break;
	  }
	
	/* カテゴリのアクティベイト(カテゴリ毎のオートロードの許可) */
	sceUsbmlActivateCategory("Mouse");
	sceUsbmlActivateCategory("Keyboard");
	sceUsbmlActivateCategory("Ether");
	
	/* オートローダーを有効にする */
	sceUsbmlEnable();
	
	/* このスレッドはこの後で消滅する */
}


/* ---------------------------------------------
  Function Name	: loadfunc
  function     	: 外部ロード関数
  Input Data	: pop_devinfo(デバイス情報を取得する関数のポインタ)
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
#define AUTOLOAD_PARAM "lmode=AUTOLOAD"
#define ARGP_MAX       256

static void loadfunc(sceUsbmlPopDevinfo pop_devinfo)
{
  USBDEV_t *device;
  char argp[ARGP_MAX];
  int result;
  int modid;
  int i;
  int argp_len = 0;
  int argv_len = 0;
  ModuleStatus modstat;
  
  /* usbmload.irx のリングバッファからロードするモジュールの情報を取得 */
  while((device = pop_devinfo()) != NULL) {
    if (device->modid >= 0) {
      if (ReferModuleStatus(modid,&modstat) == KE_OK) {
	if (strcmp(modstat.name,device->modname) == 0) {
	  /* 同じIDかつ同じ名前のモジュールが存在するので,読み込まない */
	  continue;
	}
      }
    }
    
    /* 取得したモジュール情報の表示 */
    sjis2euc(device->dispname);
    printf("NAME    :%s\n",device->dispname);
    printf("VID     :%04X\n",device->vendor);
    printf("PID     :%04X\n",device->product);
    printf("USB-SPEC:%04X\n",device->release);
    printf("CLASS   :%02X\n",device->class);
    printf("SUBCLASS:%02X\n",device->subclass);
    printf("PROTCOL :%02X\n",device->protocol);
    printf("CATEGORY:%s\n",device->category);
    printf("PATH    :%s\n",device->path);
    
    /* モジュールに渡す引数の作成 */
    argp_len = 0;
    for(i=0; i<device->argc; i++) {
      argv_len = strlen(device->argv[i]) + 1;
      if ((argp_len + argv_len) > (ARGP_MAX - sizeof(AUTOLOAD_PARAM) - 1)) 
	{ break; }
      strcpy(argp+argp_len, device->argv[i]);
      argp_len += argv_len;
    }
    strcpy(argp+argp_len, AUTOLOAD_PARAM);
    argp_len += sizeof(AUTOLOAD_PARAM);
    
    /* モジュールをロード */
    modid = LoadStartModule(device->path, argp_len, argp, &result);
    
    printf("usbmload : LoadStartModule : %d\n",modid);
    if (modid >= 0) {
      printf("AUTOLOAD RESULT:%04Xh ",result);
      switch (result & RESIDENT_END_MASK) {
      case NO_RESIDENT_END:
	printf("[NO_RESIDENT_END]\n");
	break;
      case RESIDENT_END:
	printf("[RESIDENT_END]\n");
	break;
      case REMOVABLE_RESIDENT_END:
	printf("[REMOVABLE_RESIDENT_END]\n");
	break;	
      }
    }
    
    if (modid >= 0 && (result & RESIDENT_END_MASK)!=NO_RESIDENT_END) {
      /* 常駐できた */
      printf("LOADED \"%s\"\n",device->path);
      /* 常駐できたので,モジュールの情報をコピー */
      device->modid = modid;
      device->load_result = result;
      ReferModuleStatus(modid,&modstat);
      strcpy(device->modname,modstat.name);
    } else {
      /* 常駐できなかった */
      printf("NOT LOADED \"%s\"\n",device->path);
    }
  }
}


/* ---------------------------------------------
  Function Name	: sjis2euc
  function     	: 文字列にSJISコードがあれば,
                  EUCコードに変換する
  Input Data	: string(文字列)
  Output Data   : なし
  Return Value	: 変換したSJISコードの数
----------------------------------------------*/
static int sjis2euc(char *string)
{
  u_char *p = (u_char*)string;
  u_int first;
  u_int second;
  int count = 0;
  
  while( (*p != '\0') && (*p != '\n') && (*p != '\r') )
    {
      /* SJIS かどうかを判定 */
      first = *p;  /* 第１バイト判定 */
      if (((first >=   0) && (first <  0x80)) ||
	  ((first > 0x9f) && (first <  0xe0)) ||
	  ((first > 0xef) && (first <= 0xff)))
	{ p++; continue; }
      
      second = *(p+1);  /* 第２バイト判定 */
      if (((second >=   0) && (second <  0x40)) ||
	  (second == 0x7f) ||
	  ((second > 0xfc) && (second <= 0xff))) 
	{ p++; continue; }
      
      /* SJIS -> EUC 変換 */
      if ( second < 0x9f ) {
	if( second > 0x7f ) { -- second; }
	second += 0x61;
	
	if( first < 0xa0 ) { 
	  first = (first - 0x81) * 2 + 0xa1; 
	} else { 
	  first = (first - 0xe0) * 2 + 0xdf; 
	} 
      } else {
	second += 2;
	
	if( first < 0xa0 ) { 
	  first = (first - 0x81) * 2 + 0xa2; 
	} else { 
	  first = (first - 0xe0) * 2 + 0xe0; 
	}
      }
      
      *p = (u_char)first;
      *(p+1) = (u_char)second;
      
      p+=2;
      count++;
    }
  
  //printf("COUNT:%d\n",count);
  return count;
}

