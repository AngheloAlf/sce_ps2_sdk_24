[SCE CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.5
                Copyright (C) 2001 Sony Computer Entertainment Inc.
                                                   All Rights Reserved

Network Setting File Samples

The following network setting file samples are stored in
/usr/local/sce/conf/net.

net000.cnf - Combination of ifc000.cnf and dev000.cnf
net001.cnf - Combination of ifc001.cnf and dev001.cnf
net002.cnf - Combination of ifc002.cnf and dev002.cnf
net003.cnf - Combination of ifc003.cnf and dev003.cnf
net004.cnf - Combination of ifc004.cnf and dev003.cnf
net005.cnf - Combination of ifc005.cnf and dev002.cnf
net006.cnf - Combination of ifc004.cnf and dev002.cnf

ifc000.cnf - type eth sample
ifc001.cnf - type ppp sample
ifc002.cnf - type nic sample
ifc003.cnf - type eth sample (use dhcp)
ifc004.cnf - pppoe sample
ifc005.cnf - type nic sample (use dhcp)

dev000.cnf - type eth sample
dev001.cnf - type ppp sample
dev002.cnf - type nic sample
dev003.cnf - type eth sample

* ???000.cnf to ???001.cnf are enabled by appropriately renaming vendor
  and product in dev files and by appropriately changing values of IP
  Address, User ID and Password in ifc files.

* ???002.cnf is enabled by appropriately changing values of IP Address
  and others in ifc files.

* ???003.cnf and ???005.cnf can be used as is with sample programs.

* ???004.cnf and ???006.cnf are enabled by appropriately changing values
  of User ID, Password and others in ifc files.

<net.db>
In the configuration management file net.db, data obtainable from 
the sceNetCnfList structure is arranged using ','.
(To type and stat, code conversion between ASCII and binary is applied.) 
For the sceNetCnfList structure, see "IOP Library Reference" -> "Common 
Network Setting Library (ntcnf_rf)".
For the usage of the configuration management file (e.g. addition of an 
entry), refer to sce/iop/sample/inet/setapp. 
