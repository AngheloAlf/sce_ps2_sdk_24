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
 *                         removemd.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kernel.h>

#define MYNAME "removemd"

struct cmd {
    int modid;
    int delaysec;
} command;

#define BASE_priority  (USER_LOWEST_PRIORITY+USER_HIGHEST_PRIORITY)/2

int module_start(int argc, char *argv[]);
int module_stop(int argc, char *argv[]);
void remove_module(void *arg);
int MyCreateThread(int attr, void *entry, int iprio, int stack, void *opt);
char *geterrname(int err);

int start(int argc, char *argv[])
{
    if( argc >= 0 ) 	return module_start(argc,argv);
    else		return module_stop(-argc,argv);
}

int module_start(int argc, char *argv[])
{
    int thid;

    if( argc < 2 ) {
	printf("%s: usage:\n    %s <modulename> [<delay_sec>]\n", MYNAME, MYNAME);
	return NO_RESIDENT_END;
    }
    command.modid = SearchModuleByName(argv[1]);
    command.delaysec = -1;
    if( command.modid < KE_OK ) {
	printf("'%s' not found module '%s'\n", MYNAME, argv[1]);
	return NO_RESIDENT_END;
    }
    printf("'%s': '%s' module id = %d\n", MYNAME, argv[1], command.modid);
    if( argc == 3 ) {
	command.delaysec = atoi(argv[2]);
	thid = MyCreateThread(TH_C, remove_module, BASE_priority, 0x800, 0);
	if( thid > 0 ) {
	    StartThread(thid,(int)&command);
	    return REMOVABLE_RESIDENT_END;
	} else {
	    printf("'%s' fail\n", MYNAME);
	    return NO_RESIDENT_END;
	}
    } else {
	remove_module(&command);
	return NO_RESIDENT_END;
    }
}

int module_stop(int argc, char *argv[])
{
    /* ‘¼‚Ìƒ‚ƒWƒ…[ƒ‹‚©‚ç‚Í’âŽ~‚³‚¹‚È‚¢—á */
    if( strcmp(argv[0], "self") == 0 )
	return NO_RESIDENT_END;
    else
	return REMOVABLE_RESIDENT_END;
}

void remove_module(void *arg)
{
    int  mid, ret, delaysec;
    struct cmd *cmdp = (struct cmd *)arg;

    mid = cmdp->modid;
    delaysec = cmdp->delaysec;
    if( delaysec > 0 ) {
	DelayThread(delaysec*1000000);
    }
    ret = StopModule( mid, 0, NULL, NULL);
    if( ret != mid )
	printf("'%s' StopModule() : error = %s\n", MYNAME, geterrname(ret));
    if( ret == mid
	|| ret == KE_NOT_STARTED || ret == KE_ALREADY_STOPPED ) {
	ret = UnloadModule(mid);
	if( ret < KE_OK ) {
	    printf("'%s' UnloadModule() : error = %s\n", MYNAME, geterrname(ret));
	} else {
	    printf("'%s' module Id %d: stop&unloaded \n", MYNAME, mid);
	}
    }
    if( delaysec >= 0 ) {
	SelfStopModule(0,NULL,NULL);
	SelfUnloadModule();
    }
}

int MyCreateThread(int attr, void *entry, int iprio, int stack, void *opt)
{
    struct ThreadParam param;
    param.attr = attr;
    param.entry = entry;
    param.initPriority = iprio;
    param.stackSize = stack;
    param.option = (u_int)opt;
    return CreateThread(&param);
}

char *geterrname(int err)
{
    switch( err ) {
    case KE_UNKNOWN_MODULE: return  "KE_UNKNOWN_MODULE"; break;
    case KE_NOT_REMOVABLE: return  "KE_NOT_REMOVABLE"; break;
    case KE_NOT_STARTED: return  "KE_NOT_STARTED"; break;
    case KE_ALREADY_STOPPED: return  "KE_ALREADY_STOPPED"; break;
    case KE_ALREADY_STOPPING: return  "KE_ALREADY_STOPPING"; break;
    case KE_CAN_NOT_STOP: return  "KE_CAN_NOT_STOP";break;
    }
    return  "??";
}
