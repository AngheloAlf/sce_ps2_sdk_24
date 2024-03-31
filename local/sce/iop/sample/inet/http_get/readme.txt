[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample that gets remote files using the http protocol and the inet library

<Description>
This sample program connects to a remote http port and downloads files using inet. 

[Caution]
In this sample program, an986.irx is required when using a USB Ether adapter.

Because the use of an986.irx is not allowed in titles (refer to the tech note on the developer support web site), it is not included with the release package (under sce/iop/modules). Download an986.irx from the developer support web site.

<Files>
        http.c        : Main program


<Run method>
        $ make              : Compile
        $ dsidb		    : Start dsidb
        > source inet.rc.{hdd,hdd.pppoe,usb,usb.pppoe}    : Initialize network
          Select the source file that corresponds to the environment you wish to use.
          ....
          inetctl: Waits until a display like "(ID=1) [ XXX.XXX.XXX.XXX ]" is output.
          (signals that the IP address has been set in the inet layer)

        > mstart http.irx [saddr]	     : Execute http.irx

        saddr is a WWW server address (www.scei.co.jp etc.).
        When the program is executed, HTTP is used to get the file http://<saddr>/index.html, then it is saved locally.

<Controller operation>
        None

<Notes>
The accompanying inet.rc.* files assume that the connection environment configuration is USB Ethernet, HDD Ethernet, PPPoE (USB Ethernet), or PPPoE (HDD Ethernet).

To change the IP address or the PPPoE user name, or to use PPP, change the configuration in the combination file (netXXX.cnf) that is specified for the inetctl.irx argument in each inet.rc.* file.

inet.rc.* references configuration files under /usr/local/sce/conf/net. This is because this program is for use as a development sample.  Please note that for an official title etc., individually encoded configuration files should be used. For information on how to use individually encoded configuration files, please refer to the sample configuration application.

In the current connection environment configuration files, only one interface is specified per file. Hence, in this sample program, it is assumed that multiple devices are not being used simultaneously. In this program, the target device works under the condition that only a single connection has been made. Please note that no considerations have been made regarding multiple connections for the same device.
      
<Process summary>
A summary of the process is given below.

        1. Start work thread.
        2. Get IP address.
        3. Connect to remote port.
        4. Send GET message.
        5. Receive data.

