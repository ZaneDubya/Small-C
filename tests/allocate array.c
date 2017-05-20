
#define CNT_ARGPTR 8
int argptrs[CNT_ARGPTR];

allocatevars() {
    int i;
    fprintf(stdout, "Start: %u\n", avail(NO));
    for (i = 0; i < CNT_ARGPTR; i++) {
      argptrs[i] = allocvar(64, 1);
      fprintf(stdout, "Avail: %u\n", avail(NO));
    }
    fprintf(stdout, "Mid: %u\n", avail(NO));
    for (i = CNT_ARGPTR - 1; i >= 0; i--) {
      free(argptrs[i]);
      fprintf(stdout, "Avail: %u\n", avail(NO));
    }
    fprintf(stdout, "End: %u\n", avail(NO));
}