/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *      USB Mouse Class Driver
 *
 *                          Version 0.20
 *                          Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         usbmouse - usbmouse.h
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *      0.10            Jan, 2,2000     mka        Initial
 *      0.11            Jan,15,2000     hatto      Using SifRpc
 *      0.12            Apr,14,2000     mka        Using sceUsbdGetDeviceLocation()
 *      0.20            Apr,24,2000     fukunaga   Multi-MOUSE
 *
 */

#define SCE_RPC_USB_MOUSE	0x80000210

#define MAX_MOUSE_NUM 8  /* �ő�ڑ��}�E�X�� */
#define MOUSE_DATA_LEN 8 /* �ʏ�� 3 ���� 4 ����,�傫���m�ۂ��� */
#define RINGBUF_SIZE 3   /* 1�}�E�X����̃����O�o�b�t�@�̃T�C�Y */
