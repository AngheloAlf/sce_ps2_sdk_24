/* SCEI CONFIDENTIAL
 * "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *	Network GUI Setting Library Sample
 *		Set Configuration Initializer
 *
 *                          Version 1.2
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *	                 All Rights Reserved.
 *
 *                          setinit.h
 *
 *        Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2001.06.01      tetsu      Beta Release
 *        1.2           2001.07.19      tetsu      First Ver. Release
 */
#define SETINIT_INTERFACE	(0x11)
#define DOWN			(0)
#define SET_CNF			(1)
#define GET_ADDR		(2)
#define SSIZE			(4096)

/*  --------------------------------------------------------------------
 *  RPC バッファ割り当て
 *  --------------------------------------------------------------------
 */
typedef struct setinit_arg {
    int data;
    int p0;
    int p1;
    int p2;
} setinit_arg_t;
