reset 0 0
mstart /usr/local/sce/iop/modules/usbd.irx
mstart /usr/local/sce/iop/modules/usbmload.irx conffile=host1:/usr/local/sce/conf/usb/selfunld.cnf debug=1 rbsize=8
mstart ../activate/activate.irx Mouse
