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
 *                 <netglue - netglue/netinet/in.h>
 *                          <header for IP>
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           Sep,21,2001     komaki      first version
 */

#ifndef __NETGLUE_NETINET_IN_H__
#define __NETGLUE_NETINET_IN_H__

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

#define	IPPROTO_ICMP	1
#define	IPPROTO_TCP	6
#define	IPPROTO_UDP	17

typedef struct sceNetGlueInAddr {
	u_int s_addr;
} sceNetGlueInAddr_t;

#if !defined(sceNetGlueDisableSocketSymbolAliases)
#define in_addr		sceNetGlueInAddr
#endif	/* !sceNetGlueDisableSocketSymbolAliases */

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

typedef struct sceNetGlueSockaddrIn {
	u_char			sin_len;
	u_char			sin_family;
	u_short			sin_port;
	sceNetGlueInAddr_t	sin_addr;
	char			sin_zero[8];
} sceNetGlueSockaddrIn_t;

#if !defined(sceNetGlueDisableSocketSymbolAliases)
#define sockaddr_in	sceNetGlueSockaddrIn
#endif	/* !sceNetGlueDisableSocketSymbolAliases */

u_int	sceNetGlueHtonl(u_int);
u_short	sceNetGlueHtons(u_short);
u_int	sceNetGlueNtohl(u_int);
u_short	sceNetGlueNtohs(u_short);

#if !defined(sceNetGlueDisableSocketSymbolAliases)
#undef	htonl
#undef	htons
#undef	ntohl
#undef	ntohs
#define htonl	sceNetGlueHtonl
#define htons	sceNetGlueHtons
#define ntohl	sceNetGlueNtohl
#define ntohs	sceNetGlueNtohs
#endif	/* !sceNetGlueDisableSocketSymbolAliases */

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif /* __NETGLUE_NETINET_IN_H__ */
