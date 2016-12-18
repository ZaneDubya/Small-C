..\bin\ar -x ..\ar\clib.arc
copy ..\smallc22\stdio.h stdio.h
..\bin\cc ABS -a -p
..\bin\cc ATOI -a -p
..\bin\cc ATOIB -a -p
..\bin\cc AUXBUF -a -p
..\bin\cc AVAIL -a -p
..\bin\cc BSEEK -a -p
..\bin\cc BTELL -a -p
..\bin\cc CALLOC -a -p
..\bin\cc CLEARERR -a -p
..\bin\cc CLIB.H, CSEEK -a -p
..\bin\cc CSYSLIB -a -p
..\bin\cc CTELL -a -p
..\bin\cc DTOI -a -p
..\bin\cc EXIT -a -p
..\bin\cc FCLOSE -a -p
..\bin\cc FEOF -a -p
..\bin\cc FERROR -a -p
..\bin\cc FGETC -a -p
..\bin\cc FGETS -a -p
..\bin\cc FOPEN -a -p
..\bin\cc FPRINTF -a -p
..\bin\cc FPUTC -a -p
..\bin\cc FPUTS -a -p
..\bin\cc FREAD -a -p
..\bin\cc FREE -a -p
..\bin\cc FREOPEN -a -p
..\bin\cc FSCANF -a -p
..\bin\cc FWRITE -a -p
..\bin\cc GETARG -a -p
..\bin\cc GETCHAR -a -p
..\bin\cc IS -a -p
..\bin\cc ISASCII -a -p
..\bin\cc ISATTY -a -p
..\bin\cc ISCONS -a -p
..\bin\cc ITOA -a -p
..\bin\cc ITOAB -a -p
..\bin\cc ITOD -a -p
..\bin\cc ITOO -a -p
..\bin\cc ITOU -a -p
..\bin\cc ITOX -a -p
..\bin\cc LEFT -a -p
..\bin\cc LEXCMP -a -p
..\bin\cc MALLOC -a -p
..\bin\cc OTOI -a -p
..\bin\cc PAD -a -p
..\bin\cc POLL -a -p
..\bin\cc PUTCHAR -a -p
..\bin\cc PUTS -a -p
..\bin\cc RENAME -a -p
..\bin\cc REVERSE -a -p
..\bin\cc REWIND -a -p
..\bin\cc SIGN -a -p
..\bin\cc STRCAT -a -p
..\bin\cc STRCHR -a -p
..\bin\cc STRCMP -a -p
..\bin\cc STRCPY -a -p
..\bin\cc STRLEN -a -p
..\bin\cc STRNCAT -a -p
..\bin\cc STRNCMP -a -p
..\bin\cc STRNCPY -a -p
..\bin\cc STRRCHR -a -p
..\bin\cc TOASCII -a -p
..\bin\cc TOLOWER -a -p
..\bin\cc TOUPPER -a -p
..\bin\cc UNGETC -a -p
..\bin\cc UNLINK -a -p
..\bin\cc UTOI -a -p
..\bin\cc XTOI -a -p
 
..\bin\asm ABS /p
..\bin\asm ATOI /p
..\bin\asm ATOIB /p
..\bin\asm AUXBUF /p
..\bin\asm AVAIL /p
..\bin\asm BSEEK /p
..\bin\asm BTELL /p
..\bin\asm CALL /p
..\bin\asm CALLOC /p
..\bin\asm CLEARERR /p
..\bin\asm CLIB.H
..\bin\asm CSEEK /p
..\bin\asm CSYSLIB /p
..\bin\asm CTELL /p
..\bin\asm DTOI /p
..\bin\asm EXIT /p
..\bin\asm FCLOSE /p
..\bin\asm FEOF /p
..\bin\asm FERROR /p
..\bin\asm FGETC /p
..\bin\asm FGETS /p
..\bin\asm FOPEN /p
..\bin\asm FPRINTF /p
..\bin\asm FPUTC /p
..\bin\asm FPUTS /p
..\bin\asm FREAD /p
..\bin\asm FREE /p
..\bin\asm FREOPEN /p
..\bin\asm FSCANF /p
..\bin\asm FWRITE /p
..\bin\asm GETARG /p
..\bin\asm GETCHAR /p
..\bin\asm IS /p
..\bin\asm ISASCII /p
..\bin\asm ISATTY /p
..\bin\asm ISCONS /p
..\bin\asm ITOA /p
..\bin\asm ITOAB /p
..\bin\asm ITOD /p
..\bin\asm ITOO /p
..\bin\asm ITOU /p
..\bin\asm ITOX /p
..\bin\asm LEFT /p
..\bin\asm LEXCMP /p
..\bin\asm MALLOC /p
..\bin\asm OTOI /p
..\bin\asm PAD /p
..\bin\asm POLL /p
..\bin\asm PUTCHAR /p
..\bin\asm PUTS /p
..\bin\asm RENAME /p
..\bin\asm REVERSE /p
..\bin\asm REWIND /p
..\bin\asm SIGN /p
..\bin\asm STRCAT /p
..\bin\asm STRCHR /p
..\bin\asm STRCMP /p
..\bin\asm STRCPY /p
..\bin\asm STRLEN /p
..\bin\asm STRNCAT /p
..\bin\asm STRNCMP /p
..\bin\asm STRNCPY /p
..\bin\asm STRRCHR /p
..\bin\asm TOASCII /p
..\bin\asm TOLOWER /p
..\bin\asm TOUPPER /p
..\bin\asm UNGETC /p
..\bin\asm UNLINK /p
..\bin\asm UTOI /p
..\bin\asm XTOI /p

ECHO === Linking SmallC Library ===
..\bin\link ABS ATOI ATOIB AUXBUF AVAIL BSEEK BTELL CALL CALLOC CLEARERR CSEEK CSYSLIB CTELL DTOI EXIT FCLOSE FEOF FERROR FGETC FGETS FOPEN FPRINTF FPUTC FPUTS FREAD FREE FREOPEN FSCANF FWRITE GETARG GETCHAR IS ISASCII ISATTY ISCONS ITOA ITOAB ITOD ITOO ITOU ITOX LEFT LEXCMP MALLOC OTOI PAD POLL PUTCHAR PUTS RENAME REVERSE REWIND SIGN STRCAT STRCHR STRCMP STRCPY STRLEN STRNCAT STRNCMP STRNCPY STRRCHR TOASCII TOLOWER TOUPPER UNGETC UNLINK UTOI XTOI,clib,clib
if errorlevel 1 goto exit