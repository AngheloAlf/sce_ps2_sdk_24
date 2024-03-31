#! gmake
#
# Makefile for project programmer
#
#		start 2000.09.04
#

# directory
TOP		:= ../../..
BINDIR		:= $(TOP)/bin
LIBDIR		:= $(TOP)/lib
INCDIR		:= $(TOP)/include

# HIGDIR is directory-path where did install 'HiG'
HIGDIR		:= $(TOP)
#HIGDIR		:= $(TOP)/src/lib/hig

HIGLIBDIR	:= $(LIBDIR)
PLUGINDIR	:= $(LIBDIR)

#HIGLIBDIR	:= $(HIGDIR)/hig
#PLUGINIR	:= $(HIGDIR)/hip

HIGINCDIR	:= $(HIGDIR)/include

INSTLIBDIR	:= $(HIGLIBDIR)
INSTINCDIR	:= $(HIGINCDIR)

#*******************************#
# CHANGE BELLOW FOR Ur PROJECT	#
#*******************************#
TARGET		:= libhip
TAGSUFFIX	:= a
CSRCS		:= micro.c tex2d.c shape.c hrchy.c anime.c share.c tim2.c \
			clutbump.c shadowmap.c shadowbox.c lightmap.c fisheye.c reflect.c refract.c \
			access.c clip.c skin.c camera.c
CCSRCS		:=					# suffix .cc
ASRCS		:=					# suffix .s
DVPSRCS		:= vu1micro.dsm				# suffix .dsm

LIBS		:=

LCFILE		:= $(LIBDIR)/app.cmd

# alias
MAKEFILE	:= Makefile
SHELL		:= /bin/sh
RM		:= /bin/rm -f
ECHO		:= /bin/echo
CP		:= /bin/cp

# command for PS2
PREFIX		:= ee
CC		:= $(PREFIX)-gcc
CPP		:= $(CC) -E
LD		:= $(PREFIX)-gcc
AR		:= $(PREFIX)-ar
AS		:= $(PREFIX)-gcc
DVPASM		:= $(PREFIX)-dvp-as

# FLAGS
INCLUDES	:= -I. -I$(HIGINCDIR) -I$(INCDIR)
#INCLUDES	:= -I. -I$(INCDIR)
CPPFLAGS	:= $(INCLUDES)
CFLAGS		:= $(INCLUDES) -G0 -O2 -Wall -Wa,-al -fno-common
CXXFLAGS	:= $(INCLUDES) -G0 -O2 -Wall -Wa,-al -fno-exceptions
ASFLAGS		:= $(INCLUDES) -G0 -c -xassembler-with-cpp -Wa,-al
DVPASMFLAGS	:= $(INCLUDES)
ARFLAGS		:= rc
LDFLAGS		:= -Wl,-Map,$(TARGET).map -nostartfiles -L$(LIBDIR) -lm -Wl,-gc-sections

OBJS		:= $(CSRCS:.c=.o) $(CCSRCS:.cc=.o) $(ASRCS:.s=.o) $(DVPSRCS:.dsm=.o)

.SUFFIXES: .c .s .cc .dsm .a .elf

default all : $(TARGET).$(TAGSUFFIX)

run : $(TARGET).elf
	$(RUN) $(TARGET).elf

clean :
	$(RM) core *.o *.map *.lst *.dis *.d $(TARGET).elf $(TARGET).a *~

install:
	$(CP) $(TARGET).$(TAGSUFFIX) $(INSTLIBDIR)

link:
	$(RM) $(TARGET).a

$(TARGET).a : $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET).a $(OBJS)

$(TARGET).elf : $(OBJS)
	$(LD) -o $@ -T $(LCFILE) $(OBJS) $(LIBS) $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@ > $*.lst

.cc.o:
	$(CC) -c $(CXXFLAGS) $< -o $@ > $*.lst

.s.o:
	$(AS) $(ASFLAGS) -o $@ $< > $*.lst

.dsm.o:
	$(DVPASM) $(DVPASMFLAGS) -o $@ $< > $*.lst

$(CSRCS:.c=.d) :%.d : %.c
	set -e
	$(CPP) -M $(CPPFLAGS) $< | \
	sed 's/\($*\)\.o[ :]*/\1.o $@ :/g' > $@; \
	[ -s $@ ] || rm -f $@

$(CCSRCS:.cc=.d) : %.d : %.cc
	set -e
	$(CPP) -M $(CPPFLAGS) $< | \
	sed 's/\($*\)\.o[ :]*/\1.o $@ :/g' > $@; \
	[ -s $@ ] || rm -f $@

$(ASRCS:.s=.d) : %.d : %.s
	set -e
	$(CPP) -M $(CPPFLAGS) $< | \
	sed 's/\($*\)\.o[ :]*/\1.o $@ :/g' > $@; \
	[ -s $@ ] || rm -f $@

include $(CSRCS:.c=.d) $(CCSRCS:.cc=.d) $(ASRCS:.s=.d)
