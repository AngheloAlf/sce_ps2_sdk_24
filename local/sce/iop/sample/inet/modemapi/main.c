/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                           - modemapi -
 *
 *                        Version 1.03
 *                          Shift-JIS
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * main.c - test program for modem module
 *
 * $Id: main.c,v 1.3 2001/02/06 13:13:45 komaki Exp $
 */

/* Execution Sample:
 *
 *   dsidb R> reset
 *   dsidb R> mstart main.irx		# load this program
 *   dsidb R> mstart usbd.irx           # load USBD
 *   dsidb R> mstart inet.irx		# for sceInet{Alloc,Free}Mem()
 *   dsidb R> mstart rsaq.irx		# load modem driver
 *
 */



#include <kernel.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>

#include <inet/inet.h>
#include <inet/modem.h>

ModuleInfo Module = { "INET_test_program_5", 0x0101 };

#define dprintf(args...)	printf("main: " args)

static sceModemOps_t *cur_ops = NULL;
static u_char buf[8192];

#define CQ_SIZ		1024
#define CQ_MSK		(CQ_SIZ - 1)

typedef struct {
	int put, get, len;
	u_char dat[CQ_SIZ];
} CQ;

static CQ xmit_cq;

#define EFP_Xmit	0x10000
#define EFP_Start	0x20000
#define EFP_Stop	0x40000

static inline int set_gp(void){
	int gp;  
                        
	asm volatile ("move %0,$gp; la $gp,_gp" : "=r" (gp));
	return(gp);
}

static inline void resume_gp(int gp){
	asm volatile ("move $gp,%0" : : "r" (gp));
}

 void dump_byte(char *label, void *ptr, int len){
	u_char *p = (u_char *)ptr;
	int i;

	if(label != NULL){
		if(label[0] != '\0')
			printf("%s: ", label);
		for(i = 0; i < len; i++)
			printf("%02x%s", p[i], (i < len - 1)? " ": "\n");
	}else while(0 < len){
		for(i = 0; i < 16; i++)
			printf(((i < len)? "%02x ": "   "), p[i]);
		for(printf("  "), i = 0; i < 16 && i < len; i++)
			printf("%c", (0x20 <= p[i] && p[i] < 0x7f)? p[i]: '.');
		printf("\n"), p += 16, len -= 16;
	}
}

static void print_char(u_char ch){
	switch(ch){
	case 0x08: printf("\b"); break;
#if 0
	case 0x0a: printf("\\n\n"); break;
	case 0x0d: printf("\\r"); break;
#else
	case 0x0a: printf("\n"); break;
	case 0x0d: printf("\r"); break;
#endif
	default:
		if(ch < 0x20)
			printf("\\x%02x", ch);
		else
			printf("%c", ch);
		break;
	}
}

static void key_input(u_char ch){
	sceModemOps_t *ops = cur_ops;
	CQ *q = &xmit_cq;
	int old;

	CpuSuspendIntr(&old);
	if(q->len < CQ_SIZ)
		q->dat[q->put++ & CQ_MSK] = ch, ++q->len;
	CpuResumeIntr(old);
	if(ops)
		SetEventFlag(ops->evfid, EFP_Xmit);
}

static void xmit(sceModemOps_t *ops, CQ *q){
	int r, len, n, old;

	if(0 < (len = q->len)){
		if((n = CQ_SIZ - (q->get & CQ_MSK)) < len)
			len = n;
		if((n = ops->snd_len) < len)
			len = n;
		r = (*ops->send)(ops->priv, &q->dat[q->get & CQ_MSK], len);
#if 0
		dprintf("%s_send(%d bytes) -> %d\n",
			ops->module_name, len, r);
#endif
		if(0 < r){
			CpuSuspendIntr(&old);
			q->get += r, q->len -= r;
			CpuResumeIntr(old);
		}
	}
}

static void test_thread(void){
	sceModemOps_t *ops = cur_ops;
	int r, i, started;
	u_long result;

	if(ops == NULL)
		return;

	r = (*ops->start)(ops->priv, 0);
	dprintf("%s_start() -> %d\n", ops->module_name, r);
	if(r < 0)
		return;
	started = 1;

	while(1){
		if(KE_OK != (r = WaitEventFlag(ops->evfid,
				(sceModemEFP_StartDone
					| sceModemEFP_PlugOut
					| sceModemEFP_Connect
					| sceModemEFP_Disconnect
					| sceModemEFP_Ring
					| sceModemEFP_Recv
					| sceModemEFP_Send
					| EFP_Start
					| EFP_Stop
					| EFP_Xmit),
				EW_OR | EW_CLEAR,&result))){
			dprintf("WaitEventFlag failed (%d)\n", r);
			return;
		}

		if(result & sceModemEFP_StartDone){
			dprintf("%s -> StartDone\n", ops->module_name);
			dprintf("Vendor=%s Device=%s\n",
				ops->vendor_name, ops->device_name);
#if 0
{
			int i;

			for(i = 0; i < sizeof(buf); i++)
				buf[i] = i;
			r = (*ops->send)(ops->priv, buf, sizeof(buf));
			dprintf("%s_send() -> %d\n", ops->module_name, r);
			if(0 < r)
				dump_byte(NULL, buf, r);
}
#endif
#if 0
			r = (*ops->send)(ops->priv, "AT\r", 3);
			dprintf("%s_send(\"AT\\r\") -> %d\n",
				ops->module_name, r);
#endif
		}

		if(result & sceModemEFP_PlugOut){
			dprintf("%s -> PlugOut\n", ops->module_name);
			break;
		}

		if(result & sceModemEFP_Connect)
			dprintf("%s -> Connect\n", ops->module_name);

		if(result & sceModemEFP_Disconnect){
			dprintf("%s -> Disconnect\n", ops->module_name);
			break;
		}

		if(result & sceModemEFP_Ring)
			dprintf("%s -> Ring\n", ops->module_name);

		if(result & sceModemEFP_Recv){
#if 0
			dprintf("%s -> Recv\n", ops->module_name);
			r = (*ops->recv)(ops->priv, buf, sizeof(buf));
			dprintf("%s_recv() -> %d\n", ops->module_name, r);
			if(0 < r)
				dump_byte(NULL, buf, r);
#else
			r = (*ops->recv)(ops->priv, buf, sizeof(buf));
			if(r < 0)
				dprintf("%s_recv() -> %d\n",
					ops->module_name, r);
			else
				for(i = 0; i < r; i++)
					print_char(buf[i]);
#endif
		}

		if(result & EFP_Start){
			if(started == 0){
				r = (*ops->start)(ops->priv, 0);
				dprintf("%s_start() -> %d\n",
					ops->module_name, r);
			}
			started = 1;
		}

		if(result & EFP_Stop){
			if(started != 0){
				r = (*ops->stop)(ops->priv, 0);
				dprintf("%s_stop() -> %d\n",
					ops->module_name, r);
			}
			started = 0;
		}

		if(result & sceModemEFP_Send){
#if 0
			dprintf("%s -> Send\n", ops->module_name);
#else
			xmit(ops, &xmit_cq);
#endif
		}

		if(result & EFP_Xmit)
			xmit(ops, &xmit_cq);
	}

	r = (*ops->stop)(ops->priv, 0);
	dprintf("%s_stop() -> %d\n", ops->module_name, r);
}

static int control(int code, void *ptr, int len){
	sceModemOps_t *ops = cur_ops;
	int r;

	r = (*ops->control)(ops->priv, code, ptr, len);
	return(r);
}

static void show_int(char *label, int code){
	u_int val;
	int r;

	if(label)
		printf(" %-10s", label);
	if(0 <= (r = control(code, &val, sizeof(val))))
		printf("0x%08x (%u)", val, val);
	else
		printf("[r=%d]", r);
	printf("\n");
}

static void show_stats(void){
	printf("----- modem status ----\n");
	show_int("thpri", sceModemCC_GET_THPRI);
	show_int("if_type", sceModemCC_GET_IF_TYPE);
	show_int("rx", sceModemCC_GET_RX_COUNT);
	show_int("tx", sceModemCC_GET_TX_COUNT);

	show_int("oe", sceModemCC_GET_OE_COUNT);
	show_int("pe", sceModemCC_GET_PE_COUNT);
	show_int("fe", sceModemCC_GET_FE_COUNT);
	show_int("bo", sceModemCC_GET_BO_COUNT);
	show_int("param", sceModemCC_GET_PARAM);
	show_int("line", sceModemCC_GET_LINE);
}

static void kbd_thread(void){
	sceModemOps_t *ops;
	int fd;
	u_char ch;

	if(0 > (fd = open("tty1:", O_RDONLY))){
		printf("can not open tty1:\n");
		return;
	}
	while(0 < read(fd, &ch, sizeof(ch))){
		if(ch != '~')
			key_input(ch);
		else{
			if(0 >= read(fd, &ch, sizeof(ch)))
				break;
			switch(ch){
			case '~':
				key_input(ch);
				break;
			case '?':
				show_stats();
				break;
			case '+':
				if(NULL != (ops = cur_ops))
					SetEventFlag(ops->evfid, EFP_Start);
				break;
			case '-':
				if(NULL != (ops = cur_ops))
					SetEventFlag(ops->evfid, EFP_Stop);
				break;
			default:
				key_input('~');
				key_input(ch);
				break;
			}
		}
	}
	close(fd);
}

static int start_thread(void (*func)(void)){
	struct ThreadParam thread_param;
	int thread_id, r;

	thread_param.attr = TH_C;
	thread_param.entry = func;
	thread_param.initPriority = USER_LOWEST_PRIORITY;
	thread_param.stackSize = 0x4000;
	thread_param.option = 0;
	if(0 >= (thread_id = CreateThread(&thread_param))){
		dprintf("CreateThread (%d)\n", thread_id);
		return(-1);
	}
	if(KE_OK != (r = StartThread(thread_id, 0))){
		dprintf("StartThread (%d)\n", r);
		DeleteThread(thread_id);
		return(-1);
	}
	return(0);
}

int sceModemRegisterDevice(sceModemOps_t *ops){
	int gp = set_gp(), r = 0;
	struct EventFlagParam event_param;

	if(cur_ops)
		r = sceINETE_BUSY;
	else{
		event_param.attr = EA_SINGLE;
		event_param.initPattern = 0;
		event_param.option = 0;
		if(0 >= (ops->evfid = CreateEventFlag(&event_param))){
			r = ops->evfid;
			goto done;
		}
		if(0 > start_thread(test_thread)){
			DeleteEventFlag(ops->evfid);
			r = -1;
			goto done;
		}
		cur_ops = ops;
	}
done:	dprintf("%s: r=%d\n", __FUNCTION__, r);
	resume_gp(gp);
	return(r);
}

int sceModemUnregisterDevice(sceModemOps_t *ops){
	int gp = set_gp(), r = 0;

	DeleteEventFlag(ops->evfid);
	cur_ops = NULL;
	dprintf("%s: r=%d\n", __FUNCTION__, r);
	resume_gp(gp);
	return(r);
}

int sceModemReserved(void){
	return(sceINETE_INVALID_CALL);
}

int start(int ac, char *av[]){
	extern libhead modem_entry;
	int r;

	if((r = RegisterLibraryEntries(&modem_entry)) != 0){
		dprintf("RegisterLibraryEntries (%d)\n", r);
		return(NO_RESIDENT_END);
	}
	start_thread(kbd_thread);
	return(RESIDENT_END);
}
