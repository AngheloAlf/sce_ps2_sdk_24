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
 *                       <libinsck - sys/socket.h>
 *                      <libinsck general header>
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           May,17,2001     komaki      first version
 */

#ifndef __SYS_SOCKET_H__
#define	__SYS_SOCKET_H__

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

typedef u_char		sceInsockSaFamily_t;
typedef u_int		sceInsockSocklen_t;

#if !defined(sceInsockDisableSocketSymbolAliases)
#define sa_family_t	sceInsockSaFamily_t
#define socklen_t	sceInsockSocklen_t
#endif	/* !sceInsockDisableSocketSymbolAliases */
 
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */

#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */

typedef struct sceInsockSockaddr {
	u_char		sa_len;		/* total length */
	sa_family_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
} sceInsockSockaddr_t;

#if !defined(sceInsockDisableSocketSymbolAliases)
#define sockaddr	sceInsockSockaddr
#endif	/* !sceInsockDisableSocketSymbolAliases */

#define SHUT_RD		0		/* shutdown the reading side */
#define SHUT_WR		1		/* shutdown the writing side */
#define SHUT_RDWR	2		/* shutdown both sides */

int	sceInsockAccept(int, sceInsockSockaddr_t *, sceInsockSocklen_t *);
int	sceInsockBind(int, const sceInsockSockaddr_t *, sceInsockSocklen_t);
int	sceInsockConnect(int, const sceInsockSockaddr_t *, sceInsockSocklen_t);
int	sceInsockGetpeername(int, sceInsockSockaddr_t *, sceInsockSocklen_t *);
int	sceInsockGetsockname(int, sceInsockSockaddr_t *, sceInsockSocklen_t *);
int	sceInsockGetsockopt(int, int, int, void *, sceInsockSocklen_t *);
int	sceInsockListen(int, int);
ssize_t	sceInsockRecv(int, void *, size_t, int);
ssize_t	sceInsockRecvfrom(int, void *, size_t, int, sceInsockSockaddr_t *,
		sceInsockSocklen_t *);
ssize_t	sceInsockSend(int, const void *, size_t, int);
ssize_t	sceInsockSendto(int, const void *,
		size_t, int, const sceInsockSockaddr_t *, sceInsockSocklen_t);
int	sceInsockSetsockopt(int, int, int, const void *, sceInsockSocklen_t);
int	sceInsockShutdown(int, int);
int	sceInsockSocket(int, int, int);

#if !defined(sceInsockDisableSocketSymbolAliases)
#define accept		sceInsockAccept
#define bind		sceInsockBind
#define connect		sceInsockConnect
#define getpeername	sceInsockGetpeername
#define getsockname	sceInsockGetsockname
#define getsockopt	sceInsockGetsockopt
#define listen		sceInsockListen
#define recv		sceInsockRecv
#define recvfrom	sceInsockRecvfrom
#define send		sceInsockSend
#define sendto		sceInsockSendto
#define setsockopt	sceInsockSetsockopt
#define shutdown	sceInsockShutdown
#define socket		sceInsockSocket
#endif	/* !sceInsockDisableSocketSymbolAliases */

#if defined(__LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif /* __SYS_SOCKET_H__ */
