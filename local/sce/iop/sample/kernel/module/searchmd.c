/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *		I/O Processor Library Sample Program
 *
 *			-- Module --
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         searchmd.c
 *                         develop library
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <stdlib.h>
#include <kernel.h>

#define MYNAME "Search_module_sample"

ModuleInfo Module = { MYNAME, 0x0101 };

int start(int argc, char *argv[])
{
    int modid, myid;

    printf("Search module sample start\n");

    myid = SearchModuleByName(MYNAME);
    if( myid > 0 )
	printf(" My ModuleId = %d\n", myid);
    else
	printf(" What happen ? return=%d\n", myid);

    modid = SearchModuleByName("System_Memory_Manager");
    if( modid == KE_UNKNOWN_MODULE )
	printf(" Search modlue 'System_Memory_Manager' .. not found. What happen ?\n");
    else
	printf(" 'System_Memory_Manager' Id = %d\n", modid);

    modid = SearchModuleByName("no-found-module");
    if( modid == KE_UNKNOWN_MODULE )
	printf(" Search modlue 'no-found-module' .. not found. It's OK.\n");
    else
	printf(" What happen ? return=%d\n", modid);

    modid = SearchModuleByAddress(start);
    if( modid > 0 ) {
	if( modid != myid )
	    printf(" my module id not match  %d != %d\n", modid, myid);
    }  else
	printf(" What happen ? return=%d\n", modid);

    /* 常駐ライブラリの関数名を渡しても、モジュール間リンクの機構上の理由で
     * 自モジュールの ID が帰ってきます。*/
    modid = SearchModuleByAddress(printf);
    if( modid > 0 ) {
	if( modid != myid )
	    printf(" my module id not match  %d != %d\n", modid, myid);
    }  else
	printf(" What happen ? return=%d\n", modid);

    modid = SearchModuleByAddress(&modid);
    if( modid == KE_UNKNOWN_MODULE )
	printf(" Search addr 0x%x  .. not found. It's OK.\n", (int)&modid );
    else
	printf(" What happen ? return=%d\n", modid);

    printf("Search module sample end\n");
    return NO_RESIDENT_END;
}
