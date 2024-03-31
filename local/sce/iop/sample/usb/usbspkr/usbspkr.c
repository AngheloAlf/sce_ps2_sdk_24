/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*			Copyright (C) 2001 Sony Computer Entertainment
 *			All Right Reserved
 *
 * usbspkr.c - USB Speaker Sample Driver
 *
 * $Id: usbspkr.c,v 1.1 2001/12/01 10:35:34 fukunaga Exp $
 */

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <memory.h>
#include <sys/file.h>

#include <usb.h>
#include <usbd.h>

#include "audio.h"

ModuleInfo Module = { "SCE_USB_SPEAKER_SAMPLE", 0x0101 };

#if defined(sceUsbd_MULTI_ISOCH_TRANSFERS)
#define USE_MULTI_ISOCH_TRANSFERS
#endif

typedef struct {
	char ChunkID[4];	/* "RIFF" */
	int ChunkSize;		/* data size - (44 - 8) */
	char Format[4];		/* "WAVE" */

	char Subchunk1ID[4];	/* "fmt " */
	int Subchunk1Size;	/* 16 */
	short AudioFormat;	/* 1:PCM */
	short NumChannels;	/* 1:mono, 2:stereo, ... */
	int SampleRate;		/* samples/sec */
	int ByteRate;		/* bytes/sec */
	short BlockAlign;	/* bytes/sample * NumChannels */
	short BitsPerSample;	/* bits/sample */

	char Subchunk2ID[4];	/* "data" */
	int Subchunk2Size;	/* data size [byte] */
	u_char data[0];
} WAVE_HDR;

static int tid = 0;		/* Thread ID */
static int eid = 0;		/* EventFlag ID */
static int busy = 0;		/* busy flag */

static int interface_number;	/* idesc->bInterfaceNumber */
static int alternate_setting;	/* idesc->bAlternateSetting */
static int payload;		/* payload */
static int c_pipe;		/* control pipe */
static int o_pipe;		/* output pipe */

static char *fname = NULL;	/* file name */
static int fd = -1;		/* file descriptor */
static WAVE_HDR hdr;		/* WAVE file header */

static char *buf_base = NULL;	/* buffer area base */
static int buf_siz;		/* buffer arae size */
static int buf_len;		/* # of bytes of data in buffer */
static int buf_put;		/* fill index */
static int buf_get;		/* play index */
static int rate_base;		/* bytes/1ms (base) */
static int rate_frag;		/* 1000 * bytes/1ms (fragment) */
static int curr_frag;		/* current fragment */
static int done_frame;		/* output done frame counter */
static int done_sec;		/* output done sec counter */

#define READAHEAD_UNIT	1			/* 1 sec */
#define BUFFERING_TIME	(READAHEAD_UNIT * 8)	/* 8 sec */

#if defined(USE_MULTI_ISOCH_TRANSFERS)
#define FRAME_PER_REQ	sceUsbd_MAX_ISOCH_PACKETS
#define NREQ		4	/* 8 x 4 = 32 transfers */
#else
#define FRAME_PER_REQ	1
#define NREQ		32	/* 1 x 32 = 32 transfers */
#endif

/* Event Flag Pattern */
#define EFP_READY	0x01	/* ready to play */
#define EFP_DETACH	0x02	/* device is detached */
#define EFP_FILL	0x04	/* request for buffer fill */
#define EFP_ALL		0x07	/* (all) */

#define inform(args...)	(printf("usbspkr,%s: ", __FUNCTION__), \
			 printf(args), -1)

#define GET3BYTES(p)	((p)[0] | ((p)[1] << 8) | ((p)[2] << 16))

static int control_transfer(int step, int arg);
static void output_transfer(int fno);

static void control_done(int result, int count, void *arg){
	int step = (int)arg;

	if(result){
		inform("result=0x%x count=%d arg=%p\n", result, count, arg);
		return;
	}
	control_transfer(step + 1, 0);
}

static int control_transfer(int step, int arg){
	int r;

	switch(step){
	case 0:
		inform("Set Configuration (bConfigurationValue=%d)\n", arg);
		if(0 != (r = sceUsbdSetConfiguration(c_pipe,
				arg, control_done, (void *)step))){
			return(inform("SetConfiguration -> 0x%x\n", r));
		}
		break;
	case 1:
		inform("Set Interface"
			" (bInterfaceNumber=%d, bAlternateSetting=%d)\n",
				interface_number, alternate_setting);
		if(0 != (r = sceUsbdSetInterface(c_pipe,
				interface_number, alternate_setting,
				control_done, (void *)step))){
			return(inform("SetInterface -> 0x%x\n", r));
		}
		break;
	case 2:
		SetEventFlag(eid, EFP_READY);
		break;
	}
	return(0);
}

static int attach(int dev_id){
	UsbConfigurationDescriptor *cdesc;
	UsbInterfaceDescriptor *idesc;
	UsbTypeIFormatTypeDescriptor *fdesc;
	UsbEndpointDescriptor *edesc;
	u_char *p;
	int i;

	if(busy)
		return(inform("multiple units are not supported"));
	busy = 1;

	inform("Scanning descriptors for %d chan, %d bits/sample, %dHz\n",
		hdr.NumChannels, hdr.BitsPerSample, hdr.SampleRate);
	if(NULL == (cdesc = sceUsbdScanStaticDescriptor(dev_id, NULL,
			USB_DESCRIPTOR_TYPE_CONFIGURATION)))
		return(-1);

	idesc = (void *)cdesc;
	while(1){
		if(NULL == (idesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
				USB_DESCRIPTOR_TYPE_INTERFACE)))
			return(-1);
		if(idesc->bInterfaceClass != USB_CLASS_AUDIO)
			continue;
		if(idesc->bInterfaceSubClass != USB_SUBCLASS_AUDIOSTREAMING)
			continue;
		if(idesc->bNumEndpoints != 1)
			continue;
		fdesc = (void *)idesc;
fscan:		if(NULL == (fdesc = sceUsbdScanStaticDescriptor(dev_id, fdesc,
				USB_CS_INTERFACE)))
			continue;
		if(fdesc->bDescriptorSubType != USB_FORMAT_TYPE)
			goto fscan;
		if(fdesc->bFormatType != USB_FORMAT_TYPE_I)
			continue;
		if(fdesc->bNrChannels != hdr.NumChannels)
			continue;
		if(fdesc->bSubframeSize != ((hdr.BitsPerSample + 7) >> 3))
			continue;
		if(fdesc->bBitResolution != hdr.BitsPerSample)
			continue;
		p = &fdesc->SamplingFrequencyTables[0].tSamFreq0;
		if(fdesc->bSamFreqType == 0){	/* Continuous sampling freq. */
			if(hdr.SampleRate < GET3BYTES(p))
				continue;
			if(GET3BYTES(p + 3) < hdr.SampleRate)
				continue;
		}else for(i = 0; i < fdesc->bSamFreqType; i++, p += 3){
			if(hdr.SampleRate == GET3BYTES(p))
				break;
			if(i == fdesc->bSamFreqType - 1)
				continue;
		}
		if(NULL == (edesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
				USB_DESCRIPTOR_TYPE_ENDPOINT)))
			continue;
		if((edesc->bmAttribute & USB_ENDPOINT_TRANSFER_TYPE_BITS)
				!= USB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS)
			continue;
		if((edesc->bEndpointAddress & USB_ENDPOINT_DIRECTION_BITS)
				!= USB_ENDPOINT_DIRECTION_OUT)
			continue;
		break;
	}
	interface_number = idesc->bInterfaceNumber;
	alternate_setting = idesc->bAlternateSetting;
	payload = edesc->wMaxPacketSize0 | (edesc->wMaxPacketSize1 << 8);
	if(0 > (c_pipe = sceUsbdOpenPipe(dev_id, NULL)))
		return(-1);
	if(0 > (o_pipe = sceUsbdOpenPipeAligned(dev_id, edesc)))
		return(-1);
	inform("->attached\n");
	if(control_transfer(0, cdesc->bConfigurationValue))
		return(-1);
	return(0);
}

static int detach(int dev_id){
	busy = 0;
	SetEventFlag(eid, EFP_DETACH);
	inform("->detached\n");
	return(0);
}

static int probe(int dev_id){
	UsbInterfaceDescriptor *idesc = NULL;

	while(NULL != (idesc = sceUsbdScanStaticDescriptor(dev_id, idesc,
				USB_DESCRIPTOR_TYPE_INTERFACE)))
		if(idesc->bInterfaceClass == USB_CLASS_AUDIO
				&& idesc->bInterfaceSubClass
					== USB_SUBCLASS_AUDIOSTREAMING)
			return(1);
	return(0);
}

static sceUsbdLddOps usbspkr_ops = {
	NULL, NULL,
	"usbspkr",
	probe, attach, detach,
};

#if defined(USE_MULTI_ISOCH_TRANSFERS)
static void output_done(int result, sceUsbdMultiIsochronousRequest *req,
		void *arg){
	int old, i, n = (int)arg;

	if(result)
		inform("result=0x%x req=0x%p arg=%p\n", result, req, arg);
	for(i = 0; i < req->num_packets; i++)
		if(req->Packets[i].PSW)
			inform("PSW[%d]=0x%x\n", i, req->Packets[i].PSW);

#else	/* USE_MULTI_ISOCH_TRANSFERS */
static void output_done(int result, int count, void *arg){
	int old, n = (int)arg;

	if(result)
		inform("result=0x%x count=%d arg=%p\n", result, count, arg);

#endif	/* USE_MULTI_ISOCH_TRANSFERS */

	CpuSuspendIntr(&old);
	buf_len -= n;
	CpuResumeIntr(old);
	output_transfer(0);	/* without frame interval */
	if(1000 <= (done_frame += FRAME_PER_REQ)){
		++done_sec;
		SetEventFlag(eid, EFP_FILL);
		done_frame -= 1000;
	}
}

static void output_transfer(int fno){
	int r, n, f;

	if(buf_len <= 0){
		inform("data buffer underrun\n");
		return;
	}
#if defined(USE_MULTI_ISOCH_TRANSFERS)
	{
		sceUsbdMultiIsochronousRequest req;
		int i, total;

		bzero(&req, sizeof(req));
		req.buffer_base = buf_base + buf_get;
		req.relative_start_frame = fno;
		req.num_packets = sceUsbd_MAX_ISOCH_PACKETS;

		for(total = i = 0; i < sceUsbd_MAX_ISOCH_PACKETS; i++){
			n = rate_base;
			f = (curr_frag += rate_frag) / 1000;
			if(hdr.BlockAlign <= f){
				n += hdr.BlockAlign;
				curr_frag -= hdr.BlockAlign * 1000;
			}
			if(n & 3){
				curr_frag += (n & 3) * 1000;
				n &= ~3;
			}
			total += (req.Packets[i].len = n);
		}
		n = total;
		if(0 != (r = sceUsbdMultiIsochronousTransfer(o_pipe, &req,
				output_done, (void *)n))){
			inform("Multi ISOC Transfer -> 0x%x\n", r);
			return;
		}
	}
#else	/* USE_MULTI_ISOCH_TRANSFERS */
	n = rate_base;
	f = (curr_frag += rate_frag) / 1000;
	if(hdr.BlockAlign <= f){	/* adjust by fragmentation */
		n += hdr.BlockAlign;
		curr_frag -= hdr.BlockAlign * 1000;
	}
	if(n & 3){	/* output pipe is aligned pipe */
		curr_frag += (n & 3) * 1000;
		n &= ~3;
	}
	if(0 != (r = sceUsbdIsochronousTransfer(o_pipe,
			buf_base + buf_get, n, fno,
			output_done, (void *)n))){
		inform("ISOC Transfer -> 0x%x\n", r);
		return;
	}
#endif	/* USE_MULTI_ISOCH_TRANSFERS */
	if(buf_siz <= (buf_get += n))
		buf_get = 0;
}

static int fill_buffer(int fd){
	int old, n, l, r;

	if((l = buf_siz - buf_put) < (n = buf_siz - buf_len))
		n = l;
again:
	if(0 > (r = read(fd, buf_base + buf_put, n)))
		return(inform("read error\n"));
	else if(0 < r){
		CpuSuspendIntr(&old);
		buf_len += r;
		CpuResumeIntr(old);
		if(buf_siz <= (buf_put += r))
			buf_put = 0;
	}
	if(0 < (n -= r)){
		lseek(fd, sizeof(WAVE_HDR), SEEK_SET);
		goto again;
	}
	return(0);
}

static int read_wave_header(void){
	if(0 > (fd = open(fname, O_RDONLY)))
		return(inform("can't open %s\n", fname));
	if(sizeof(hdr) != read(fd, &hdr, sizeof(hdr)))
		return(inform("can't read header (fname=%s)\n", fname), -1);
	if(strncmp("RIFF", hdr.ChunkID, 4) != 0)
		return(inform("invalid ChunkID\n"));
	if(strncmp("WAVE", hdr.Format, 4) != 0)
		return(inform("not WAVE format\n"));
	if(strncmp("fmt ", hdr.Subchunk1ID, 4) != 0)
		return(inform("invalid Subchunk1ID\n"));
	if(hdr.Subchunk1Size != 16)
		return(inform("invalid Subchunk1Size\n"));
	if(hdr.AudioFormat != 1)
		return(inform("not PCM format\n"));
	if(strncmp("data", hdr.Subchunk2ID, 4))
		return(inform("invalid Subchunk2ID\n"));
	return(0);
}

static void thread(u_long arg){
	int r, i;
	u_long result;

	if(0 > read_wave_header())
		goto done;
	rate_base = hdr.ByteRate / 1000;
	rate_frag = hdr.ByteRate % 1000;

	buf_siz = hdr.ByteRate * BUFFERING_TIME;
	if(NULL == (buf_base = AllocSysMemory(0, buf_siz , NULL))){
		inform("no space\n");
		goto done;
	}

	if(0 != (r = sceUsbdRegisterLdd(&usbspkr_ops))){
		printf("usbspkr: sceUsbdRegisterLdd -> %d\n", r);
		goto done;
	}

restart:
	buf_len = buf_put = buf_get = 0;
	curr_frag = 0;

	if(fill_buffer(fd))
		goto done;

	while(KE_OK == (r = WaitEventFlag(eid, EFP_ALL, EW_OR | EW_CLEAR,
			&result))){
		if(result & EFP_READY){
			output_transfer(10);	/* play about 10ms after */
			for(i = 0; i < NREQ - 1; i++)
				output_transfer(0); /* without frame interval */
		}
		if(result & EFP_DETACH){
			lseek(fd, sizeof(WAVE_HDR), SEEK_SET);
			goto restart;
		}
		if(result & EFP_FILL){
			printf("%d:%02d:%02d\n", done_sec / 3600,
				(done_sec / 60) % 60, done_sec % 60);
			if(READAHEAD_UNIT <= buf_siz - buf_len)
				if(fill_buffer(fd))
					goto done;
		}
	}
	inform("WaitEventFlag -> %d\n", r);

done:
	while(1)
		SleepThread();
}

static void release_resources(void){
	if(fname != NULL){
		FreeSysMemory(fname);
		fname = NULL;
	}
	if(0 < tid){
		TerminateThread(tid);
		DeleteThread(tid);
		tid = 0;
	}
	if(0 < eid){
		DeleteEventFlag(tid);
		tid = 0;
	}
	if(0 <= fd){
		close(fd);
		fd = -1;
	}
	if(buf_base != NULL){
		FreeSysMemory(buf_base);
		buf_base = NULL;
	}
}

int start(int ac, char *av[]){
	struct ThreadParam tparam;
	struct EventFlagParam eparam;
	int r;

	if(ac != 2){
		printf("usbspkr: Usage: usbspkr <fname>\n");
		return(NO_RESIDENT_END);
	}
	printf("USB Speaker sample driver (%s ISOC)\n",
#if defined(USE_MULTI_ISOCH_TRANSFERS)
		"Mutiple");
#else
		"Single");
#endif
	if(NULL == (fname = AllocSysMemory(0, strlen(av[1]) + 1, NULL))){
		printf("usbspkr: no space\n");
		return(NO_RESIDENT_END);
	}
	strcpy(fname, av[1]);

	eparam.attr = EA_SINGLE;
	eparam.initPattern = 0;
	eparam.option = 0;
	if(0 > (r = CreateEventFlag(&eparam))){
		printf("usbspkr: CreateThread -> %d\n", r);
		release_resources();
		return(NO_RESIDENT_END);
	}
	eid = r;

	tparam.attr = TH_C;
	tparam.entry = thread;
	tparam.initPriority = USER_LOWEST_PRIORITY;
	tparam.stackSize = 1024;
	tparam.option = 0;
	if(0 > (r = CreateThread(&tparam))){
		printf("usbspkr: CreateThread -> %d\n", r);
		release_resources();
		return(NO_RESIDENT_END);
	}
	tid = r;
	if(KE_OK != (r = StartThread(tid, 0))){
		printf("usbspkr: StartThread -> %d\n", r);
		release_resources();
		return(NO_RESIDENT_END);
	}

	return(RESIDENT_END);
}
