C++ IOP Compiler.
-----------------

This compiler is built using the GCC 2.95.2 source code.  It slots in and works directly with the debugger/VSI (if used).  There are three important things you should be aware of: wrapping your SCE libs with extern "C", the method of calling and destroying global objects, and the bugfix to the stdio.h SCE header in V2.2 of the libs.

Wrap SCE headers with 'extern "C" { ... }'
------------------------------------------
You will need to wrap the Sony includes with extern "C", the most convenient way of doing this is in your source code eg:

...
#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

#include <kernel.h>
#include <sys\file.h>
#include <sif.h>
#include <stdlib.h>
#include <stdio.h>
#include <sifrpc.h>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif
...


C++ Global constructors and destructors
---------------------------------------
The C++ port has been written so that you have to manually call constructors and destructors.  The functions you use are SN_CALL_CTORS; and SN_CALL_DTORS; eg:

...
#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

#include <stdio.h>
#include <kernel.h>
#include <sysmem.h>
#include <sif.h>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#include <libsniop.h>  //Contains definitions for CTOR and DTOR calls.

#define BASE_priority  32

class foo
{
public:
	int hoo;
	int bar;
	foo () {hoo = 40; printf("\nfoo Constructor");}
	~foo () {hoo = 0; bar = 0; printf("\nfoo Destructor.");}
};

foo globfunc;  //Global object of class foo

ModuleInfo Module = { "cxx_fiddling", 0x0101 };

int thread1(void)
{
	printf("\nThread1 startup.");
	printf("\nCall Global Ctors.");

	SN_CALL_CTORS;  //Call all global CTORs

	globfunc.hoo = 32;
	globfunc.bar = 256;
	printf("\nCall global Dtors.");

	SN_CALL_DTORS;  //Call all global DTORs

	printf("\nStuff happening after global destructor call.");
	return 0;
}

int start (void)
{
    struct ThreadParam param;
    int thread;

    printf("\nStartup thread init.");

    CpuEnableIntr();

    if (!sceSifCheckInit())
    {
	sceSifInit();
    }

    param.attr         = TH_C;
    param.entry        = thread1;
    param.initPriority = BASE_priority - 2;
    param.stackSize    = 64*1024;
    param.option       = 0;
    thread = CreateThread (&param);
    if (thread > 0)
    {
	StartThread (thread, 0);
	printf ("\nThread 1 started.");
	printf ("\nStartup thread terminated.");
	return 0;
    }
    else
    {
	printf ("\nStartup thread terminated.  Errors encountered");
	return 1;
    }
}
...


The logic behind this is that most IOP programs terminate the start() thread before executing any program code.  You can call global constructors and destructors from non-startup threads, and access them as usual until they are destroyed.  If the globals were tied to start() then they would be destroyed once start terminates, as with main() in C++ programs on the EE.  A single call to either of these SN_CALL_... macros will initialise or destroy all global objects from every source file which is linked to your program.

You will need to #include <ioplibsn.h> for the definitions for C++ memory functions and also for the global constructor and destructor calling code.

Required change in V2.2 SCE Headers
-----------------------------------

GCC incompatibility in V2.2 of SCE headers:

The file sce\iop\gcc\mipsel-scei-elfl\include\stdio.h in incompatible with the GNU C++ compiler:
It contains the line

extern int   symlink(const char *existing, const char *new;

which must be changed to

extern int   symlink(const char *existing, const char *newname);


IOPFIXUP error: unresolved symbols
----------------------------------
If you get unresolved symbols for malloc and free, which are used for new and delete, or exit() and _exit(), linking with libsniop.a will resolve them, which maps malloc and free to AllocSysMemory and FreeSysMemory, and patches in termination code for exit and _exit.

If you get symbols similar to this (with suffixed name mangling visible):

iopfixup Warning: Unresolved Symbol: CpuEnableIntr__Fv
iopfixup Warning: Unresolved Symbol: CreateThread__FP11ThreadParam
iopfixup Warning: Unresolved Symbol: StartThread__FiUl
iopfixup Warning: Unresolved Symbol: AllocSysMemory__FiUlPv
iopfixup Warning: Unresolved Symbol: FreeSysMemory__FPv

It is because you have not wrapped the SCE header files in extern "C" { ... }.


Doubles and Float software emulation
------------------------------------
You now have access to float and double types and all computations involving them, although conversion from float to double is currently not possible.  Use doubles where possible.  Note that the IOP printf does not support floating point output.  To get this you link with libsniop.a, by adding '-lsniop' to your makefile or VSI link stage.  Note that this is software emulation, and is not really desirable for use in release code that needs to run quickly.  There is also a maths library, which will allow you to use the standard math functions such as trigonometry, logarithms and so on.  To use this link with -lm on the command line.

STL for IOP
-----------
This uses the latest acclaimed STLport, proving a compact and quick implementation of the standard template library.  Will work straight out of the installer, if you used zips however you will need to add the following line to your sn.ini:

iop_cplus_include_path=c:\usr\stlport-4.5\stlport;c:\usr\local\sce\iop\gcc\lib\gcc-lib\mipsel-scei-elfl\2.8.1\include;c:\usr\local\sce\iop\gcc\mipsel-scei-elfl\include;c:\usr\local\sce\iop\gcc\lib\gcc-lib\mipsel-scei-elfl\2.8.1;C:\usr\local\sce\iop\gcc\mipsel-scei-elf

Special thanks to Matt Curtis from Infogrammes Melbourne House for all his help on this one.  Cheers Matt!

Alternative C Compiler
----------------------
I have also built a new C compiler using the 2.95.2 source code as the compiler is more advanced than the 2.8.1 version, and may produce better code than the 2.8.1 version.  I have left the 2.8.1 version in this release, although you can change to the 2.95.2 C compiler by renaming two executables:

2.95.2 IOP C compiler (SN)
-----------------
\usr\local\sce\iop\gcc\lib\gcc-lib\mipsel-scei-elfl\2.8.1\cc12952.exe

2.8.1 IOP C compiler (SCE)
-----------------
\usr\local\sce\iop\gcc\lib\gcc-lib\mipsel-scei-elfl\2.8.1\cc1.exe

Simply store the 2.8.1 cc1.exe compiler safely away and rename the cc12952.exe compiler to cc1.exe.




If you have any problems or feedback please contact support@snsys.com.

Regards,

Dave Brown
SN Systems
Updated 1st October 2001.