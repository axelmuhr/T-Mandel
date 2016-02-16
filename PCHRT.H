/*---------------------------------------------------------------------------
 |  PCHRT.H                                                                 |
 |                                                                          |
 |  Compiler independant header module for PCHRT in a C environment.        |
 |                                                                          |
 |  (c)1988,1989 Ryle Design, P.O. Box 22, Mt. Pleasant, Michigan 48804     |
 |                                                                          |
 |  V3.00  Turbo C Shareware Evaluation Version                             |
 ---------------------------------------------------------------------------*/

#ifndef TRUE
    #define TRUE    1
    #define FALSE   0
#endif

#define NUMVEC          9               /* # of BIOS interrupts we can profile  */

#define CRT10           1               /* BIOS timer bitmask switches        */
#define DISK            2
#define COM             4
#define KEYBD           8
#define PRT             16
#define DOS21           32
#define MOUSE           64
#define EMS             128
#define USER            256
                                                                                                
#define INTS_ON         1               /* timer modes - default with interrupts on             */
#define NO_INTS_ON      2               /* short duration with interrupts off                   */
#define DEF_NUMTIMERS   10              /* default number of timers - override with t_request() */

#define CONTROL_PORT    0x43            /* 8253 control registers */
#define DATA_PORT       0x40

#define HIGHWATER       0               /* timer report type specifiers */
#define NONZERO         1

#define TIMERSTACK      20              /* size of BIOS timer stack     */

typedef char tname[21];                 /* timer name data type         */

typedef struct							/* hires timestamp              */
{
	unsigned int	ticks;				/* 8253 838ns "ticks"           */
	unsigned int	tocks;              /* BIOS TOD 54.925 ms "tocks"   */
}
pchrt_type;


typedef struct
{
    long unsigned   entry;              /* timer entry count         */
    long unsigned   elapsed;            /* timer elapsed time count  */
    tname           timername;          /* timer name                */
    pchrt_type      tstart;             /* timer entry timestamp     */
    pchrt_type      tstop;              /* timer exit timestamp      */
}
pchrt_data;


typedef struct                                  /* bios timer data structure             */
{
    int             which_int;                  /* interrupt vector timer               */
    char            int_name[31];               /* description of interrupt functionality  */
    int             enabled;                    /* interrupt timer state flag            */
    unsigned char   tstack[TIMERSTACK];         /* timer stack for recursive calls         */
    unsigned char   tstptr;                     /* timer stack pointer                     */
    tname           func_name[256];             /* interrupt function name                 */
    long unsigned   entry[256];                 /* # of activations for each timer         */
    long unsigned   elapsed[256];               /* total elapsed for each timer            */
    pchrt_type      bstart[256];                /* entry timestamps                        */
    pchrt_type      bstop[256];                 /* exit timestamps                         */
}
tbios_data;


typedef struct                                  /* data structure for delay function */
{
    int     inner;                              /* inner loop count */
    int     outer;                              /* outer loop count */
}
tdelay_type;


/*---------------------------------------------------------------------------
 |                        global variable definitions                       |
 ---------------------------------------------------------------------------*/

extern pchrt_data   *tdata;                      /* timer data struct       */
extern tbios_data   *i10data;                    /* INT 10h timer data      */
extern tbios_data   *i13data;                    /* INT 13h timer data      */
extern tbios_data   *i14data;                    /* INT 14h timer data      */
extern tbios_data   *i16data;                    /* INT 16h timer data      */
extern tbios_data   *i17data;                    /* INT 17h timer data      */
extern tbios_data   *i21data;                    /* INT 21h timer data      */
extern tbios_data   *i33data;                    /* INT 33h timer data      */
extern tbios_data   *i67data;                    /* INT 67h timer data      */
extern tbios_data   *iuserdata;                  /* user int timer data     */

extern char         pchrt_fname[67];            /* default report file name */
extern char         pchrt_title[81];            /* default report title     */

extern char         tbios_fname[67];            /* default bios report file name                            */
extern char         tbios_title[81];            /* default bios report title                                */
extern char         tbios_user_file[67];        /* file containing user interrupt function descriptions     */

extern char         t_timermode;                /* flag for timer mode - INTS_ON or NO_INTS_ON  */
extern int          t_maxtimer;                 /* # of timers available                        */
extern int          t_maxentry;                 /* timer highwater mark                         */
extern int          t_report_type;              /* timer report type                            */ 
extern int          tbios_user_int;             /* user specified bios interrupt timer          */
extern int          tbios_mask;                 /* global bios timer mask                       */
extern int          t_valid;                    /* timer data valid flag                        */

extern long unsigned    t_iohead;               /* overhead in INTS_ON mode     */
extern long unsigned    t_noint_iohead;         /* overhead in NOINTS_ON mode   */
extern long unsigned    tdelay_ohead;           /* t_delay calling overhead     */
extern long unsigned    tdelay_inner;           /* t_delay inner loop time      */
extern long unsigned    tdelay_outer;           /* t_delay outer loop time      */

extern float            tdelay_ff;              /* t_delay "fudge factor"       */


/*---------------------------------------------------------------------------
 |                            function prototypes                           |
 ---------------------------------------------------------------------------*/

/* timer functions */

void          t_request(int numtimer);
int           t_start(void);
void          t_get(pchrt_type* timestamp);
void          t_stop(void);
void          t_hires_entry(pchrt_type *timestamp);
void          t_hires_exit(pchrt_type *timestamp);
void          t_setmode(int modetype);
long unsigned t_diff(pchrt_type *start, pchrt_type *stop);
void          t_calib(void);
void          t_entry(int timernum);
void          t_exit(int timernum);
void          t_name(int timernum, char* timername);
void          t_set_report(int whichkind);
void          t_report(int whereto);
char *        t_cvt_time(unsigned long usecs, char *time_string);
void          t_fname(char *filename);
void          t_rname(char *report_title);
void          t_reset(int tnum);
void          t_ask_timer(int tnum, long unsigned *hits, long unsigned *elapsed);
void          t_suspend(int tnum);
void          t_resume(int tnum);

/* bios timer functions */

int  t_bios_start(int biosmask);
void t_bios_stop(void);
void t_bios_suspend(int biosmask);
void t_bios_resume(int biosmask);
void t_bios_report(int whereto);
void t_bios_ask(int whichint, int whichfunc, unsigned long *hits, unsigned long *elapsed);
void t_bios_reset(int whichint, int whichfunc);
void t_bios_fname(char *fname);
void t_bios_rname(char *rname);
void t_bios_set_user(int user_int);
void t_bios_set_file(char *user_int_file);
void t_bios_load_desc(int whichint, char *data_file);
void t_bios_entry(int whichint, int whichfunc);
void t_bios_exit(int whichint);
void t_hook_int(int int_no);
void t_unhook_int(int int_no);

/* delay functions */

void            t_delay_calib(void);
float           t_calc_delay_ff(unsigned long how_long, int ints_or_not);
float           t_get_delay_ff(void);
void            t_set_delay_ff(float delay_ff);
void            t_ask_delay(long unsigned usec, tdelay_type *dp);
long unsigned   t_min_delay(void);
long unsigned   t_res_delay(void);
void            t_do_delay(tdelay_type *delay_params);
void            t_do_delay_wints(tdelay_type *delay_params);


/*---------------------------------------------------------------------------
                                  end pchrt.h
 ---------------------------------------------------------------------------*/
