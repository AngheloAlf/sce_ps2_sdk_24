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
 *                         modlist.c
 *                         develop library
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 *       2.3.4          Jul,17,2001     isii
 */

#include <stdio.h>
#include <stdlib.h>
#include <kernel.h>

/*                    ----+----1----+----2----+----3----+----4----+----5----+-*/
ModuleInfo Module = {"Module_list_program___________________________________/X???", 0x0101 };

char *status_name[] = {
    "?", "loaded", "starting", "resident", "stopping",
    "self-stopping", "stopped", "self-stopped"
};

int modulelist[100];

void printstatus(ModuleStatus *status);

int start(int argc, char *argv[])
{
    int i, mcount, totalmodcount, n;
    ModuleStatus status;

    if( argc > 1 && atoi(argv[1]) > 0 ) {
	/*  usage: modlist.irx <module_id>... */
	printf("Modules status\n");
	for( i = 1; i < argc; i++ ) {
	    n = ReferModuleStatus(atoi(argv[i]), &status);
	    if( n  == KE_OK ) {
		printstatus(&status);
	    } else if( n  == KE_UNKNOWN_MODULE ) {
		printf(" modid = %d unknown \n", atoi(argv[i]));
	    } else {
		printf(" What happen ?\n");
	    }
	}
    } else {
	printf("Modules list\n");
	if( argc > 1 ) {
	    /*  usage: modlist.irx <module_name> */
	    mcount = GetModuleIdListByName(argv[1],modulelist,
					   sizeof(modulelist)/sizeof(int),
					   &totalmodcount);
	} else {
	    /*  usage: modlist.irx  */
	    mcount = GetModuleIdList(modulelist,
				     sizeof(modulelist)/sizeof(int),
				     &totalmodcount);
	}
	if( totalmodcount > mcount ) {
	    printf("Too many modules %d for readbuffer\n", totalmodcount);
	}
	for( i = 0; i < mcount; i++ ) {
	    n = ReferModuleStatus(modulelist[i], &status);
	    if( n  == KE_OK ) {
		printstatus(&status);
	    } else if( n  == KE_UNKNOWN_MODULE ) {
		printf(" modid = %d unknown \n",modulelist[i]);
	    } else {
		printf(" What happen ?\n");
	    }	    
	}
    }
    return NO_RESIDENT_END;
}

void printflag(int flags)
{
    printf("%s", status_name[MSTF_MASK(flags)]);
    if( flags & MSTF_REMOVABLE ) printf(" removable");
    if( flags & MSTF_NOSYSALLOC ) printf(" no-auto-alloc");
}

void printstatus(ModuleStatus *status)
{
    printf(" %3d: v%d.%d %s\n", status->id,
	   status->version>>8, status->version&0xff,
	   status->name);
    printf("      0x%06lx..0x%06lx, entry=0x%lx, gp=0x%lx, size=0x%lx,0x%lx,0x%lx\n",
	   status->text_addr,
	   status->text_addr
	   + status->text_size + status->data_size + status->bss_size -1,
	   status->entry_addr, status->gp_value,
	   status->text_size, status->data_size, status->bss_size);
    printf("      flag=0x%04x (", status->flags );
    printflag(status->flags);
    printf(")\n");
}
