/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *      USB Static Descriptor Printer
 *
 *                          Version 0.13
 *                          Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                             usbdesc.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *        0.10          Apr,28,2000     fukunaga   Initial
 *        0.11          July,21,2000    fukunaga   
 *        0.12          Jan,25,2001     fukunaga   
 *        0.13          May,16,2001     fukunaga   Report Descriptor
 */

#include <stdio.h>
#include <kernel.h>
#include <sys/file.h>
#include <memory.h>

#include <usb.h>
#include <usbd.h>

#include "usbdesc.h"
#include "hid.h"


ModuleInfo Module = {"USB_DESCRIPTOR_PRINTER", 0x0201 };

/*********************************************************
                           Type 
**********************************************************/

/*********************************************************
                         Structure
**********************************************************/

/*********************************************************
                     Common Variables
**********************************************************/
static int dumptotty = 0;


/*********************************************************
                         Macros
**********************************************************/
#define err(p, f, r)	if(r) printf("usbdesc%d: %s -> 0x%x\n", \
				(p)->number, (f), (r))


/*********************************************************
                         Prototype
**********************************************************/
static int usbdesc_probe(int dev_id);
static int usbdesc_attach(int dev_id);
static int usbdesc_detach(int dev_id);
static void  main_thread(void);
static void dump_device_desc(int fd, int dev_id, void *p);
static void dump_configration_desc(int fd, int dev_id, void *p);
static void dump_interface_desc(int fd, int dev_id, void *p);
static void dump_endpoint_desc(int fd, int dev_id, void *p);
static void dump_class_specific_desc(int fd, int dev_id, u_char *desc);


/*********************************************************
                      Descriptor Table
**********************************************************/
typedef struct {
  u_char bDescriptorType;
  void (*dump_descriptor)(int fd, int dev_id, void* desc);
} Descriptor_table_t;

Descriptor_table_t Descriptor_table[] = {
  { 0x01, dump_device_desc },
  { 0x02, dump_configration_desc },
  { 0x04, dump_interface_desc },
  { 0x05, dump_endpoint_desc },
  { 0x21, dump_HID_desc },
};

#define DESCRIPTOR_TABLE_SIZE (sizeof(Descriptor_table)/sizeof(Descriptor_table_t))


/*********************************************************
                      Program Start
**********************************************************/
static sceUsbdLddOps usbdesc_ops = {
	NULL, NULL,
	"usbdesc",
	usbdesc_probe,
	usbdesc_attach,
	usbdesc_detach,
};

#define BASE_priority  32


/* ---------------------------------------------
  Function Name	: start
  function     	: メイン関数
  Input Data	: none
  Output Data   : none
  Return Value	: 0(正常終了),not0(異常終了)
----------------------------------------------*/
int start(int argc, char *argv[]){
	int r;
	
	if( argc > 1 && argv[1][0] == '-' )
	    dumptotty = 1;
	if(0 != (r = sceUsbdRegisterLdd(&usbdesc_ops)))
		printf("usbdesc: sceUsbdRegisterLdd -> 0x%x\n", r);
	
	return(r);
}

/* ---------------------------------------------
  Function Name	: usbdesc_probe
  function     	: LDD検出処理関数
  Input Data	: dev_id(デバイスID)
  Output Data   : none
  Return Value	: 0(検出したデバイスはこのLDDに対応しない)
----------------------------------------------*/
static int usbdesc_probe(int dev_id) {        
        char* desc = NULL;
	int i;
	char filename[256];
	int fd;
	UsbDescPrivateData_t *pdata;
	
	pdata = AllocSysMemory(0,sizeof(UsbDescPrivateData_t),NULL);
	sceUsbdSetPrivateData(dev_id,pdata);
	
	if( dumptotty ) {
	    fd = open("tty0:" ,O_WRONLY);
	} else {
	    sprintf(filename, "host1:dev_id%d.dsc", dev_id);
	    fd = open(filename,O_WRONLY | O_CREAT);
	}
	
	if (fd == 0) {
	  printf("Cannot open file\n");
	  SleepThread();
	}
	
	printf("New device plug-in!!!\n");
	printf("DEVICE-ID:%d\n",dev_id);

	fdprintf(fd,"-------------------------------------------\n");

	while (1) {
	  if (NULL == (desc = sceUsbdScanStaticDescriptor(dev_id, desc,0)))
	    { break; }
	  for (i=0;i<DESCRIPTOR_TABLE_SIZE;i++) {
	    if (Descriptor_table[i].bDescriptorType == desc[1]) break;
	  }
	  if (i != DESCRIPTOR_TABLE_SIZE) {
	    Descriptor_table[i].dump_descriptor(fd,dev_id,desc);
	  } else {
	    dump_class_specific_desc(fd,dev_id,desc);
	  }
	  for(i=0;i<100000;i++);  /* wait for fdprintf */
	}
	
	fdprintf(fd,"-------------------------------------------\n");
	
	close(fd);
	
	printf("Dump end\n");
	
	FreeSysMemory(pdata);
	
	return 0;
}

/* ---------------------------------------------
  Function Name	: usbdesc_attach
  function     	: LDD接続処理関数
                  (usbdesc_probeの戻り値が1のときコールされる)
  Input Data	: dev_id(デバイスID)
  Output Data   : none
  Return Value	: -1(コンフィグレーション失敗)
----------------------------------------------*/
static int usbdesc_attach(int dev_id)
{
	return(0);
}

/* ---------------------------------------------
  Function Name	: usbdesc_detach
  function     	: LDD切断処理関数
  Input Data	: dev_id(デバイスID)
  Output Data   : none
  Return Value	: 0(正常終了)
----------------------------------------------*/
static int usbdesc_detach(int dev_id)
{
	return(0);
}

/* ---------------------------------------------
  Function Name	: dump_device_desc
  function     	: デバイスディスクリプタのダンプ
  Input Data	: desc
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void dump_device_desc(int fd, int dev_id, void *p)
{
        UsbDeviceDescriptor *desc = p;
	
        fdprintf(fd,"\nDEVICE DESCRIPTOR\n");
	
	fdprintf(fd,"  bLength:                 %02Xh(%d)\n",
		 desc->bLength,desc->bLength);
	fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",
	       desc->bDescriptorType,desc->bDescriptorType);

	fdprintf(fd,"  bcdUSB:                %04Xh\n",desc->bcdUSB);
	fdprintf(fd,"  bDeviceClass:            %02Xh(%d)\n",
		 desc->bDeviceClass,desc->bDeviceClass);
	fdprintf(fd,"  bDeviceSubClass:         %02Xh(%d)\n",
	       desc->bDeviceSubClass,desc->bDeviceSubClass);
	fdprintf(fd,"  bDeviceProtocol:         %02Xh(%d)\n",
	       desc->bDeviceProtocol,desc->bDeviceProtocol);
	fdprintf(fd,"  bMaxPacketSize0:         %02Xh(%d)\n",
	       desc->bMaxPacketSize0,desc->bMaxPacketSize0);
	fdprintf(fd,"  idVendor:              %04Xh\n",desc->idVendor);
	fdprintf(fd,"  idProduct:             %04Xh\n",desc->idProduct);
	fdprintf(fd,"  bcdDevice:             %04Xh\n",desc->bcdDevice);
	fdprintf(fd,"  iManufacturer:           %02Xh(%d)\n",
	       desc->iManufacturer,desc->iManufacturer);
	fdprintf(fd,"  iProduct:                %02Xh(%d)\n",
		 desc->iProduct,desc->iProduct);
	fdprintf(fd,"  iSerialNumber:           %02Xh(%d)\n",
	       desc->iSerialNumber,desc->iSerialNumber);
	fdprintf(fd,"  bNumConfigurations:      %02Xh(%d)\n",
	       desc->bNumConfigurations,desc->bNumConfigurations);
	
}

/* ---------------------------------------------
  Function Name	: dump_configration_desc
  function     	: コンフィグレーションディスクリプタのダンプ
  Input Data	: desc
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void dump_configration_desc(int fd, int dev_id, void *p)
{
        UsbConfigurationDescriptor *desc = p;
        u_short wTotalLength;
        UsbDescPrivateData_t *pdata;

        fdprintf(fd,"\nCONFIGURATION DESCRIPTOR\n");
	
	fdprintf(fd,"  bLength:                 %02Xh(%d)\n",
		 desc->bLength,desc->bLength);
	fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",
	       desc->bDescriptorType,desc->bDescriptorType);
	
	wTotalLength = desc->wTotalLength0 + desc->wTotalLength1 * 256;
	fdprintf(fd,"  wTotalLength:          %04Xh(%d)\n",
		 wTotalLength,wTotalLength);
	fdprintf(fd,"  bNumInterfaces:          %02Xh(%d)\n",
	       desc->bNumInterfaces,desc->bNumInterfaces);
	fdprintf(fd,"  bConfigurationValue:     %02Xh(%d)\n",
	       desc->bConfigurationValue,desc->bConfigurationValue);
	fdprintf(fd,"  iConfiguration:          %02Xh(%d)\n",
	       desc->iConfiguration,desc->iConfiguration);
	fdprintf(fd,"  bmAttribute:             %02Xh(%d)\n",
		 desc->bmAttribute,desc->bmAttribute);
	fdprintf(fd,"  MaxPower:                %02Xh(%d)\n",
		 desc->MaxPower,desc->MaxPower);
	
	pdata = sceUsbdGetPrivateData(dev_id);
	pdata->last_cfg_num = desc->bConfigurationValue;
}

/* ---------------------------------------------
  Function Name	: dump_interface_desc
  function     	: インターフェースディスクリプタのダンプ
  Input Data	: desc
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void dump_interface_desc(int fd, int dev_id, void *p)
{
        UsbDescPrivateData_t *pdata;
        UsbInterfaceDescriptor *desc = p;

        fdprintf(fd,"\nINTERFACE DESCRIPTOR\n");
  
	fdprintf(fd,"  bLength:                 %02Xh(%d)\n",
		 desc->bLength,desc->bLength);
	fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",
	       desc->bDescriptorType,desc->bDescriptorType);

	fdprintf(fd,"  bInterfaceNumber:        %02Xh(%d)\n",
	       desc->bInterfaceNumber,desc->bInterfaceNumber);
	fdprintf(fd,"  bAlternateSetting:       %02Xh(%d)\n",
	       desc->bAlternateSetting,desc->bAlternateSetting);
	fdprintf(fd,"  bNumEndpoints:           %02Xh(%d)\n",
	       desc->bNumEndpoints,desc->bNumEndpoints);
	fdprintf(fd,"  bInterfaceClass:         %02Xh(%d)\n",
	       desc->bInterfaceClass,desc->bInterfaceClass);
	fdprintf(fd,"  bInterfaceSubClass:      %02Xh(%d)\n",
	       desc->bInterfaceSubClass,desc->bInterfaceSubClass);
	fdprintf(fd,"  bInterfaceProtocol:      %02Xh(%d)\n",
	       desc->bInterfaceProtocol,desc->bInterfaceProtocol);
	fdprintf(fd,"  iInterface:              %02Xh(%d)\n",
		 desc->iInterface,desc->iInterface);
	
	pdata = sceUsbdGetPrivateData(dev_id);
	pdata->last_if_num = desc->bInterfaceNumber;
}

/* ---------------------------------------------
  Function Name	: dump_endpoint_desc
  function     	: エンドポイントディスクリプタのダンプ
  Input Data	: desc
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void dump_endpoint_desc(int fd, int dev_id, void *p)
{
        UsbEndpointDescriptor *desc = p;
        u_short wMaxPacketSize;
	char dmystr[128];
	
        fdprintf(fd,"\nENDPOINT DESCRIPTOR\n");
	
	fdprintf(fd,"  bLength:                 %02Xh(%d)\n",
		 desc->bLength,desc->bLength);
	fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",
	       desc->bDescriptorType,desc->bDescriptorType);
	
	if (desc->bEndpointAddress & 0x80) {
	  sprintf(dmystr,"In");
	} else {
	  sprintf(dmystr,"Out");
	}
	fdprintf(fd,"  bEndpointAddress:        %02Xh(%d) [%s]\n",
		 desc->bEndpointAddress,desc->bEndpointAddress,dmystr);
	
	switch(desc->bmAttribute & 0x03) {
	case 0:
	  sprintf(dmystr,"Control");
	  break;
	case 1:
	  sprintf(dmystr,"Isochronous");
	  break;
	case 2:
	  sprintf(dmystr,"Bulk");
	  break;
	case 3:
	  sprintf(dmystr,"Interrupt");
	  break;
	}
	fdprintf(fd,"  bmAttribute:             %02Xh(%d) [%s]\n",
		 desc->bmAttribute,desc->bmAttribute,dmystr);
	
	wMaxPacketSize = desc->wMaxPacketSize0 + desc->wMaxPacketSize1 * 256;
	fdprintf(fd,"  wMaxPacketSize:        %04Xh(%d)\n",
		 wMaxPacketSize,wMaxPacketSize);
	fdprintf(fd,"  bInterval:               %02Xh(%d)\n",
		 desc->bInterval,desc->bInterval);

}

/* ---------------------------------------------
  Function Name	: dump_class_specific_desc
  function     	: クラス固有ディスクリプタのダンプ
  Input Data	: desc
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static void dump_class_specific_desc(int fd, int dev_id, u_char *desc)
{
        int i;

        fdprintf(fd,"\nCLASS-SPECIFIC DESCRIPTOR\n");

	fdprintf(fd,"  bLength:                 %02Xh(%d)\n",desc[0],desc[0]);
	fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",desc[1],desc[1]);

	for(i=2;i<desc[0];i++) {
	  fdprintf(fd,"                           %02Xh(%d)\n",desc[i],desc[i]);
	}

}

