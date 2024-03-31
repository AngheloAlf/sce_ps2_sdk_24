/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
// Copyright (C) 2001 Sony Computer Entertainment Inc., All Rights Reserved.
// --------------------------------------------------------------------
// 0.00           Mar,22,2001     Koji Tashiro 1st version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libcdvd.h>

#define MPATH	"/usr/local/sce/iop/modules/"

#if defined(__R5900__) || defined(__ee__)
#define __EE__
#endif

#if defined(__EE__)

#include <unistd.h>
#include <eekernel.h>
#include <sifrpc.h>
#include <sifdev.h>
#include <libmc.h>

#define MODPATH		"host0:" MPATH

typedef unsigned long u64;

#else	// __EE__

#include <kernel.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <dirent.h>

#define MODPATH		"host1:" MPATH

typedef unsigned long long u64;

#define sceOpen		open
#define sceClose	close
#define sceRead		read
#define sceWrite	write
#define sceDopen	dopen
#define sceDread	dread
#define sceDclose	dclose
#define sceGetstat	getstat
#define sceFormat	format
#define sceMkdir	mkdir
#define sceRmdir	rmdir
#define sceRemove	remove
#define sceRename	rename
#define sceSync		sync
#define sceMount	mount
#define sceUmount	umount
#define sceDevctl	devctl
#define sceIoctl2	ioctl2
#define sceSifLoadStartModule	LoadStartModule
#define stdout
#define fflush(n)

#define SCE_RDONLY	O_RDONLY
#define SCE_RDWR	O_RDWR
#define SCE_CREAT	O_CREAT

#endif	// __EE__


// defines for string manipulation
#define LINE_LENGTH	80
#define CTRL(x)         (x&0x1f)
#define	DEL		0x7f
#define	INTR		CTRL('C')
#define	BELL		0x7

#define SECTOR_SIZE	512
#define DEPTH_MAX	(64-1)			// max directory depth
#define NAME_MAX	(255)			// max file name
#define PATH_MAX	(1024)			// max path name
#define DEVNAME_MAX	(64)			// max device name
#define MOUNT_MAX	(16)			// max # of mounts
#define CMD_MAX		(32)			// max length of commands
#define error(m...)	printf("%s: ", bname), printf(m)
#define BUFSZ		(1024*16)

typedef const char c_char;

typedef enum {
    DEV_NONE,
    DEV_PFS,
    DEV_MC,
    DEV_CD,
    DEV_HOST,
} DevType;					// device types

typedef enum {
    HELP_NONE,
    HELP_SHORT,
    HELP_LONG,
} HelpLevel;

typedef struct _command_type {
    char *name;
    void (*exec)(const struct _command_type *cmd, HelpLevel hl, c_char *p);
} CmdType;

static struct {
    char fs[DEVNAME_MAX];			// fs name
    char blk[DEVNAME_MAX];			// block device name
} dev[MOUNT_MAX];

static const struct FsType {			// file system type
    char   *str;
    u_short type;
} fstype[] = {
    {"EMPTY",    0x0000},
    {"EXT2SWAP", 0x0082},
    {"EXT2",     0x0083},
    {"PFS",      0x0100},
};

static char bname[64];				// basename
static char path[PATH_MAX];			// current path
static int devno = 0;				// current device numbder
static char buf0[BUFSZ] __attribute__((aligned(64)));	// temporary buffer
static char buf1[BUFSZ] __attribute__((aligned(64)));	// temporary buffer

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#if defined(__EE__)
static int myputc(char ch) {			// emulate putc
    static int column;
    switch (ch) {
	case '\n':
	    write(1, "\r\n", 2);
	    column = 0;
	    break;
	case '\t':
	    write(1, "        ", 8 - (column & 7));
	    column = (column & ~7) + 8;
	    break;
	default:
	    if (isprint(ch)) column++;
	    write(1, &ch, 1);
	    break;
    }
    return ch;
}


static char *mygets(char *buf) {		// emulate gets
    char c;
    char *bufp = buf;

    while (1) {
	read(0, &c, 1);
	switch (c & 0xff) {
	    case '\n':
	    case '\r':
		myputc('\n');
		*bufp = 0;
		return buf;
	    case CTRL('H'):
	    case DEL:
		if (bufp > buf) {
		    bufp--;
		    myputc(CTRL('H')); myputc(' '); myputc(CTRL('H'));
		}
		break;
	    case '\t': c = ' ';
	    default:
		if (isprint(c) && bufp < &buf[LINE_LENGTH - 3]) {
		    *bufp++ = c;
		    myputc(c);
		} else
		    myputc(BELL);
		break;
	}
    }
}
#else	// __EE__
#define mygets	gets
#endif


// read one line
static char *ReadLine(void) {
    char *p;
    static char buf[LINE_LENGTH];

    if (!mygets(buf)) return NULL;
    p = buf;
    while (*p && !isgraph(*p)) p++;
    return p;
}


// read charactor as integer, check range, and return it
static u_int ReadInt(u_int low, u_int high, const char *mesg) {
    u_int i;
    char *p;
    char ms[256];

    sprintf(ms, "%s (%d-%d): ", mesg, low, high);

    while (1) {
	do {
	    printf("%s", ms);
	    fflush(stdout);
	} while (*(p = ReadLine()) != '\n' && !isdigit(*p));

	i = atoi(p);
	while (isdigit(*p)) p++;
	if (i >= low && i <= high) break;
	else printf("Value out of range.\n");
    }
    return i;
}


// ask yes or no, and then return 1 as yes or 0 as no
static int Ask(const char *string, int def) {
    char c;

    printf("%s (%s)? ", string, (def) ? "Y/n" : "N/y");
    fflush(stdout);
    do {
	read(0, &c, 1);
    } while (c != 'y' && c != 'n' && !isspace(c));
    printf("\n");
    if (isspace(c)) return def;
    if (c == 'y') return 1;
    return 0;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// lists all partitions
static void ShowPartition(const char *devname) {
    int i, r, n, fd;
    char size[16];
    char type[16];
    struct sce_dirent dirbuf;

    if ((fd = sceDopen(devname)) < 0) {
	printf("cannot open device %s, %d\n", devname, fd);
	return;
    }

    printf("No      Size       Fs sub Id\n");

    n = 0;
    while ((r = sceDread(fd, &dirbuf)) > 0) {
	if (dirbuf.d_stat.st_size >= 0x00200000)
	    sprintf(size, "%4dGB", dirbuf.d_stat.st_size/0x00200000);
	else
	    sprintf(size, "%4dMB", dirbuf.d_stat.st_size/0x00000800);


	strcpy(type, "UNKNOWN");
	for (i=0; i<sizeof(fstype)/sizeof(struct FsType); i++)
	    if (dirbuf.d_stat.st_mode == fstype[i].type) 
		strcpy(type, fstype[i].str);

	printf("%03d:%c %s %8s  %2d %-16s\n", n++,
		(dirbuf.d_stat.st_attr & 0x01) ? 'S' : 'M',
		size, type, dirbuf.d_stat.st_private[0], dirbuf.d_name);
    }
    putchar('\n');
    sceDclose(fd);
}


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


// show file mode
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
static void ShowStat(struct sce_stat *ss, DevType type) {
    char *strmonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			  "Jal", "Aug", "Sep", "Oct", "Nov", "Dec",};
    u64 size = ((u64)ss->st_hisize << 32) | ss->st_size;

    if (type == DEV_MC) {
	ShowMode(((ss->st_mode & 0x7)
		    | ((ss->st_mode & SCE_STM_D) ? SCE_STM_FDIR : 0)));
	printf(" %-5d %-5d 0x%04x %5d %9d %s %2d %2d:%02d",
		ss->st_private[0], ss->st_private[1], ss->st_mode,
		ss->st_private[2], (u_int)size,
		strmonth[ss->st_mtime[5]-1],
		ss->st_mtime[4], ss->st_mtime[3], ss->st_mtime[2]);
    } else {
	ShowMode(ss->st_mode);
	printf(" %-5d %-5d 0x%04x %5d %9d %s %2d %2d:%02d",
		ss->st_private[0], ss->st_private[1], ss->st_attr,
		ss->st_private[2], (u_int)size,
		strmonth[ss->st_mtime[5]-1],
		ss->st_mtime[4], ss->st_mtime[3], ss->st_mtime[2]);
    }
}


// show files under specified directory
static int ShowDir(char *dir, DevType type) {
    int r, fd;
    struct sce_dirent dirbuf;

    if ((fd = sceDopen(dir)) < 0) {
	printf("cannot open directory %s, %d\n", dir, fd);
	return fd;
    }
    while ((r = sceDread(fd, &dirbuf)) > 0) {
	ShowStat(&dirbuf.d_stat, type);
	printf(" %s\n", dirbuf.d_name);
    }
    sceDclose(fd);
    return r;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// functions for fdisk command

#define NID		32
#define NPASSWD		8

static const struct Psize {			// valid partition sizes
    const char  *str;
    u_int nsector;
} psize[] = {
    {"128M", 0x00040000},
    {"256M", 0x00080000},
    {"512M", 0x00100000},
    {  "1G", 0x00200000},
    {  "2G", 0x00400000},
    {  "4G", 0x00800000},
    {  "8G", 0x01000000},
    { "16G", 0x02000000},
    { "32G", 0x04000000},
};


static void FdiskGetId(char *id) {
    printf("Id(max %d): ", NID); fflush(stdout);
    strncpy(id, ReadLine(), NID);
}


static void FdiskGetPwd(char *pwd, int f) {
    printf("%s password(max %d): ", ((f) ? "full" : "readonly"), NPASSWD);
    fflush(stdout);
    strncpy(pwd, ReadLine(), NPASSWD);
}


static void FdiskNew(const char *dev, u_int maxsector) {
    int i, fd, n = 0;
    char chsize[8], id[NID], fpwd[NPASSWD], rpwd[NPASSWD];

    for (i=0; i<sizeof(psize)/sizeof(struct Psize); i++) {
	printf("%2d: %s\n", i, psize[i].str);
	if (psize[i].nsector == maxsector) break;
    }

    n = ReadInt(0, i, "\nSize");
    strcpy(chsize, psize[n].str);

    FdiskGetId(id);
    FdiskGetPwd(fpwd, 1);
    FdiskGetPwd(rpwd, 0);
    sprintf(buf0, "%s%s,%s,%s,%s,PFS", dev, id, fpwd, rpwd, chsize);

    if ((fd = sceOpen(buf0, SCE_CREAT)) < 0) {
	error("cannot create partition\n");
	return;
    }
    sceClose(fd);
}


static void FdiskAddSub(const char *dev, u_int maxsector) {
    int i, fd, n = 0;
    char chsize[8], id[NID], fpwd[NPASSWD];

    for (i=0; i<sizeof(psize)/sizeof(struct Psize); i++) {
	printf("%2d: %s\n", i, psize[i].str);
	if (psize[i].nsector == maxsector) break;
    }

    n = ReadInt(0, i, "\nSize");
    strcpy(chsize, psize[n].str);

    FdiskGetId(id);
    FdiskGetPwd(fpwd, 1);
    sprintf(buf0, "%s%s,%s", dev, id, fpwd);

    if ((fd = sceOpen(buf0, SCE_RDWR)) < 0) {
	error("cannot open partition\n");
	return;
    }
    if ((sceIoctl2(fd, HIOCADDSUB, chsize, strlen(chsize)+1, NULL, 0)) < 0)
	error("cannot add sub partition.\n");
    sceClose(fd);
}


static void FdiskRemove(const char *dev) {
    int r;
    char id[NID], fpwd[NPASSWD];

    FdiskGetId(id);
    FdiskGetPwd(fpwd, 1);
    sprintf(buf0, "%s%s,%s", dev, id, fpwd);

    if ((r = sceRemove(buf0)) < 0) error("cannot remove partition, %d\n", r);
}


static void FdiskDelSub(const char *dev) {
    int fd, r;
    char id[NID], fpwd[NPASSWD];

    FdiskGetId(id);
    FdiskGetPwd(fpwd, 1);
    sprintf(buf0, "%s%s,%s", dev, id, fpwd);

    if ((fd = sceOpen(buf0, SCE_RDWR)) < 0) {
	printf("cannot open partition\n");
	return;
    }

    if ((r = sceIoctl2(fd, HIOCDELSUB, NULL, 0, NULL, 0)) < 0) {
	error("cannot delete sub partition, %d.\n", r);
    }
    sceClose(fd);
}


static int FdiskFormat(const char *dev) {
    int r;
    if ((r = sceFormat(dev, NULL, NULL, 0)) < 0) {
	printf("cannot format disk, %d.\n", r);
    }
    return r;
}


static void FdiskShowMenu(void) {
    printf("\nCommand action\n");
    printf("  m   print this menu\n");
    printf("  f   format disk\n");
    printf("  n   create partition\n");
    printf("  s   add a sub partition\n");
    printf("  d   remove partition\n");
    printf("  o   delete a sub partition\n");
    printf("  p   print the partition table\n");
    printf("  q   quit\n");
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


// find all partitions and try to mount them automatically
static void AutoMountHdd(char *devname, int dmd) {
    int r, fd;
    struct sce_dirent dirbuf;

    printf("\n");
    if ((fd = sceDopen(devname)) < 0) {
	printf("cannot open device %s, %d\n", devname, fd);
	return;
    }

    while ((r = sceDread(fd, &dirbuf)) > 0) {
	if (dirbuf.d_stat.st_mode == 0x100 
	    && !(dirbuf.d_stat.st_attr & 0x01) && devno < MOUNT_MAX) {
	    // try to mount, if password is set, mount will fail
	    devno++;
	    sprintf(dev[devno].fs, "pfs%d:", devno-dmd);
	    sprintf(dev[devno].blk, "%s%s", devname, dirbuf.d_name);

	    printf("try to mount %s to %s\n", dev[devno].blk, dev[devno].fs);
	    if ((r = sceMount(dev[devno].fs, dev[devno].blk,
						SCE_MT_RDWR, NULL, 0)) < 0) {
		printf("cannot mount partition, %d.\n", r);
		memset(dev[devno].fs, 0, DEVNAME_MAX);
		memset(dev[devno].blk, 0, DEVNAME_MAX);
		devno--;
		continue;
	    }
	}
    }
    sceDclose(fd);
}


static char *GetDevname(c_char *name) {			// extract device name
    char *p, *digit;
    static char devname[32];

    while (*name == ' ') name++;
    if (!(p = strchr(name, ':'))) return NULL;
    strncpy(devname, name, (u_int)(p - name));
    digit = &devname[p - name];
    devname[p - name] = 0;
    while (isdigit(digit[-1])) digit--;
    *digit = 0;
    return devname;
}


static void Slash2BackSlash(char *str) {	// exchange slash to backslash
    char *p = str;
    while (*p && !isspace(*p)) {
	if (*p == '/') *p = '\\';
	p++;
    }
}


static void BackSlash2Slash(char *str) {	// exchange backslash to slash
    char *p = str;
    while (*p && !isspace(*p)) {
	if (*p == '\\') *p = '/';
	p++;
    }
}


// append device name to p1
static void ExpandName(char *p0, char *p1) {
    if (p1[strlen(p1)-1] == '.') {
	if (!strcmp(GetDevname(p0), "cdrom")) {
	    if (strrchr(p0, '\\'))
		strcpy(&p1[strlen(p1)-1], strrchr(p0, '\\')+1);
	    else
		strcpy(&p1[strlen(p1)-1], strchr(p0, ':')+1);
	} else
	    if (strrchr(p0, '/'))
		strcpy(&p1[strlen(p1)-1], strrchr(p0, '/')+1);
	    else
		strcpy(&p1[strlen(p1)-1], strchr(p0, ':')+1);
    } else {
	int dir = 0;
	struct sce_stat ss;
	if (!sceGetstat(buf1, &ss)) {
	    if (!strcmp(GetDevname(buf1), "mc")) {
		if (ss.st_mode & SCE_STM_D) dir = 1;
	    } else {
		if (SCE_STM_ISDIR(ss.st_mode)) dir = 1;
	    }
	}
	if (dir) {
	    strcat(p1, "/");
	    if (!strcmp(GetDevname(p0), "cdrom"))
		strcpy(&p1[strlen(p1)], strrchr(p0, '\\')+1);
	    else
		strcpy(&p1[strlen(p1)], strrchr(p0, '/')+1);
	}
    }
}


// update current path
static void UpdateCwd(char *p, char *newwd) {
    int n;
    char *q, str[NAME_MAX];

    BackSlash2Slash(p);
//  printf("p = %s, newwd = %s\n", p, newwd);

    memset(newwd, 0, PATH_MAX);
    p++;
    while (*p) {
	if (!(q = strchr(p, '/'))) {
	    n = strlen(p);
	    memcpy(str, p, n);
	    p = p + n;
	} else {
	    n = q - p;
	    memcpy(str, p, n);
	    p = q + 1;
	}
	str[n] = '\0';
	if (!strcmp(str, ".")) continue;
	else if (!strcmp(str, "..")) {
	    if ((q = strrchr(newwd, '/'))) *q = '\0';
	    else newwd[0] = '\0';
	} else {
	    if (strlen(newwd)) strcat(newwd, "/");
	    strcat(newwd, str);
	}
    }
}


// get full path name
static c_char *GetFullName(c_char *p, char name[PATH_MAX]) {
    char *q;

    while (*p && isspace(*p)) p++;

    memset(name, 0, PATH_MAX);

    if (!p || !*p)      return p;
    else if (*p == '/') sprintf(name, "%s", dev[devno].fs);
    else {
	if (!strchr(p, ':')) {
	    if (*path) sprintf(name, "%s/%s/", dev[devno].fs, path);
	    else       sprintf(name, "%s/", dev[devno].fs);
	} 
    }
    q = name + strlen(name);
    while (*p && !isspace(*p)) *q++ = *p++;

    if (!strcmp(GetDevname(name), "cdrom")) Slash2BackSlash(name);
//  printf("fullname = %s\n", name);

    while (*p && isspace(*p)) p++;
    return p;
}


// show short help message
static void PrShort(const CmdType *cmd, c_char *syntax, c_char *desc) {
    printf("  %-8s %-*s  %s\n", cmd->name, 35, syntax, desc);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


// print file on the standard output
static void DoCat(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r, fd;

    if (hl) return PrShort(cmd, "<[dev:]file>", "display contents of file");

    (void)GetFullName(p, buf0);
    if ((fd = sceOpen(buf0, SCE_RDONLY)) < 0) {
	printf("cannot open file:%s, %d\n", buf0, fd);
	return;
    }
    memset(buf0, 0, BUFSZ);
    while ((r = sceRead(fd, buf0, BUFSZ)) > 0) {
	printf("%s", buf0);
	memset(buf0, 0, BUFSZ);
    }
    printf("\n");
    sceClose(fd);
}


// change directory
static void DoCd(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;
    char *q;
    struct sce_stat ss;

    if (hl) return PrShort(cmd, "[[dev:]directory]","change current directory");

    (void)GetFullName(p, buf0);
    if (!buf0[0]) sprintf(buf0, "%s/", dev[devno].fs);

#if 0	// I do not use chdir, because chdir is only supported by pfs
    if ((r = chdir(buf0)) < 0) {
	printf("chdir name to %s failed, %d\n", buf0, r);
	return;
    }
#else
    // I cannot check there is a specified directory on host
    if (!strcmp(GetDevname(buf0), "host")) {
	error("specified device does not support cd\n");
	return;
    }

    if (!strcmp(GetDevname(buf0), "cdrom")) Slash2BackSlash(buf0);

    if ((r = sceGetstat(buf0, &ss)) < 0) {
	printf("cannot get stat for %s, %d\n", buf0, r);
	return;
    }

    if (!strcmp(GetDevname(buf0), "mc")) {			// memory card
	if (!(ss.st_mode & SCE_STM_D)) error("not a directory\n");
    } else {
	if (!SCE_STM_ISDIR(ss.st_mode)) error("not a directory\n");
    }
#endif

    if ((q = strchr(buf0, ':'))) {
	int i, n = q - buf0;
	for (i=0; i<MOUNT_MAX; i++) {
	    if (dev[i].fs[0] && !strncmp(dev[i].fs, buf0, n)) {
		devno = i;
		break;
	    }
	}
	UpdateCwd(++q, path);
    } else UpdateCwd(buf0, path);
}


// copy files
static void DoCp(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r, rfd, wfd;
    int cmode = 0666;

    if (hl) return PrShort(cmd, "<[dev:]file> <[dev:]file>", "copy file");

    p = GetFullName(p, buf0);
//  if (!*p) return DoCp(cmd, HELP_LONG, NULL);
    if (!*p) return DoCp(cmd, HELP_SHORT, NULL);
    (void)GetFullName(p, buf1);
    ExpandName(buf0, buf1);

    if ((rfd = sceOpen(buf0, SCE_RDONLY)) < 0) {
	printf("cannot open file:%s, %d\n", buf0, rfd);
	return;
    }
    if ((wfd = sceOpen(buf1, SCE_RDWR|SCE_CREAT, cmode)) < 0) {
	printf("cannot open file:%s, %d\n", buf1, wfd);
	sceClose(rfd);
	return;
    }

    memset(buf0, 0, BUFSZ);
    while ((r = sceRead(rfd, buf0, BUFSZ)) > 0) {
	if ((r = sceWrite(wfd, buf0, r)) < 0) {
	    error("cannot write file, %d\n", r);
	    break;
	}
    }
    sceClose(rfd);
    sceClose(wfd);
}


#if defined(__EE__)
// execute program
static void DoExec(const CmdType *cmd, HelpLevel hl, c_char *p) {
    if (hl) return PrShort(cmd, "<[dev:]file>", "execute file");

    GetFullName(p, buf0);
    LoadExecPS2(buf0, 0, NULL);			// never return here
    printf("cannot execute file %s\n", buf0);
}
#endif


// manipulate partition table
static void DoFdisk(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int c;
    u_int mb;
    u_int total     = 0;
    u_int maxsector = 0;
    int   status    = 3;
    char devname[] = "hdd0:";

    if (hl) return PrShort(cmd, "[dev:]", "partition manipulator");

    if (*p && strcmp(GetDevname(p), "hdd")) {
	error("fdisk only support hdd device\n");
	return;
    }
    if (*p) strcpy(devname, p);

    if ((status = sceDevctl(devname, HDIOC_STATUS, NULL, 0, NULL, 0))) {
	if (status == 1) {
	    printf("This disk is not formatted, need format disk first.\n");
	    if (Ask("Format disk", 1)) {
		if (FdiskFormat(devname)) return;
	    }
	} else {
	    printf("drive status error, %d.\n", status);
	    return;
	}
    }

    if ((total = sceDevctl(devname, HDIOC_TOTALSECTOR, NULL, 0, NULL, 0)) < 0){
	printf("device control error, %d.\n", total);
	return;
    }
    if ((maxsector = sceDevctl(devname, HDIOC_MAXSECTOR, NULL,0,NULL,0)) < 0) {
	printf("device control error, %d.\n", maxsector);
	return;
    }

    while (1) {
	printf("\nCommand (m for help): ");
	fflush(stdout);
	if (!(c = tolower(*ReadLine()))) continue;
	switch (c) {
	    case 'f': (void)FdiskFormat(devname); break;
	    case 'n': FdiskNew(devname, maxsector); break;
	    case 's': FdiskAddSub(devname, maxsector); break;
	    case 'd': FdiskRemove(devname); break;
	    case 'o': FdiskDelSub(devname); break;
	    case 'p': 
		mb = total/2048;
		printf("Total number of sectors %d, %dMB = %dGB\n\n",
			total, mb, mb/1024);
		ShowPartition(devname);
		break;
	    case 'q': printf("\n"); return;
	    default: FdiskShowMenu(); break;
	}
    }
}


// format specified partition
static void DoFormat(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int i, r;
    char *q;
    u_int zonesz = 8192;

    if (hl) return PrShort(cmd, "<dev:> [zone size]", "format device");

    if (!strcmp(GetDevname(p), "mc")) {			// memory card
	if (Ask("Are you sure to continue", 0)) {
	    if ((r = sceFormat(p, NULL, NULL, 0)) < 0)
		error("cannot format %s, %d\n", p, r);
	}
	return;
    }

    if (strcmp(GetDevname(p), "hdd")) {
	error("cannot format device:%s\n", p);
	return;
    }

    for (i=0; i<MOUNT_MAX; i++) {
	if (dev[i].blk[0] && !strncmp(dev[i].blk, p, strlen(dev[i].blk))) {
	    printf("dev[i].blk %s, p %s, strlen(dev[i].blk) %d\n", dev[i].blk,
	    p, strlen(dev[i].blk));
	    error("cannot format currently mounted device.\n");
	    return;
	}
    }

    q = (char *)p;
    while (*q && !isspace(*q)) q++;
    *q = '\0';
    while (*q && isspace(*q)) q++;
    if (*q) zonesz = atoi(q);

    if ((r = sceFormat("pfs:", p, &zonesz, sizeof(zonesz))) < 0)
	error("cannot format partition:%s, %d\n", p, r);
}


static void DoHelp(const CmdType *cmd, HelpLevel hl, c_char *p);


// list files and directories
static void DoLs(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;
    struct sce_stat ss;

    if (hl) return PrShort(cmd, "[file]", "list files");

    (void)GetFullName(p, buf0);
    if (!buf0[0]) sprintf(buf0, "%s/%s", dev[devno].fs, path);

    // I cannot check there is a specified directory on host
    if (!strcmp(GetDevname(buf0), "host")) {
	error("specified device does not support ls\n");
	return;
    }

    if (!strcmp(GetDevname(buf0), "cdrom")) Slash2BackSlash(buf0);

    if ((r = sceGetstat(buf0, &ss)) < 0) {
	printf("cannot get stat for %s, %d\n", buf0, r);
	return;
    }

    if (!strcmp(GetDevname(buf0), "mc")) {			// memory card
	if (ss.st_mode & SCE_STM_D) ShowDir(buf0, DEV_MC);
	else {
	    ShowStat(&ss, DEV_MC);
	    printf(" %s\n", strrchr(buf0, '/')+1);
	}
    } else {
	if (SCE_STM_ISDIR(ss.st_mode)) ShowDir(buf0, DEV_PFS);
	else {
	    ShowStat(&ss, DEV_PFS);
	    if (!strcmp(GetDevname(buf0), "cdrom"))
		printf(" %s\n", strrchr(buf0, '\\')+1);
	    else
		printf(" %s\n", strrchr(buf0, '/')+1);
	}
    }
}


// make directory
static void DoMkdir(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;

    if (hl) return PrShort(cmd, "<directory>", "make directory");

    (void)GetFullName(p, buf0);
    if ((r = sceMkdir(buf0, 0777)) < 0)
	error("cannot make dir:%s, %d\n", buf0, r);
}


// mount a file system
static void DoMount(const CmdType *cmd, HelpLevel hl, c_char *p) {
    char *q;
    int i, r;

    if (hl) return PrShort(cmd, "<block device> <fs>", "mount filesystem");

    if (!p || !*p) {				// print currently mounted
	for (i=0; i<MOUNT_MAX; i++) {
	    if (dev[i].fs[0]) printf("%s\t  on  %s\n", dev[i].fs, dev[i].blk);
	}
	return;
    }

    for (i=0; i<MOUNT_MAX; i++) if (!dev[i].fs[0]) break;
    if (i == MOUNT_MAX) {
	error("no more space to mount.\n");
	return;
    }

    q = dev[i].blk;
    while (*p && !isspace(*p)) *q++ = *p++;

    while (*p && isspace(*p)) p++;
//  if (!*p) return DoMount(cmd, HELP_LONG, NULL);
    if (!*p) return DoMount(cmd, HELP_SHORT, NULL);
    
    q = dev[i].fs;
    while (*p && !isspace(*p)) *q++ = *p++;

    if ((r = sceMount(dev[i].fs, dev[i].blk, SCE_MT_RDWR, NULL, 0)) < 0) {
	printf("cannot mount %s to %s, %d.\n", dev[i].blk, dev[i].fs, r);
	memset(dev[i].fs, 0, DEVNAME_MAX);
	memset(dev[i].blk, 0, DEVNAME_MAX);
	return;
    }
    if ((q = strchr(dev[i].blk, ','))) *q = '\0';
}


// move files
static void DoMv(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;

    if (hl) return PrShort(cmd, "<[dev:]file> <[dev:]file>", "move file");

    p = GetFullName(p, buf0);
//  if (!*p) return DoMv(cmd, HELP_LONG, NULL);
    if (!*p) return DoMv(cmd, HELP_SHORT, NULL);
    (void)GetFullName(p, buf1);
    ExpandName(buf0, buf1);

    if (strcmp(GetDevname(buf0), "pfs") || strcmp(GetDevname(buf1), "pfs")) {
	error("mv currently only support both source and destination\n");
	printf("\ton same pfs device.\n");
	return;
    }
    if ((r = sceRename(buf0, buf1)) < 0)
	error("cannot mv file:%s to %s, %d\n", buf0, buf1, r);
}


// list all partitions
static void DoPlist(const CmdType *cmd, HelpLevel hl, c_char *p) {
    if (hl) return PrShort(cmd, "[dev:]", "list partitions");

    if (*p) ShowPartition(p);
    else    ShowPartition("hdd0:");
}


// remove a file
static void DoRm(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;

    if (hl) return PrShort(cmd, "<file>", "remove file");

    (void)GetFullName(p, buf0);
    if ((r = sceRemove(buf0)) < 0)
	error("cannot remove file:%s, %d\n", buf0, r);
}


// remove a directory
static void DoRmdir(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;

    if (hl) return PrShort(cmd, "<directory>", "remove directory");

    (void)GetFullName(p, buf0);
    if ((r = sceRmdir(buf0)) < 0)
	error("cannot remove directory:%s, %d\n", buf0, r);
}


// flush filesystem buffers
static void DoSync(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r;

    if (hl) return PrShort(cmd, "", "sync filesystem");

    if ((r = sceSync(dev[devno].fs, 0)) < 0)
	error("cannot sync fs:%s, %d\n", dev[devno].fs, r);
}


// unmount file system
static void DoUmount(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int i, r;

    if (hl) return PrShort(cmd, "<fs>", "unmount filesystem");

    for (i=0; i<MOUNT_MAX; i++) {
	if (dev[i].fs[0] && !strncmp(dev[i].fs, p, strlen(dev[i].fs))) {
	    if (i == devno) {
		error("cannot umount current device.\n");
		return;
	    }
	    if ((r = sceUmount(dev[i].fs)) < 0)
		error("cannot umount partition:%s, %d.\n", dev[i].fs, r);
	    else {
		memset(dev[i].fs, 0, DEVNAME_MAX);
		memset(dev[i].blk, 0, DEVNAME_MAX);
	    }
	    return;
	}
    }
    error("device %s is not mounted\n", p);
}


// show hex dump
static void DoXd(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int r, fd;

    if (hl) return PrShort(cmd, "<file>", "hex dump");

    (void)GetFullName(p, buf0);
    if ((fd = sceOpen(buf0, SCE_RDONLY)) < 0) {
	error("cannot open file:%s, %d\n", buf0, fd);
	return;
    }
    memset(buf0, 0, SECTOR_SIZE);
    while ((r = sceRead(fd, buf0, SECTOR_SIZE)) > 0) {
	ShowSector(buf0);
	if (!Ask("continue", 1)) break;
	memset(buf0, 0, SECTOR_SIZE);
    }
    printf("\n");
    sceClose(fd);
}


// change currently selected device
static void DoChdev(const CmdType *cmd, HelpLevel hl, c_char *p) {
    if (hl) return PrShort(cmd, "", "change current device");
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

static const CmdType commands[] = {
    {"cat",	DoCat,		},
    {"cd",	DoCd,		},
    {"cp",	DoCp,		},
#if defined(__EE__)
    {"exec",	DoExec,		},
#endif
    {"fdisk",	DoFdisk,	},
    {"format",	DoFormat,	},
    {"help",	DoHelp,		},
    {"ls",	DoLs,		},
    {"mkdir",	DoMkdir,	},
    {"mount",	DoMount,	},
    {"mv",	DoMv,		},
    {"plist",	DoPlist,	},
    {"rm",	DoRm,		},
    {"rmdir",	DoRmdir,	},
    {"umount",	DoUmount,	},
    {"sync",	DoSync,		},
    {"xd",	DoXd,		},
    {"?",	DoHelp,		},
    {"dev:",    DoChdev,	},
};

static int ncmd = sizeof(commands)/sizeof(commands[0]);


static void DoHelp(const CmdType *cmd, HelpLevel hl, c_char *p) {
    int i;

    if (hl) return PrShort(cmd, "[command]", "print command usage");

    if (!*p) {
	for (i=0; i<ncmd; i++) commands[i].exec(&commands[i], HELP_SHORT, NULL);
	return;
    }

    for (i=0; i<ncmd; i++)
	if (!strcmp(commands[i].name, p)) {
	    commands[i].exec(&commands[i], HELP_SHORT, NULL);
	    return;
	}

    error("command not found: %s\n", p);
}


static void Interprete(char *p) {			// interpret commands
    int i = 0;
    char cmd[CMD_MAX];

    if (!p) return;
    memset(cmd, 0, CMD_MAX);
    while (*p == ' ' || *p == '\t') p++;
    while (*p && !isspace(*p) && i<CMD_MAX) cmd[i++] = *p++;
    while (*p && isspace(*p)) p++;

    if (!*cmd) return;

    for (i=0; i<ncmd; i++)
	if (!strcmp(commands[i].name, cmd)) {
	    commands[i].exec(&commands[i], HELP_NONE, p);
	    return;
	}

    // if specified command does not match to I know, assume it as device name
    for (i=0; i<MOUNT_MAX; i++) {
	if (!strcmp(dev[i].fs, cmd)) {
	    devno = i;
	    memset(path, 0, PATH_MAX);
	    return;
	}
    }

    error("command not found: %s\n", cmd);
    for (i=0; i<ncmd; i++) commands[i].exec(&commands[i], HELP_SHORT, NULL);
}


static void mainthread(void) {
    int i, r;
    int dmd;		// number of default mounted devices without pfs

    memset(path, 0, PATH_MAX);

    for (i=0; i<MOUNT_MAX; i++) {
	memset(dev[i].fs, 0, DEVNAME_MAX);
	memset(dev[i].blk, 0, DEVNAME_MAX);
    }

    // set default devices
    devno = 0;
    strcpy(dev[devno].fs,  "mc0:");
    strcpy(dev[devno].blk, "mc0:");
    devno++;
    strcpy(dev[devno].fs,  "mc1:");
    strcpy(dev[devno].blk, "mc1:");
    devno++;
    strcpy(dev[devno].fs,  "cdrom0:");
    strcpy(dev[devno].blk, "cdrom0:");
    devno++;
    strcpy(dev[devno].fs,  "host0:");
    strcpy(dev[devno].blk, "host0:");
    devno++;
    strcpy(dev[devno].fs,  "host1:");
    strcpy(dev[devno].blk, "host1:");

    dmd = devno + 1;

    if (!sceDevctl("hdd0:", HDIOC_STATUS, NULL, 0, NULL, 0)) 
	AutoMountHdd("hdd0:", dmd);
    if (!sceDevctl("hdd1:", HDIOC_STATUS, NULL, 0, NULL, 0))
	AutoMountHdd("hdd1:", dmd);

    printf("\n\ncurrently mounted devices are:\n");
    DoMount(NULL, HELP_NONE, NULL);		// list mounted devices
    printf("\n");

    while (1) {
	printf("[%s]/%s $ ", dev[devno].fs, path);
	fflush(stdout);
	Interprete(ReadLine());
    }

    for (i=0; i<MOUNT_MAX; i++) {
	if (dev[i].fs[0] && !strncmp(dev[i].fs, "pfs", 3)) {
	    if ((r = sceUmount(dev[i].fs)) < 0)
		error("cannot umount partition, %d.\n", r);
	}
    }
}


#if defined(__EE__)
int main(int argc, char **argv) {
#else
int start(int argc, char **argv) {
#endif
    int r, rr;
    char *p, *name;
    char hddarg[] = "-o" "\0" "8" "\0" "-n" "\0" "32";
    char pfsarg[] = "-m" "\0" "8" "\0" "-o" "\0" "10" "\0" "-n" "\0" "64";

    name = ((p = strchr(argv[0], ':'))) ? (p + 1) : argv[0];
    name = ((p = strrchr(name, '/'))) ? (p + 1) : name;
    strcpy(bname, name);
#if defined(__EE__)
    sceSifInitRpc(0);
#endif

    if (!sceCdInit(SCECdINoD)) return 1;

    if ((r = sceSifLoadStartModule(MODPATH"dev9.irx", 0, NULL, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = sceSifLoadStartModule(MODPATH"atad.irx", 0, NULL, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = sceSifLoadStartModule(MODPATH"hdd.irx", sizeof(hddarg),
							hddarg, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = sceSifLoadStartModule(MODPATH"pfs.irx", sizeof(pfsarg),
							pfsarg, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = sceSifLoadStartModule(MODPATH"sio2man.irx", 0, NULL, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

    if ((r = sceSifLoadStartModule(MODPATH"mcman.irx", 0, NULL, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;

#if defined(__EE__)
    if ((r = sceSifLoadStartModule(MODPATH"mcserv.irx", 0, NULL, &rr)) < 0) {
	printf("cannot load module, %d.\n", r);
	return 1;
    }
    if (rr == NO_RESIDENT_END) return 1;
    sceMcInit();

    mainthread();
#else
    {
	struct ThreadParam th;
	th.attr = TH_C;
	th.entry = mainthread;
	th.initPriority = 30;
	th.stackSize = 4096;
	StartThread(CreateThread(&th), 0);
    }
#endif
    return 0;
}

// Local variables:
// tab-width: 8
// End:
// vi:set tabstop=8:
