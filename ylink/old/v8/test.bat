SET LIB=..\..\smalllib

ylink work\hello.obj,%LIB%\clib.lib -e=y-hello.exe -d=dbghello.txt
ylink work\cc1.obj,work\cc2.obj,work\cc3.obj,work\cc4.obj,%LIB%\clib.lib -d=dbgcc.txt -e=y-cc.exe