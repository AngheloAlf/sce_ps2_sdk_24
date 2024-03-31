/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                         - ping -
 *
 *                        Version 1.04
 *                          Shift-JIS
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * ping.c - send ICMP ECHO_REQUEST packets
 *
 * $Id: ping.c,v 1.8 2001/08/17 05:13:39 ksh Exp $
 */

#if 0
#define HDD_ETHERNET
#endif

#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <string.h>
#include <kernel.h>
#include <sys/file.h>

#if defined(REMOVABLE_RESIDENT_END) && !defined(DISABLE_MODULE_UNLOAD)
#define	 ENABLE_MODULE_UNLOAD
#endif

#if defined(USE_LOCAL_INET_HEADERS)
#include "inet.h"
#include "in.h"
#include "ip.h"
#include "icmp.h"
#include "dns.h"
#else	/* USE_LOCAL_INET_HEADERS */
#include <inet/inet.h>
#include <inet/in.h>
#include <inet/ip.h>
#include <inet/icmp.h>
#include <inet/dns.h>
#endif	/* USE_LOCAL_INET_HEADERS */

#ifdef HDD_ETHERNET
#include <poweroff.h>
#endif

ModuleInfo Module = { "INET_ping_client", 0x0101 };

#define DEFAULT_TTY_NUMBER	0

static struct common {
	int count;
	int size;

	int cid;
	int main_evfid;
	int time_evfid;
	int rtid;
	int stid;
	int atid;
#if defined(ENABLE_MODULE_UNLOAD)
	int pwid;
#endif
	u_char *rbuf;
	u_char *sbuf;
	int alarmed;
	int tty_num;
	int tty_fd;

	sceInetAddress_t hostaddr;
	char hostname[1024];
	int bufsiz;
	struct SysClock sys_clock;
	u_short ping_id;
	int n_send;
	int n_recv;
	int n_dup;

	int dms_total;
	int dms_min;
	int dms_max;
} common;

#define RECV_DONE	0x01
#define SEND_DONE	0x02
#define ABORT_DONE	0x04

void dump_byte(void *ptr, int len){
	u_char *p = ptr;
	int i;

	while(0 < len){
		for(i = 0; i < 16; i++)
			printf(((i < len)? "%02x ": "   "), p[i]);
		for(printf("  "), i = 0; i < 16 && i < len; i++)
			printf("%c", (0x20 <= p[i] && p[i] < 0x7f)? p[i]: '.');
		printf("\n"), p += 16, len -= 16;
	}
}

static int usage(void){
	printf("Usage: ping [<option>...] <hostname>\n");
	printf("  <option>:\n");
	printf("    -c <count>\n");
	printf("    -s <size>\n");
	printf("    -tty <N>\n");
	return(-1);
}

static int scan_number(char *str, int *pv){
	char *s = str;
	int value, v, base = 10;

	if(*s == '0' && *(s + 1) != '\0')
		if(base = 8, *++s == 'x')
			++s, base = 16;
	if(*s == '\0')
		goto err;
	for(value = 0; *s; s++, value = value * base + v){
		if('0' <= *s && *s <= '9')
			v = *s - '0';
		else if('a' <= *s && *s <= 'f')
			v = *s - 'a' + 10;
		else
			goto err;
		if(base <= v)
			goto err;
	}
	*pv = value;
	return(0);
err:	printf("%s: %s - invalid digit\n", __FUNCTION__, str);
	return(-1);
}

static void *alloc_memory(int size){
	void *ptr;
	int old;

	CpuSuspendIntr(&old);
	ptr = AllocSysMemory(0, size, NULL);
	CpuResumeIntr(old);
	if(NULL == ptr)
		printf("ping: AllocSysMemory(%d) no space\n", size);
	else
		bzero(ptr, size);
	return(ptr);
}

static int create_event_flag(void){
	struct EventFlagParam eparam;
	int r;

	eparam.attr = EA_SINGLE;
	eparam.initPattern = 0;
	eparam.option = 0;
	if(0 >= (r = CreateEventFlag(&eparam)))
		printf("ping: CreateEventFlag (%d)\n", r);
	return(r);
}

static int start_thread(void (*func)(u_long arg), void *arg){
	struct ThreadParam tparam;
	int tid, r;

	tparam.attr = TH_C;
	tparam.entry = func;
	tparam.initPriority = USER_LOWEST_PRIORITY;
	tparam.stackSize = 0x4000;	/* 16KB */
	tparam.option = 0;
	if(0 >= (tid = CreateThread(&tparam))){
		printf("ping: CreateThread (%d)\n", tid);
		return(tid);
	}
	if(KE_OK != (r = StartThread(tid, (u_long)arg))){
		printf("ping: StartThread (%d)\n", r); 
		DeleteThread(tid);
		return(r);
	}
	return(tid);
}

static int open_raw(void){
	sceInetParam_t iparam;
	int r, cid;

	bzero(&iparam, sizeof(iparam));
	iparam.type = sceINETT_RAW;
	iparam.local_port = IP_PROTO_ICMP;
	sceInetName2Address(0, &iparam.remote_addr, NULL, 0, 0);
	iparam.remote_port = sceINETP_ANY;
	if(0 > (cid = sceInetCreate(&iparam))){
		printf("ping: sceInetCreate -> %d\n", cid);
		return(cid);
	}
	if(0 > (r = sceInetOpen(cid, 0))){
		printf("ping: sceInetOpen -> %d\n", r);
		return(r);
	}
	return(cid);
}

static u_int alarm_handler(void *arg){
	struct common *com = arg;
	int r, gp;
                        
	asm volatile ("move %0,$gp; la $gp,_gp" : "=r" (gp));
	if(KE_OK != (r = iSetEventFlag(com->time_evfid, 0x01)))
		printf("ping: iSetEventFlag (%d)\n", r);
	asm volatile ("move $gp,%0" : : "r" (gp));
	return(com->sys_clock.low);
}

static void release_resources(struct common *com){
	int r, old;

	DelayThread(100 * 1000);
	if(0 < com->cid){
		if(0 > (r = sceInetAbort(com->cid, 0)))
			printf("ping: sceInetAbort (%d)\n", r);
		DelayThread(100 * 1000);
		if(0 > (r = sceInetClose(com->cid, 0)))
			printf("ping: sceInetClose (%d)\n", r);
	}
	if(0 < com->rtid){
		(void)TerminateThread(com->rtid);
		if(KE_OK != (r = DeleteThread(com->rtid)))
			printf("ping: DeleteThread (%d)\n", r);
	}
	if(0 < com->stid){
		(void)TerminateThread(com->stid);
		if(KE_OK != (r = DeleteThread(com->stid)))
			printf("ping: DeleteThread (%d)\n", r);
	}
	if(0 < com->atid){
		(void)TerminateThread(com->atid);
		if(KE_OK != (r = DeleteThread(com->atid)))
			printf("ping: DeleteThread (%d)\n", r);
	}
	if(com->alarmed)
		if(KE_OK != (r = CancelAlarm(alarm_handler, com)))
			printf("ping: CancelAlarm (%d)\n", r);
	if(NULL != com->rbuf){
		CpuSuspendIntr(&old);
		r = FreeSysMemory(com->rbuf);
		CpuResumeIntr(old);
		if(KE_OK != r)
			printf("ping: FreeSysMemory (%d)\n", r);
	}
	if(NULL != com->sbuf){
		CpuSuspendIntr(&old);
		r = FreeSysMemory(com->sbuf);
		CpuResumeIntr(old);
		if(KE_OK != r)
			printf("ping: FreeSysMemory (%d)\n", r);
	}
	if(0 < com->main_evfid)
		if(KE_OK != (r = DeleteEventFlag(com->main_evfid)))
			printf("ping: DeleteEventFlag (%d)\n", r);
	if(0 < com->time_evfid)
		if(KE_OK != (r = DeleteEventFlag(com->time_evfid)))
			printf("ping: DeleteEventFlag (%d)\n", r);
	if(0 <= com->tty_fd)
		close(com->tty_fd);
}

u_long sum16asm(u_long v, u_char *p, long len);
asm("	.globl sum16asm			");
asm("	.ent sum16asm			");
asm("sum16asm:				");
asm("	move	$2,$4			");
asm("	lhu	$8,0($5)		");
asm("	srl	$6,$6,4			");
asm("	beq	$0,$6,sum16asm_end	");
asm("	.set	noreorder		");
asm("sum16asm_loop:			");
asm("	addu	$5,16			");
asm("	addu	$6,-1			");
asm("	addu	$2,$8			");
asm("	lhu	$9,0-14($5)		");
asm("	lhu	$10,0-12($5)		");
asm("	lhu	$11,0-10($5)		");
asm("	lhu	$12,0-8($5)		");
asm("	lhu	$13,0-6($5)		");
asm("	lhu	$14,0-4($5)		");
asm("	lhu	$15,0-2($5)		");
asm("	lhu	$8,0($5)		");
asm("	addu	$2,$9			");
asm("	addu	$2,$10			");
asm("	addu	$2,$11			");
asm("	addu	$2,$12			");
asm("	addu	$2,$13			");
asm("	addu	$2,$14			");
asm("	bne	$0,$6,sum16asm_loop	");
asm("	addu	$2,$15			");
asm("	.set	reorder			");
asm("sum16asm_end:			");
asm("	j	$31			");
asm("	.end	sum16asm		");

u_short sum16(register u_long v, register u_char *p, long len){
	if(((long)p & 1) != 0)
		return(0);
	if(32 <= len){
		switch(((long)p >> 1) & 7){
		case 1: v += *((u_short *)p)++, len -= 2;
		case 2: v += *((u_short *)p)++, len -= 2;
		case 3: v += *((u_short *)p)++, len -= 2;
		case 4: v += *((u_short *)p)++, len -= 2;
		case 5: v += *((u_short *)p)++, len -= 2;
		case 6: v += *((u_short *)p)++, len -= 2;
		case 7: v += *((u_short *)p)++, len -= 2;
		}
		v = sum16asm(v, p, len & ~0xf);
		p += len & ~0xf;
		len &= 0xf;
	}
	for( ; 1 < len; len -= 2)
		v += *((u_short *)p)++;
	if((len & 1) != 0)
		v += *((u_char *)p + 0) << 0;	/* only LITTLE_ENDIAN */
	while(v >> 16)
		v = (u_short)v + (v >> 16);
	return(v);
}

typedef struct record {
	int seq, nrcv;
	struct SysClock sys_clock;
} RECORD;

#define RECORD_SIZE	256
#define RECORD_MASK	(RECORD_SIZE - 1)

static RECORD records[RECORD_SIZE];

static void ping_recv(u_long arg){
	struct common *com = (void *)arg;
	char tmp[128];
	int r, flags;
	IP_HDR *ip = (IP_HDR *)com->rbuf;
	ICMP_HDR *icmp = (ICMP_HDR *)(ip + 1);
	RECORD rec, *rp = &rec, *rp0;
	struct SysClock sys_clock;
	sceInetAddress_t addr;
	int sec, usec, dms;

	if(0 != sceInetAddress2String(tmp, sizeof(tmp), &com->hostaddr))
		strcpy(tmp, "???");
	printf("PING %s (%s): %d data bytes (I%dTTY)\n",
		com->hostname, tmp, com->size, com->tty_num);
loop:
	if(0 > (r = sceInetRecv(com->cid, com->rbuf, com->bufsiz, &flags, -1)))
		goto done;
	GetSystemTime(&sys_clock);

#if 0
	printf("ping: sceInetRecv %d -> %d\n", com->bufsiz, r);
	if(0 < r){
		dump_byte(com->rbuf, r);
		printf("\n");
	}
#endif
	if(r < sizeof(*ip) + sizeof(*icmp))
		goto loop;
	ip_ntoh(ip);
	if(ip->proto != IP_PROTO_ICMP)
		goto loop;
	icmp_ntoh(icmp);
	if(icmp->type != ICMP_TYPE_ECHO_REPLY)
		goto loop;
	if(icmp->id != com->ping_id)
		goto loop;
	++com->n_recv;
	printf("%d bytes", r - sizeof(*ip));
	bzero(&addr, sizeof(addr));
	bcopy(&ip->src, addr.data, sizeof(ip->src));
	if(0 != sceInetAddress2String(tmp, sizeof(tmp), &addr))
		strcpy(tmp, "???");
	printf(" from %s:", tmp);
	printf(" icmp_seq=%d", icmp->seq);
	printf(" ttl=%d", ip->ttl);
	printf(" time=");
	bcopy(rp0 = &records[icmp->seq & RECORD_MASK], rp, sizeof(*rp));
	if(rp->seq == icmp->seq){
		sys_clock.hi -= rp->sys_clock.hi;
		if(sys_clock.low < rp->sys_clock.low)
			--sys_clock.hi;
		sys_clock.low -= rp->sys_clock.low;
		SysClock2USec(&sys_clock, &sec, &usec);
		usec += sec * 1000000;
		dms = (usec + 50) / 100;
		if(com->dms_min <= 0 || dms < com->dms_min)
			com->dms_min = dms;
		if(com->dms_max < dms)
			com->dms_max = dms;
		printf("%d.%d", dms / 10, dms % 10);
		if(0 < rp0->nrcv++){
			printf(" (DUP!)");
			--com->n_recv;
			++com->n_dup;
		}else
			com->dms_total += dms;
	}else{
		printf("?.?");
	}
	printf(" ms\n");
	if(com->count == 0 || com->n_recv < com->count)
		goto loop;
done:	SetEventFlag(com->main_evfid, RECV_DONE);
	while(1)
		SleepThread();
}

static void ping_send(u_long arg){
	struct common *com = (void *)arg;
	int r, seq = 0, flags = 0;
	u_long result;
	IP_HDR *ip = (IP_HDR *)com->sbuf;
	ICMP_HDR *icmp = (ICMP_HDR *)(ip + 1);
	RECORD *rp;

	bcopy(com->hostaddr.data, &ip->dst, sizeof(ip->dst));
	ip->proto = IP_PROTO_ICMP;
	ip->ttl = IP_TTL_DEFAULT;
	ip_hton(ip);
loop:
	icmp->type = ICMP_TYPE_ECHO_REQUEST;
	icmp->code = 0;
	icmp->id = com->ping_id;
	icmp->seq = seq;
	icmp_hton(icmp);
	icmp->chksum = ~sum16(icmp->chksum = 0, (u_char *)icmp,
		sizeof(*icmp) + com->size);
	if(0 > (r = sceInetSend(com->cid, com->sbuf, com->bufsiz, &flags, 0)))
		goto done;

	++com->n_send;
	rp = &records[seq & RECORD_MASK];
	rp->seq = seq;
	rp->nrcv = 0;
	GetSystemTime(&rp->sys_clock);

#if 0
	printf("ping: sceInetSend %d -> %d\n", com->bufsiz, r);
	if(0 < r){
		dump_byte(com->sbuf, r);
		printf("\n");
	}
#endif

	if(++seq, com->count == 0 || seq < com->count){
		if(KE_OK != (r = WaitEventFlag(com->time_evfid, 0x01,
				EW_OR | EW_CLEAR, &result)))
			printf("ping: WaitEventFlag (%d)\n", r);
		goto loop;
	}
done:	SetEventFlag(com->main_evfid, SEND_DONE);
	while(1)
		SleepThread();
}

static void ping_abort(u_long arg){
	struct common *com = (void *)arg;
	int r;
	u_char ch;
	char tmp[10];

	sprintf(tmp, "tty%d:", com->tty_num);
	if(0 > (com->tty_fd = open(tmp, O_RDONLY))){
		printf("ping: can not open %s\n", tmp);
		return;
	}
	while(0 < (r = read(com->tty_fd, &ch, sizeof(ch))))
		if(ch == 0x03)
			break;
	close(com->tty_fd);
	com->tty_fd = -1;
	SetEventFlag(com->main_evfid, ABORT_DONE);
	while(1)
		SleepThread();
}

static void show_stat(struct common *com){
	int ns, nr, nd, loss;

	ns = com->n_send;
	nr = com->n_recv;
	nd = com->n_dup;
	loss = (ns)? ((ns - nr) * 100 / ns): 0;
	printf("\n--- %s ping staticstics ---\n", com->hostname);
	printf("%d packets transmitted", ns);
	printf(", %d packets received", nr);
	if(0 < nd)
		printf(", +%d duplicates", nd);
	printf(", %d%% packet loss\n", loss);
	printf("round-trip min/avg/max = ");
	if(0 <= com->dms_min)
		printf("%d.%d", com->dms_min / 10, com->dms_min % 10);
	else
		printf("?.?");
	if(0 < nr)
		printf("/%d.%d", (com->dms_total / nr) / 10,
			(com->dms_total / nr) % 10);
	else
		printf("?.?");
	printf("/%d.%d", com->dms_max / 10, com->dms_max % 10);
	printf("\n");
}

static int ping_start(struct common *com, int ac, char *av[]){
	int r, i;

	com->count = 0;
	com->size = 56;
	for(--ac, ++av; 0 < ac && *av[0] == '-'; --ac, ++av)
		if(!strcmp("-help", av[0]))
			return(usage());
		else if(!strcmp("-c", av[0])){
			if(++av, --ac < 1)
				return(usage());
			if(scan_number(av[0], &com->count))
				return(-1);
			if(com->count <= 0){
				printf("ping: bad number of packets"
					" to transmit..\n");
				return(-1);
			}
		}else if(!strcmp("-s", av[0])){
			if(++av, --ac < 1)
				return(usage());
			if(scan_number(av[0], &com->size))
				return(-1);
			if(com->size <= 0){
				printf("ping: illegal packet size.\n");
				return(-1);
			}
		}else if(!strcmp("-tty", av[0])){
			if(++av, --ac < 1)
				return(usage());
			if(scan_number(av[0], &com->tty_num))
				return(-1);
			if(com->tty_num < 0 || 9 < com->tty_num){
				printf("ping: illegal tty number.\n");
				return(-1);
			}
		}else
			return(usage());
	if(ac != 1)
		return(usage());
	if(0 > (r = sceInetName2Address(0, &com->hostaddr, av[0],
			RES_TIMEOUT, RES_RETRY))){
		printf("sceInetName2Address(%s) -> %d\n", av[0], r);
		return(r);
	}
	strcpy(com->hostname, av[0]);

	if(0 > (com->time_evfid = create_event_flag()))
		return(-1);
	USec2SysClock(1000 * 1000, &com->sys_clock);
	if(KE_OK != (r = SetAlarm(&com->sys_clock, alarm_handler, com))){
		printf("ping: SetAlarm (%d)\n", r);
		return(-1);
	}
	com->alarmed = 1;

	if(0 >= (com->cid = open_raw()))
		return(-1);

	com->bufsiz = sizeof(IP_HDR) + sizeof(ICMP_HDR) + com->size;
	for(i = 0; i < RECORD_SIZE; i++)
		records[i].seq = -1;
	com->dms_min = -1;

	if(0 > (com->main_evfid = create_event_flag()))
		return(-1);

	if(NULL == (com->rbuf = alloc_memory(64 * 1024)))	/* 64 KB */
		return(-1);
	if(0 >= (com->rtid = start_thread(ping_recv, com)))
		return(-1);

	if(NULL == (com->sbuf = alloc_memory(com->bufsiz)))
		return(-1);
	if(0 >= (com->stid = start_thread(ping_send, com)))
		return(-1);

	com->ping_id = (u_short)((com->stid >> 16) + com->stid);

	if(0 >= (com->atid = start_thread(ping_abort, com)))
		return(-1);

	return(0);
}

#if defined(ENABLE_MODULE_UNLOAD)
static int unloadable(void){
	int dummy, mcount = -1;

	GetModuleIdList(&dummy, 1, &mcount);
	return((mcount < 0)? 0: 1);
}
#endif

static void ping_wait(u_long arg){
	struct common *com = (void *)arg;
	u_long result, state = 0;
	int r;

	while(1){
		if(KE_OK != (r = WaitEventFlag(com->main_evfid,
				RECV_DONE | SEND_DONE | ABORT_DONE,
				EW_OR | EW_CLEAR, &result))){
			printf("ping: WaitEventFlag (%d)\n", r);
			break;
		}

		state |= result;
		if(state & ABORT_DONE)
			break;
		if((state & RECV_DONE) == 0)
			continue;
		if((state & SEND_DONE) == 0)
			continue;
	}

#if defined(ENABLE_MODULE_UNLOAD)
	if(unloadable()){
		show_stat(com);
		SelfStopModule(0, NULL, NULL);
		SelfUnloadModule();
/* never returns here */
	}
#endif
	release_resources(com);
	show_stat(com);
			ExitThread();
}

int start(int ac, char *av[]){
	struct common *com = &common;
	int r;

#ifdef HDD_ETHERNET
	PreparePowerOff();
#endif

#if defined(ENABLE_MODULE_UNLOAD)
	if(ac < 0){
		if(!strcmp("other", av[0])){
			(void)TerminateThread(com->pwid);
			if(KE_OK != (r = DeleteThread(com->pwid)))
				printf("ping: DeleteThread (%d)\n", r);
		}else if(!strcmp("self", av[0]))
			;
		release_resources(com);
		return(NO_RESIDENT_END);
	}
#endif
	bzero(com, sizeof(*com));
	com->tty_num = DEFAULT_TTY_NUMBER;
	com->tty_fd = -1;
	if(0 != (r = ping_start(com, ac, av)))
		return((r << 4) | NO_RESIDENT_END);
#if defined(ENABLE_MODULE_UNLOAD)
	if(0 >= (com->pwid = start_thread(ping_wait, com))){
#else
	if(0 >= start_thread(ping_wait, com)){
#endif
		release_resources(com);
		return(NO_RESIDENT_END);
	}
#if defined(ENABLE_MODULE_UNLOAD)
	if(unloadable())
		return(REMOVABLE_RESIDENT_END);
#endif
	return(RESIDENT_END);
}
