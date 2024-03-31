/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
// Last Modified on 12/21/00 13:42:11 koji

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <kernel.h>
#include <dirent.h>
#include <libcdvd.h>

#define SECTOR_SIZE	512

#define MODPATH		"host1:/usr/local/sce/iop/modules/"

#define ONEMEG	(1024*1024)
#define TOTAL	(ONEMEG*32)
#define BUFSZ	(1024*32)
#define LOOPC	TOTAL/BUFSZ

#define trace()         printf("\n%s()\n", __FUNCTION__);

typedef unsigned long long u64;

static int buf0[BUFSZ/4];
static int buf1[BUFSZ/4];

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// show contents in one sector
static void ShowSector(u_char *p) {
    int i, j;
    char *ptr, digit[49], str[17];

    str[16] = '\0';
    for (i=0; i<SECTOR_SIZE/16; i++) {
	memset(digit, 0, 49);
	ptr = digit;
	for (j=0; j<16; j++) {
	    int n = j+i*16;
	    sprintf(ptr, "%02x ", p[n]);
	    ptr += 3;
	    str[j] = (isascii(p[n]) && isgraph(p[n])) ? p[n] : '.';
	}
	printf("%s%s\n", digit, str);
    }
}


// list all partitions
static void ShowPartition(char *devname) {
    int i, r, n, fd, sub;
    char size[16];
    char type[16];
    struct sce_dirent dirbuf;
    struct FsType {
	char   *str;
	u_short type;
    } fstype[] = {
	{"EMPTY",    0x0000},
	{"PFS",      0x0100},
	{"EXT2",     0x0083},
	{"EXT2SWAP", 0x0082},
    };

    if ((fd = dopen(devname)) < 0) {
	printf("cannot open device %s, %d\n", devname, fd);
	return;
    }

    printf("No      Size       Fs sub Id\n");

    n = 0;
    while ((r = dread(fd, &dirbuf)) > 0) {
	if (dirbuf.d_stat.st_size >= 0x00200000)
	    sprintf(size, "%4dGB", dirbuf.d_stat.st_size/0x00200000);
	else
	    sprintf(size, "%4dMB", dirbuf.d_stat.st_size/0x00000800);


	strcpy(type, "UNKNOWN");
	for (i=0; i<sizeof(fstype)/sizeof(struct FsType); i++)
	    if (dirbuf.d_stat.st_mode == fstype[i].type) 
		strcpy(type, fstype[i].str);

	sub = dirbuf.d_stat.st_attr & 0x01;
	printf("%03d:%c %s %8s  %2d %-16s\n", n++,
		(sub ? 'S' : 'M'),
		size, type, dirbuf.d_stat.st_private[0], dirbuf.d_name);
    }
    dclose(fd);
}


// show file modes
static void ShowMode(u_short mode) {
    int i;
    char *p, str[12];
    p = str;
    *p++ = (SCE_STM_ISDIR(mode)) ? 'd' : (SCE_STM_ISLNK(mode)) ? 'l': '-';
    for (i=8; i>=0; i--) {
	if ((1 << i) & mode) {
	    if (i == 9) *p++ = 's';
	    else if ((i%3) == 2) *p++ = 'r';
	    else if ((i%3) == 1) *p++ = 'w';
	    else if ((i%3) == 0) *p++ = 'x';
	} else *p++ = '-';
    }
    *p = '\0';
    printf("%s", str);
}


// show file status
static void ShowStat(struct sce_stat *ss) {
    char *strmonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			  "Jal", "Aug", "Sep", "Oct", "Nov", "Dec",};
    u64 size = ((u64)ss->st_hisize << 32) | ss->st_size;

    ShowMode(ss->st_mode);
    printf(" %-5d %-5d 0x%04x %5d %9d %s %2d %2d:%02d",
	    ss->st_private[0], ss->st_private[1], ss->st_attr,
	    ss->st_private[2], (u_int)size,
	    strmonth[ss->st_mtime[5]-1],
	    ss->st_mtime[4], ss->st_mtime[3], ss->st_mtime[2]);
}


// show files under specified directory
static int ShowDir(char *dir) {
    int r, fd;
    struct sce_dirent dirbuf;

    if ((fd = dopen(dir)) < 0) {
	printf("cannot open directory %s, %d\n", dir, fd);
	return fd;
    }
    while ((r = dread(fd, &dirbuf)) > 0) {
	ShowStat(&dirbuf.d_stat);
	printf(" %s\n", dirbuf.d_name);
    }
    dclose(fd);
    return r;
}


// show free capacity
static void ShowFree(void) {
    int zonesz, freezone;
    zonesz = devctl("pfs0:", PDIOC_ZONESZ, NULL, 0, NULL, 0);
    freezone = devctl("pfs0:", PDIOC_ZONEFREE, NULL, 0, NULL, 0);
    printf("zone size %d, %d zones free, %ld Kbytes free\n", zonesz, freezone,
           (long int)freezone*(long int)(zonesz/1024));
}


static void ShowStatics(int tb, int sec, int usec) {
    int tms, bps;
    tms = sec * 1000 + (usec + 500) / 1000;
    bps = (0 < tms) ? (int)(tb * 1000 / tms) : 0;
    printf("%d bytes %d.%03d sec\n", tb, tms/1000, tms%1000);
}


static void GetStat(char *path) {
    int r;
    struct sce_stat ss;

    if ((r = getstat(path, &ss)) < 0) {
	printf("cannot get stat for %s, %d\n", path, r);
	return;
    }
    ShowStat(&ss);
    printf("\n");
}


// create a diretory and then create a file under it
static int mkdir_test(const char *dev) {
    int r, fd;
    char path[256];

    trace();
    sprintf(path, "%s/%s", dev, __FUNCTION__);
    if ((r = mkdir(path, SCE_STM_RWXUGO)) < 0) {
	printf("cannot create directory %s, %d\n", path, r);
	return r;
    }

    sprintf(path, "%s/%s/bar", dev, __FUNCTION__);
    if ((fd = open(path, O_CREAT|O_RDWR, SCE_STM_RUGO|SCE_STM_WUGO)) < 0) {
	printf("cannot create file %s, %d\n", path, fd);
	return fd;
    }
    if ((r = write(fd, buf0, 0x111)) < 0) {
	printf("cannot write %d\n", r);
	close(fd);
	return fd;
    }
    close(fd);
    GetStat(path);
    return 0;
}


// create directory and change directory to it repeatedly
static int chdir_test(const char *dev) {
    int i, r, cnt = 5;
    char dirname[8];
    char path[256];

    trace();
    for (i=0; i<cnt; i++) {
	sprintf(path, "%s./chdir%d", dev, i);
	if ((r = mkdir(path, SCE_STM_RWXUGO)) < 0) {
	    printf("cannot create directory %s, %d\n", path, r);
	    return r;
	}
	if ((r = chdir(path)) < 0) {
	    printf("cannot change directory to %s, %d\n", path, r);
	    return r;
	}
    }

    strcpy(path, dev);
    for (i=0; i<cnt-1; i++) {
	sprintf(dirname, "/chdir%d", i);
	strcat(path, dirname);
    }
    ShowDir(path);
    return 0;
}


// create files
static int mkfile_test(const char *dev) {
    int i, fd, cnt = 0;
    char path[256];

    trace();
    for (i=0; i<16; i++) {
	sprintf(path, "%s/%s%04d", dev, __FUNCTION__, i);
	if ((fd = open(path, O_CREAT|O_RDWR, 0666)) < 0) {
	    printf("cannot create file %s, %d\n", path, fd);
	    return fd;
	}
	close(fd);
    }
    cnt = i;
    printf("made %d files\n", cnt);

#if 0
    while (cnt) {
	sprintf(path, "%s/%s%04d", dev, __FUNCTION__, --cnt);
	if ((r = remove(path)) < 0) {
	    printf("cannot remove file %s, %d\n", path, r);
	    return r;
	}
    }
#endif
    return 0;
}


// count time while executing func
static int CountTime(int fd, int *sec, int *usec, void (*func)(int fd)) {
    struct SysClock clk0, clk1;

    GetSystemTime(&clk0);
    func(fd);
    GetSystemTime(&clk1);
    *(unsigned long long *)&clk0 = *(unsigned long long *)&clk1
					    - *(unsigned long long *)&clk0;
    SysClock2USec(&clk0, sec, usec);
    return 0;
}


// write contents of buf0
static void wr_buf0_loopcount(int fd) {
    int i, r;
    for (i=0; i<LOOPC; i++) {
	if ((r = write(fd, buf0, BUFSZ)) < 0) {
	    printf("cannot write %d\n", r);
	    return;
	}
    }
}


// write contents of buf1
static void rd_buf1_loopcount(int fd) {
    int i, r;
    for (i=0; i<LOOPC; i++) {
	if ((r = read(fd, buf1, BUFSZ)) < 0) {
	    printf("cannot read %d\n", r);
	    return;
	}
    }
}


// compare data
static int compare(int *wdata, int *rdata) {
    int i;
    for (i=0; i<BUFSZ/4; i++) {
	if (wdata[i] != rdata[i]) {
	    printf("compare failed, data invalid at %d\n", i*4);
	    ShowSector((char *)&rdata[i]);
	    return -1;
	}
    }
    return 0;
}


// write a file has LOOPC*BUFSZ bytes
static int wrspeed_test(int allocflag) {
    int i, r, fd;
    int sec, usec;
    char *name = "pfs0:/wrspeed_test";

    trace();
    if ((fd = open(name, O_CREAT|O_RDWR, SCE_STM_RUGO|SCE_STM_WUGO)) < 0) {
	printf("cannot create file, %d\n", fd);
	return fd;
    }

    if (allocflag) {
	u_int size = (LOOPC*BUFSZ);
	if ((r = ioctl2(fd, PIOCALLOC, &size, sizeof(int), NULL, 0)) < 0) {
	    printf("ioctl2 failed, %d\n", r);
	    close(fd);
	    return r;
	}
    }
    for (i=0; i<BUFSZ/4; i++) buf0[i] = i;
    CountTime(fd, &sec, &usec, wr_buf0_loopcount);
    printf("written %d bytes, %d times.\n", BUFSZ, LOOPC);
    ShowStatics((u64)LOOPC*(u64)BUFSZ, sec, usec);
    close(fd);
    GetStat("pfs0:/wrspeed_test");
    return 0;
}


// read a file has LOOPC*BUFSZ bytes
int rdspeed_test(void) {
    int fd;
    int sec, usec;

    trace();
    if ((fd = open("pfs0:/wrspeed_test", O_RDWR)) < 0) {
	printf("cannot open file, %d\n", fd);
	return fd;
    }

    memset(buf1, 0, BUFSZ);
    CountTime(fd, &sec, &usec, rd_buf1_loopcount);
    printf("read    %d bytes, %d times.\n", BUFSZ, LOOPC);
    ShowStatics((u64)LOOPC*(u64)BUFSZ, sec, usec);
    close(fd);
    return 0;
}


// read, write and compare a file
int wrrd_test(void) {
    int i, r, fd;
    int sec, usec;
    char *name = "pfs0:/wrrd_test";

    trace();
    if ((fd = open(name, O_CREAT|O_RDWR, SCE_STM_RUGO|SCE_STM_WUGO)) < 0) {
	printf("cannot create file, %d\n", fd);
	return fd;
    }

    for (i=0; i<BUFSZ/4; i++) buf0[i] = i;
    CountTime(fd, &sec, &usec, wr_buf0_loopcount);
    printf("written %d bytes, %d times.\n", BUFSZ, LOOPC);
    ShowStatics((u64)LOOPC*(u64)BUFSZ, sec, usec);

    if ((r = lseek(fd, 0, SEEK_SET)) < 0) {
	printf("cannot seek %d\n", r);
	return r;
    }

    for (i=0; i<LOOPC; i++) {
	memset(buf1, 0, BUFSZ);
	if ((r = read(fd, buf1, BUFSZ)) < 0) {
	    printf("cannot read %d\n", r);
	    return r;
	}
	if (compare(buf0, buf1) < 0) break;
    }
    printf("read    %d bytes, %d times = %d\n",
		    BUFSZ, LOOPC, (u_int)((u64)LOOPC*(u64)BUFSZ));
    close(fd);
    return 0;
}


static void mainthread(void) {
    int i, r, fd, zonesz;

#if 1
    // initialize disk
    if ((r = format("hdd0:", NULL, NULL, 0)) < 0) {
	printf("cannot format disk\n");
	return;
    }

    r = devctl("hdd0:", HDIOC_MAXSECTOR, NULL, 0, NULL, 0);
    printf("max # of sectors for one partition = %d, %dMbytes\n", r,
	((r/1024)*SECTOR_SIZE)/1024);

    // create main partition
    if ((fd = open("hdd0:test,fpwd,rpwd,128M,PFS", O_CREAT|O_RDWR)) < 0) {
	printf("cannot create partition\n");
	return;
    }

    // add 8 sub partitions
    for (i=0; i<8; i++) {
	char psize[8] = "128M";
	if ((ioctl2(fd, HIOCADDSUB, psize, strlen(psize)+1, NULL, 0)) < 0) {
	    printf("cannot add partition\n");
	    return;
	}
    }
    close(fd);			// we must close before, format or mount

    // initialize partition
    zonesz = 8192;
    if ((r = format("pfs:", "hdd0:test,fpwd", &zonesz, 4)) < 0) {
	printf("cannot format partition\n");
	return;
    }
#endif

    if ((r = mount("pfs0:", "hdd0:test,fpwd", SCE_MT_RDWR, NULL, 0)) < 0) {
	printf("cannot mount partition\n");
	return;
    }

    ShowPartition("hdd0:");
    ShowFree();

    if ((r = mkdir_test("pfs0:")) < 0) return;
    if ((r = chdir_test("pfs0:")) < 0) return;
    if ((r = wrspeed_test(0)) < 0) return;
    if ((r = rdspeed_test()) < 0) return;
    if ((r = wrrd_test()) < 0) return;
    if ((r = mkfile_test("pfs0:")) < 0) return;

    printf("\n");
    if ((r = ShowDir("pfs0:/")) < 0) return;
    if ((r = umount("pfs0:")) < 0) {
	printf("cannot umount partition\n");
	return;
    }

    printf("done\n");
}


int start(int argc, char **argv) {
    int r, rr;
    struct ThreadParam th;
    char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "10";
    char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "24";

    if (!sceCdInit(SCECdINoD)) return 1;

    if ((r = LoadStartModule(MODPATH "dev9.irx", 0, NULL, &rr) < 0)) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = LoadStartModule(MODPATH "atad.irx", 0, NULL, &rr) < 0)) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = LoadStartModule(MODPATH "hdd.irx", sizeof(hddarg),
						    hddarg, &rr) < 0)) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = LoadStartModule(MODPATH "pfs.irx", sizeof(pfsarg),
						    pfsarg, &rr) < 0)) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    th.attr = TH_C;
    th.entry = mainthread;
    th.initPriority = 30;
    th.stackSize = 0x800 << 2;
    StartThread(CreateThread(&th), 0);
    while(1) DelayThread(1000*1000);
    return 0;
}

// Local variables:
// tab-width: 8
// End:
// vi:set tabstop=8:
