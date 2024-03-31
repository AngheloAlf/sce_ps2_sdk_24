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
 *  ���W���[����
 *  --------------------------------------------------------------------
 */
ModuleInfo Module = {"USB_AutoLoader_Starter", 0x0101};

static void
usb_autoload_setup(void)
{
  /* �J�e�S����o�^ */
  sceUsbmlActivateCategory("Ether");
  sceUsbmlActivateCategory("Modem");

  /* �I�[�g���[�h������ */
  sceUsbmlEnable();
}

int
start(int argc, char *argv[])
{
    struct ThreadParam param;
    int thid;

    /* �X���b�h�̏����� */
    param.attr		= TH_C;
    param.entry		= usb_autoload_setup;
    param.initPriority	= USER_LOWEST_PRIORITY - 3;
    param.stackSize	= 0x400;
    param.option	= 0;

    /* �X���b�h�̍쐬 */
    thid = CreateThread(&param);

    /* �X���b�h���X�^�[�g */
    if( thid > 0 ){
	StartThread( thid, 0 );
	return RESIDENT_END;
    }else{
	return NO_RESIDENT_END;
    }
}
