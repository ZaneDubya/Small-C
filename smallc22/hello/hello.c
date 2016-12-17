

main() {
    printf("hello world\n");
}


printint(i) int i; /* int i;*/ {
    char a, b, c, d, e;
    a = (i % 10) + 0x30;
    b = (i / 10) % 10 + 0x30;
    c = (i / 100) % 10 + 0x30;
    d = (i / 1000) % 10 + 0x30;
    e = (i / 10000) % 10 + 0x30;
    fputc(e, stderr);
    fputc(d, stderr);
    fputc(c, stderr);
    fputc(b, stderr);
    fputc(a, stderr);
    fputc(NEWLINE, stderr);
    return;
}

printchar(i) char i; /* int i;*/ {
    char a, b, c;
    a = (i % 10) + 0x30;
    b = (i / 10) % 10 + 0x30;
    c = (i / 100) % 10 + 0x30;
    fputc(c, stderr);
    fputc(b, stderr);
    fputc(a, stderr);
    fputc(NEWLINE, stderr);
    return;
}