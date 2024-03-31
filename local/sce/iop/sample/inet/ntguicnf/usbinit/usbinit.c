/* SCEI CONFIDENTIAL
 * "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *	GUI Network Setting Application Sample
 *		USB AutoLoader Starter
 *
 *                          Version 1.2
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *	                 All Rights Reserved.
 *
 *                          usbinit.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2001.06.01      tetsu      Beta Release
 *        1.2           2001.07.29      tetsu      First Ver. Release
 */
#include <kernel.h>
#include <usbmload.h>

/*  --------------------------------------------------------------------
 *  モジュール名
 *  --------------------------------------------------------------------
 */
ModuleInfo Module = {"USB_AutoLoader_Starter", 0x0101};

static void
usb_autoload_setup(void)
{
  /* カテゴリを登録 */
  sceUsbmlActivateCategory("Ether");
  sceUsbmlActivateCategory("Modem");

  /* オートロードを許可 */
  sceUsbmlEnable();
}

int
start(int argc, char *argv[])
{
    struct ThreadParam param;
    int thid;

    /* スレッドの初期化 */
    param.attr		= TH_C;
    param.entry		= usb_autoload_setup;
    param.initPriority	= USER_LOWEST_PRIORITY - 3;
    param.stackSize	= 0x400;
    param.option	= 0;

    /* スレッドの作成 */
    thid = CreateThread(&param);

    /* スレッドをスタート */
    if( thid > 0 ){
	StartThread( thid, 0 );
	return RESIDENT_END;
    }else{
	return NO_RESIDENT_END;
    }
}
