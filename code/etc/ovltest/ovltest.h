// ovltest.h -- shared declarations for the ovltest demo.
// Resident utilities (defined in ovltest.c):
void init_rand();
unsigned int rand_x();
unsigned int rand_y();
unsigned int rand_c();
unsigned int rand_ex();
unsigned int rand_ey();
unsigned int rand_ec();
unsigned int rand_vc();
unsigned int rand_range_c(unsigned int range);
int kbhit();
void consume_key();
void setmode(int mode);
int sb_detect();
void opl_write(int reg, int val);
void opl_init();
void opl_note(int flo, int fhi);
void opl_off();

// Overlay-exported draw functions (defined in draw_cga/ega/vga.c):
void cga_phase();
void ega_phase();
void vga_phase();

// Overlay-exported sound functions (defined in snd_c4/e4/g4.c):
void play_c4();
void play_e4();
void play_g4();
