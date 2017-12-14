SET LIB=..\..\smalllib

ylink work\hello.obj,%LIB%\clib.lib -e=y-hello.exe
ylink work\cc1.obj,work\cc2.obj,work\cc3.obj,work\cc4.obj,%LIB%\clib.lib -e=y-cc.exe