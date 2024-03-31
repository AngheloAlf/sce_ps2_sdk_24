/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
*/
/*
 *                   Library Sample Program
 *
 *                         - Sound -
 *
 *	### PS2 PCM raw-format converter ###
 *      PCM raw-format (.int)
 *      -> PCM raw-format for bypass processing (.inb)
 *	.int file is made by wav2int, in iop/sample/cdvd/stmspcm/wav2int.c
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *      All Rights Reserved.
 *
 *	how to make this command: cc -o int2inb int2inb.c
 *      how to make inb file: ./int2inb < foo.int > foo.inb
 *      [ !!! Notice !!!
 *          This program expects that the size of input data
 *          MUST be the multiple of 1024. ]
 */

#include <stdio.h>
#include <fcntl.h>

int
main (void)
{
    unsigned short l [256], r [256];
    unsigned short *lp, *rp;
    int i, ll, rr, c;
    unsigned short x;

    short z = 0;

    /*  input: standard input  (0)
     * output: standard output (1) */
    while (1) {
	bzero (l, 512);
	bzero (r, 512);
	/* The size of input data MUST be the multiple of 1024 */
	ll = read (0, l, 512); /* read 256 samples */
	rr = read (0, r, 512); /* read 256 samples */
	if (ll == 0 || rr == 0) break;
	lp = l; rp = r;
	/* l[0], r[0], l[1], r[1], ..., l[127], r[127] */
	for (i = 0; i < 128; i ++) {
	    write (1, lp, 2); lp ++;
	    write (1, rp, 2); rp ++;
	}
	/* 0 ... 256 samples, 512 bytes */
	for (i = 0; i < 256; i ++) {
	    write (1, &z, 2);
	}
	/* l[128], r[128], l[129], r[129], ..., l[255], r[255] */
	for (i = 0; i < 128; i ++) {
	    write (1, lp, 2); lp ++;
	    write (1, rp, 2); rp ++;
	}
	/* 0 ... 256 samples, 512 bytes */
	for (i = 0; i < 256; i ++) {
	    write (1, &z, 2);
	}
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
