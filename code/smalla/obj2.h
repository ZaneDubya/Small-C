/*
** obj2.h -- second header for .OBJ file processing
*/

/*
** SEGDEF Parameters
*/
#define A_ABS    0  /* ablolute segment */
#define A_BYTE   1  /* byte aligned */
#define A_WORD   2  /* word aligned */
#define A_PARA   3  /* paragraph aligned */
#define A_PAGE   4  /* page  aligned */

#define C_NOT    0  /* does not combine */
#define C_PUBLIC 2  /* concatenates */
#define C_STACK  5  /* concatenates */
#define C_COMMON 6  /* overlaps */

#define B_NOTBIG 0  /* not a big (64k) segment */
#define B_BIG    1  /* is a big (64k) segment */

/*
** FIXUPP Parameters
*/
#define F_M_SELF     0  /* self-rel fixup mode */
#define F_M_SEGMENT  1  /* segment-rel fixup mode */

#define F_L_LO       0  /* fix lo-byte */
#define F_L_OFF      1  /* fix offset */
#define F_L_BASE     2  /* fix base */
#define F_L_PTR      3  /* fix pointer */
#define F_L_HI       4  /* fix hi-byte */

#define F_F_SI       0  /* frame given by segment index */
#define F_F_GI       1  /* frame given by group index */
#define F_F_EI       2  /* frame given by external index */
#define F_F_LOC      4  /* frame is that of the location being fixed */
#define F_F_TAR      5  /* frame is determined by the target */
#define F_F_TH0      8  /* frame given by thread 0 */
#define F_F_TH1      9  /* frame given by thread 1 */
#define F_F_TH2     10  /* frame given by thread 2 */
#define F_F_TH3     11  /* frame given by thread 3 */

#define F_T_SID      0  /* target given by segment index + displ */
#define F_T_GID      1  /* target given by group index + displ */
#define F_T_EID      2  /* target given by external index + displ */
#define F_T_SI0      4  /* target given by segment index alone */
#define F_T_GI0      5  /* target given by group index alone */
#define F_T_EI0      6  /* target given by external index alone */
#define F_T_TH0      8  /* target given by thread 0 */
#define F_T_TH1      9  /* target given by thread 1 */
#define F_T_TH2     10  /* target given by thread 2 */
#define F_T_TH3     11  /* target given by thread 3 */

/*
** THREAD Parameters
*/
#define T_T_TH0      0  /* define thread 0 */
#define T_T_TH1      1  /* define thread 1 */
#define T_T_TH2      2  /* define thread 2 */
#define T_T_TH3      3  /* define thread 3 */

#define T_M_T_SI     0  /* target given by segment index */
#define T_M_T_GI     1  /* target given by group index */
#define T_M_T_EI     2  /* target given by external index */
#define T_M_F_SI    16  /* frame given by segment index */
#define T_M_F_GI    17  /* frame given by group index */
#define T_M_F_EI    18  /* frame given by external index */
#define T_M_F_LOC   20  /* frame is that of the location being fixed */
#define T_M_F_TAR   21  /* frame is determined by the target */

/*
** MODEND Attribute
*/
#define M_A_NN       0  /* non-main module, no start addr */
#define M_A_NA       1  /* non-main module, start addr */
#define M_A_MN       2  /* main module, no start addr */
#define M_A_MA       3  /* main module, start addr */
