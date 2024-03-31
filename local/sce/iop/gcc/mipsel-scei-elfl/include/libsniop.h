//////////////////////////////////////////////////////////////////
//
// IOPLIBSN.H
//
// Contains definitions for global constructor/destructor code,
//

extern "C" {

#include <sysmem.h>
#include <setjmp.h>

static jmp_buf exitbuf;
#define SN_CALL_CTORS asm ("jal __do_global_ctors")
#define SN_CALL_DTORS asm ("jal __do_global_dtors")

int free (void* freemem)
{
	return FreeSysMemory(freemem);
}

void *malloc(unsigned long Size)
{
     return(AllocSysMemory(0, Size, NULL));
}

void _exit()
{
    longjmp(exitbuf, 1);
}

void exit()
{
    longjmp(exitbuf, 1);
}
}