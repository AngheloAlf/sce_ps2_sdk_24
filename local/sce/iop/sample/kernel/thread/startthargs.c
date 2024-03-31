/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                         - thread -
 * 
 * 
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            stratthargs.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.4.0           
 */

#include <stdio.h>
#include <kernel.h>

#define BASE_priority  32

void thmain(int args, long argp[]);
int thread(int arg);

int start(int argc, char *argv[])
{
    struct ThreadParam param;
    int	thid, i;
    long argp[10];
    
    printf("\nStartThreadArgs() test ! \n");
    param.attr         = TH_C;
    param.entry        = thmain;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    for( i = 0; i < 10 ; i++ )
	argp[i] = i+1;
    printf("start() \n args = %d argp = ", sizeof(long)*5);
    for( i = 0; i < 10; i++ )
	printf("%ld ",argp[i]);
    printf("\n");

    if( thid > 0 ) {
	/* スレッドを起動   5 個の long 引数を渡す。*/
	StartThreadArgs(thid, sizeof(long)*5, argp);
	/* 引数のオリジナルを消去 */
	for( i = 0; i < 10 ; i++ ) argp[i] = 0;
	printf("start() \n args = %d argp = ", sizeof(long)*5);
	for( i = 0; i < 10; i++ )
	    printf("%ld ",argp[i]);
	printf("\n");

	/* スレッドを起床 */
	WakeupThread(thid);

	return RESIDENT_END;
    } else {
	return NO_RESIDENT_END;
    }
}

void thmain(int args, long argp[])
{
    int i;
   
    printf("thmain start \n args = %d argp = ", args);
    for( i = 0; i < 5; i++ )	printf("%ld ",argp[i]);
    for( ;i < 10; i++ )		printf("[%ld] ",argp[i]);
    printf("\n");
    printf("thmain sleep\n");
    SleepThread();
    printf("thmain wakup \n args = %d argp = ", args);
    for( i = 0; i < 5; i++ )	printf("%ld ",argp[i]);
    for( ;i < 10; i++ )		printf("[%ld] ",argp[i]);
    printf("\n");
}

