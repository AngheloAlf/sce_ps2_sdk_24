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

#define MAX_KEYBD_NUM 8  /* 最大接続キーボード数 */
#define KEY_DATA_LEN 16  /* 通常は８だが,大きく確保する */
#define RINGBUF_SIZE 3   /* 1キーボード当りのバッファのサイズ */

#define MAC 1
#define WIN 2
#define CAPS_LED_TYPE MAC /* CAPS LOCK LEDの仕様 */
