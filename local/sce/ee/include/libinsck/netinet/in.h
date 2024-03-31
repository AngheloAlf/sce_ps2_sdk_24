/* SCE CONFIDENTIAL
"PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *                      Emotion Engine Library
 *                          Version 1.00
 *                           Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                 <libinsck - libinsck/netinet/in.h>
 *                          <header for IP>
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           May,17,2001     komaki      first version
 */

#ifndef __NETINET_IN_H__
#define __NETINET_IN_H__

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

#define IPPROTO_IP	0
#define	IPPROTO_ICMP	1
#define IPPROTO_IGMP	2
#define	IPPROTO_TCP	6
#define	IPPROTO_UDP	17

typedef struct sceInsockInAddr {
	u_int s_addr;
} sceInsockInAddr_t;

#if !defined(sceInsockDisableSocketSymbolAliases)
#define in_addr		sceInsockInAddr
#endif	/* !sceInsockDisableSocketSymbolAliases */

#define	IN_CLASSA(i)		(((u_int)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((u_int)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((u_int)(i) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)		(((u_int)(i) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	INADDR_ANY		(u_int)0x00000000
#define	INADDR_LOOPBACK		(u_int)0x7f000001
#define	INADDR_BROADCAST	(u_int)0xffffffff
#define	INADDR_NONE		0xffffffff

#define	IN_LOOPBACKNET		127

typedef struct sceInsockSockaddrIn {
	u_char			sin_len;
	u_char			sin_family;
	u_short			sin_port;
	sceInsockInAddr_t	sin_addr;
	char			sin_zero[8];
} sceInsockSockaddrIn_t;

#if !defined(sceInsockDisableSocketSymbolAliases)
#define sockaddr_in	sceInsockSockaddrIn
#endif	/* !sceInsockDisableSocketSymbolAliases */

#define IP_MULTICAST_IF		9
#define IP_MULTICAST_TTL	10
#define IP_MULTICAST_LOOP	11
#define IP_ADD_MEMBERSHIP	12
#define IP_DROP_MEMBERSHIP	13

#define IP_DEFAULT_MULTICAST_TTL	1
#define IP_DEFAULT_MULTICAST_LOOP	1

typedef struct sceInsockIpMreq {
	sceInsockInAddr_t imr_multiaddr;
	sceInsockInAddr_t imr_interface;
} sceInsockIpMreq_t;

#if !defined(sceInsockDisableSocketSymbolAliases)
#define ip_mreq	sceInsockIpMreq
#endif	/* !sceInsockDisableSocketSymbolAliases */

u_int	sceInsockHtonl(u_int);
u_short	sceInsockHtons(u_short);
u_int	sceInsockNtohl(u_int);
u_short	sceInsockNtohs(u_short);

#if !defined(sceInsockDisableSocketSymbolAliases)
#undef	htonl
#undef	htons
#undef	ntohl
#undef	ntohs
#define htonl	sceInsockHtonl
#define htons	sceInsockHtons
#define ntohl	sceInsockNtohl
#define ntohs	sceInsockNtohs
#endif	/* !sceInsockDisableSocketSymbolAliases */

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif /* __NETINET_IN_H__ */
