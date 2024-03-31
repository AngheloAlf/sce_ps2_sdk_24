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
 *                             hid.c
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 */

#include <stdio.h>
#include <kernel.h>
#include <sys/file.h>
#include <usb.h>
#include <usbd.h>

#include "hid.h"
#include "usbdesc.h"

static int dump_HIDSUB_desc(int fd, int dev_id, UsbHidDescriptor *desc);
static void dump_report_descriptor(int fd, u_char * desc , int len);


/* ---------------------------------------------
  Function Name	: dump_HID_desc
  function     	: HIDディスクリプタのダンプ
  Input Data	: desc
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
void dump_HID_desc(int fd, int dev_id, void *p)
{
        UsbHidDescriptor *desc = p;
	u_short wDescriptorLength;
	int i;
	
        fdprintf(fd,"\nHID DESCRIPTOR\n");
	
	fdprintf(fd,"  bLength:                 %02Xh(%d)\n",
		 desc->bLength,desc->bLength);
	fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",
	       desc->bDescriptorType,desc->bDescriptorType);
	
	fdprintf(fd,"  bcdHID:                %02X%02Xh\n",
	       desc->bcdHID1,desc->bcdHID0);
	fdprintf(fd,"  bCountryCode:            %02Xh(%d)\n",
	       desc->bCountryCode,desc->bCountryCode);
	fdprintf(fd,"  bNumDescriptors:         %02Xh(%d)\n",
	       desc->bNumDescriptors,desc->bNumDescriptors);
	
	for (i=0;i<desc->bNumDescriptors;i++) {
	  fdprintf(fd,"  bDescriptorType:         %02Xh(%d)\n",
		   desc->SubDescriptorInfo[i].bDescriptorType,
		   desc->SubDescriptorInfo[i].bDescriptorType);
	  
	  wDescriptorLength = desc->SubDescriptorInfo[i].wDescriptorLength0 + 
	                      desc->SubDescriptorInfo[i].wDescriptorLength1 * 256;
	  fdprintf(fd,"  wDescriptorLength:     %04Xh(%d)\n",
		   wDescriptorLength, wDescriptorLength);
	}
	
	dump_HIDSUB_desc(fd,dev_id,desc);
}


/* ---------------------------------------------
  Function Name	: dump_HIDSUB_desc
  function     	: HID SUBディスクリプタのダンプ
  Input Data	: none
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static int dump_HIDSUB_desc(int fd, int dev_id, UsbHidDescriptor *desc)
{
  UsbDescPrivateData_t *pdata = sceUsbdGetPrivateData(dev_id);
  int i,r;
  void *reportdesc;
  int len;
  
  for (i=0;i<desc->bNumDescriptors;i++) {
    if (desc->SubDescriptorInfo[i].bDescriptorType
	== USB_DESCRIPTOR_TYPE_REPORT) 
      {
	r = sceUsbdGetReportDescriptor(dev_id,
				       pdata->last_cfg_num,
				       pdata->last_if_num,
				       &reportdesc,
				       &len);
	if (r == sceUsbd_NOERR) {
	  dump_report_descriptor(fd, reportdesc, len);
	}
      }
  }
  
  return 0;
}


/* ---------------------------------------------
  Function Name	: dump_report_descriptor
  function     	: REPORTディスクリプタのダンプ
  Input Data	: 
  Output Data   : none
  Return Value	: none
----------------------------------------------*/
static char* const ItemType[] = { "Main  ","Global","Local " };
static char* const Collection[] = { "Physical", "Application", "Logical" };
static char* const UsagePages00[] = {
  "Undefined",  /* 0x00 */
  "Generic Desktop Controls",
  "Simulation Controls",
  "VR Controls",
  "Sport Controls",
  "Game Controls",
  "Reserved",
  "Keyboard/Keypad",
  "LEDs",
  "Button",
  "Ordinal",
  "Telephony",
  "Consumer",
  "Digitizer",
  "Reserved",
  "PID Page",
  "Unicode",
  "Reserved",
  "Reserved",
  "Reserved",
  "Alphanumeric Display", /* 0x14 */
};
static char* const UsagePages80[] = {
  "Monitor pages",  /* 0x80 */
  "Monitor pages",
  "Monitor pages",
  "Monitor pages",
  "Power pages",
  "Power pages",
  "Power pages",
  "Power pages",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Bar Code Scanner page",
  "Scale Page",
  "Reserved",
  "Reserved",
  "Camera Control Page",
  "Arcade Page",   /* 0x91 */
};
static char* const GlobalItems[] = {
  "Usage Page",
  "Logical Minimum",
  "Logical Maximum",
  "Physical Minimum",
  "Physical Maximum",
  "Unit Exponent",
  "Unit",
  "Report Size",
  "Report ID",
  "Report Count",
  "Push",
  "Pop",
};
static char* const LocalItems[] = {
  "Usage",
  "Usage Minimum",
  "Usage Maximum",
  "Designator Index",
  "Designator Minimum",
  "Designator Maximum",
  "Reserved",
  "String Index",
  "String Minimum",
  "String Maximum",
  "Delimiter",
};

static void dump_report_descriptor(int fd, u_char * desc , int len)
{
  int i;
  int bSize,bType,bTag,bDataSize,bLongItemTag;
  u_char *bp = desc;
  int indent=0;
  u_int udata; /* unsigned data */
  int data;
  
  fdprintf(fd,"\nREPORT DESCRIPTOR\n");
  
  if (desc == NULL) { return; }
  
  while(len > 0) {
    if (*bp != 0xFE) {
      /*--- Short Item ---*/
      bSize = *bp & 0x03;
      if (bSize == 3) { bSize = 4; }
      bType = (*bp >> 2) & 0x03;
      bTag = (*bp >> 4) & 0x0F;
      
      for (i=bSize,udata=0; i > 0; i--) { udata = (udata<<8) + bp[i]; }

      if (bSize == 1) { data = (char)udata; }
      if (bSize == 2) { data = (short)udata; }
      if (bSize > 2)  { data = (int)udata; }

      fdprintf(fd,"  %s: ",ItemType[bType]);
      for(i=0; i < bSize+1; i++) 
	{ fdprintf(fd,"%02X ",*(bp+i)); }
      for(i=0; i < 5-bSize+1; i++) { fdprintf(fd,"   "); }
      
      if ((bType == 0x00) && (bTag == 0xC)) { indent-=2; } /* End Collection */
      for(i=0; i < indent; i++) { fdprintf(fd," "); }
      
      switch(bType) {
      case 0x00: /* Main */
	switch(bTag) {
	case 0x8: /* Input */
	  fdprintf(fd,"Input(%04Xh)\n",udata);
	  break;
	case 0x9: /* Output */
	  fdprintf(fd,"Output(%04Xh)\n",udata);
	  break;
	case 0xB: /* Feature */
	  fdprintf(fd,"Feature(%04Xh)\n",udata);
	  break;
	case 0xA: /* Collection */
	  fdprintf(fd,"Collection(%s)\n",Collection[udata]);
	  indent+=2;
	  break;
	case 0xC: /* End Collection */
	  fdprintf(fd,"End Collection\n");
	  break;
	default: /* Reserve */
	  fdprintf(fd,"Reserve\n");
	  break;
	}
	break;
      case 0x01: /* Global */
	if (bTag == 0x0) {
	  if (udata <= 0x14) {
	    fdprintf(fd,"Usage Page(%s)\n",UsagePages00[udata]);
	    break;
	  }
	  if ((udata >= 0x80) && (udata <= 0x91)) {
	    fdprintf(fd,"Usage Page(%s)\n",UsagePages80[udata-0x80]);
	  } else {
	    fdprintf(fd,"Usage Page(Reserved)\n");
	  }
	  break;
	}
	
	if ((bTag >= 0x1) && (bTag <= 0xB)) {
	  fdprintf(fd,"%s(%d)\n",GlobalItems[bTag],data);
	} else {
	  fdprintf(fd,"Reserved\n");
	}
	break;
      case 0x02: /* Local */
	if (bTag == 0x00) {  /* Usage */
	  fdprintf(fd,"%s(%04Xh)\n",LocalItems[bTag],udata);
	  break;
	}
	if (bTag <= 0xA) {
	  fdprintf(fd,"%s(%d)\n",LocalItems[bTag],udata);
	} else {
	  fdprintf(fd,"Reserved\n");
	}
	break;
      default:
	break;
      }
      len -= bSize+1;
      bp += bSize+1;
    } else {
      /*--- Long Item ---*/
      bDataSize = *(bp+1);
      bLongItemTag = *(bp+2);
      fdprintf(fd,"  LongItem(%02d): ",bLongItemTag);
      for(i=0; i < bDataSize+3; i++,bp++,len--) 
	{ fdprintf(fd,"%02X ",*bp); }
      fdprintf(fd,"\n");
    }
  }
}
