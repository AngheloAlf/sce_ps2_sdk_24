#
#  USBMLOAD SAMPLE
#
#  $Id: Makefile,v 1.6 2001/07/05 11:01:16 fukunaga Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#----------- customize section --------------
CFLAGS   =  -Wall -G0 -I/usr/local/sce/common/include/
ASFLAGS  =
LDLIBS   = -ilb=usbmload.ilb -ilb=cdvdman.ilb
PROGNAME = usbloadf.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@



####################################################
# usbmload.irx の引数で設定ファイルを指定する場合  #
####################################################

# 設定ファイル：HOST , ドライバ：HOST
run00:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx conffile=host1:/usr/local/sce/conf/usb/usbdrvho.cnf debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME)

# 設定ファイル：HOST , ドライバ：CD
run01:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx conffile=host1:/usr/local/sce/conf/usb/usbdrvcd.cnf debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME)


##############################################
#      API で設定ファイルを指定する場合      #
##############################################

# 設定ファイル：HOST , ドライバ：HOST
run10:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME) load=APILOAD conffile=HOST driver=HOST

# 設定ファイル：HOST , ドライバ：CD
run11:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME) load=APILOAD conffile=HOST driver=CD

# 設定ファイル：CD , ドライバ：HOST
run12:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME) load=APILOAD conffile=CD driver=HOST

# 設定ファイル：CD , ドライバ：CD
run13:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME) load=APILOAD conffile=CD driver=CD


##############################################
#  設定ファイルを使わずに API で登録する場合 #
##############################################

# ドライバ：HOST
run20:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME) load=APIREGI driver=HOST

# ドライバ：CD
run21:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx debug=1 rbsize=8
	dsidb -nr -noexit mstart $(PROGNAME) load=APIREGI driver=CD

