/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                         - autodma -
 * 
 *                           Shift-JIS
 * 
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            main.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.40            Jul,21,1999     morita    provisional version
 *      0.50            Aug,15,1999     morita    alloc/open/read
 */

#include <stdio.h>
#include <sys/file.h>
#include <kernel.h>
#include <string.h>
#include <libspu2.h>

#define IS_LOOP		0

#define BASE_priority	32
#define BUFFADDR_441	0x180000
#define BUFFADDR_48	0x90000
#define BUFSIZ_48	(65536>>1)
#define BUFSIZ_441	(60208>>1)
#define PCM_FILENAME	"host1:/usr/local/sce/data/sound/wave/knot_l.int"
#define PRINTF(x) printf x

int myMain(int arg);


/* --- Interrupt Callback Function --- */
int IntFunc( void* common )
{
	printf("----IntFunc\n");
	return 1;
}


char* dataset(int *size)
{
	int fd;
	char *buffer;

	fd = open(PCM_FILENAME, O_RDONLY);
 	*size = lseek(fd, 0, 2);
	if( *size <= 0 ) { printf( "\nCan't load PCM file to iop heap \n" ); return NULL; }
	lseek( fd, 0, 0);

	PRINTF(("allocate IOP heap memory - " ));
	buffer = AllocSysMemory(0, *size, NULL);
	if( buffer == NULL ) { printf( "\nCan't alloc heap \n"); return NULL; }
	PRINTF(("alloced 0x%x  ", (int)buffer));

	read( fd, buffer, *size);
	close(fd);
	return buffer;
}


int start( int argc, char *argv[] )
{
 	struct ThreadParam param;
	int	thid;

	/* --- initialize thread --- */
	param.attr         = TH_C;
	param.entry        = myMain;
	param.initPriority = BASE_priority;
	param.stackSize    = 0x800;
	param.option       = 0;
	thid = CreateThread(&param);
	if( thid > 0 ) {
		StartThread(thid,0);
		return 0;	/* 0: program is resident */
	} else {
		return 1;	/* 1: program is deleted */
	}
}


int myMain( int arg )
{
	volatile int i;
	SpuCommonAttr c_attr;
	char *buffer;
	int size;

	if( (buffer = dataset(&size)) == NULL ){ return -1; }

	SpuInit();
	SpuSetCore(0);

	for( i = 0; i < 2; i++ )
	{
		// --- set commonn attribute
		SpuSetCore(i);
		c_attr.mask = (SPU_COMMON_MVOLL |  SPU_COMMON_MVOLR);
		c_attr.mvol.left  = 0x3fff;	/* Master volume (left) */
		c_attr.mvol.right = 0x3fff;	/* Master volume (right) */
		SpuSetCommonAttr (&c_attr);
	}

	SpuAutoDMASetCallback((void*)IntFunc);
#if IS_LOOP
	SpuAutoDMAWrite(buffer, (size/512)*512, SPU_AUTODMA_LOOP);
#else
	SpuAutoDMAWrite(buffer, size, SPU_AUTODMA_ONESHOT); // test
#endif

	while(SpuAutoDMAGetStatus() != 0){
		printf("Status = %8x \n", (unsigned int)SpuAutoDMAGetStatus() );
		i = 1000000; while( i > 0 ){ i--;  }
	}

	/* --- clean up --- */
	SpuAutoDMAStop();
	printf("Quit...\n");

	return 0;
}
