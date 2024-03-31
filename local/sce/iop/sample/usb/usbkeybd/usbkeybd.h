/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *      USB Keyboard Class Driver (for IOP)
 *
 *                          Version 0.10
 *                          Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                              usbkeybd.h
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *      0.10            Apr,13,2000     fukunaga   USB Keyboard sample
 *
 */


#define SCE_RPC_USB_KEYBD	0x80000211

#define MAX_KEYBD_NUM 8  /* �ő�ڑ��L�[�{�[�h�� */
#define KEY_DATA_LEN 16  /* �ʏ�͂W����,�傫���m�ۂ��� */
#define RINGBUF_SIZE 3   /* 1�L�[�{�[�h����̃o�b�t�@�̃T�C�Y */

#define MAC 1
#define WIN 2
#define CAPS_LED_TYPE MAC /* CAPS LOCK LED�̎d�l */
