/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *      USB Mouse Class Driver
 *
 *                          Version 0.70
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
 *      0.50            Nov,9,2000      fukunaga   for Release2.1
 *      0.60            Mar,22,2001     fukunaga   
 *      0.61            May,30,2001     fukunaga   Change start()
 *      0.70            July,12,2001    fukunaga   Unload
 */

#define SCE_RPC_USB_MOUSE	0x80000210

#define MAX_MOUSE_NUM 8  /* 最大接続マウス数 */
#define MOUSE_DATA_LEN 8 /* 通常は 3 から 4 だが,大きく確保する */
#define RINGBUF_SIZE 3   /* 1マウス当りのリングバッファのサイズ */
