/******************************** SMALLMAN.C *********************************
;  (C) Copyright 1987-1993 by Computer System Architects, Provo UT.         *
;  This  program is property of Computer System Architects (CSA)            *
;  and is provided only as an example of a transputer/PC program for use    *
;  with CSA's Transputer Education Kit and other transputer products.	    *
;  You may freely distribute copies, modifiy whole or part of program       *
;  provided you insert in each copy appropriate copyright notices and       *
;  disclaimer of warranty.						    *
;  This program is provided as is without warranty of any kind. CSA is not  *
;  responsible for any damages arising out of the use of this program.      *
;***************************************************************************/

/* compile: tcx file -cf1p0rv */

#include "c:\trans\lsc\89\include\conc.h"

/*{{{  notes on compiling and linking*/
/*
This module is compiled as indicated above so it will be relocatable.
It is then linked with main (not _main) specified as the entry point.
This is because the fload loader loads the code in and thens jumps
to the first byte of the code read in.  By specifying main as the
entry point the code is correctly linked and the first instruction of
main will be the first byte in the code.
*/
/*}}}  */
/*{{{  notes on overlay*/
/*

This version of the mandelbrot uses overlays so that it can fit in the
2k internal RAM available on the T400 and T414.  The procedure main sets
up the workspaces for the parallel processes and then stops.  Since it
only runs at the first mains code can be overwritten once it is executed.
also the structure passed into main is only used in the first half of main
so after that it can be also be overwritten.  There is also space at the
bottom of the stack that is used as workspace for a process.  The workspaces
not indicated below are declared as static int arrays in the code and so
reside in the same space.  A note here.  The workspace for job is declared
but is declared too small.  This is because it then uses part of the
memory that contains the code for the main process.

Below is a diagram showing the memory map at startup and as it is after
setup when it is doing computation.


#80000800+------------------------+      #80000800+-----------------------+
	 | unused                 |               | unused                |
#80000768+------------------------+      #80000768+-----------------------+
	 | heap for ChanAlloc     |               | heap for ChanAlloc    |
#80000724+------------------------+      #80000724+-----------------------+
	 | rest of procedure      |               | rest of procedure     |
	 | code and in line       |               | code and in line      |
	 | workspace declarations |               | workspace declarations|
#800003FA+------------------------+      #800003FA+-----------------------+
	 | job code               |               | jobcode               |
#80000360+------------------------+      #80000360+-----------------------+
	 | jobws                  |               |                       |
	 |                        |               | jobws                 |
#800002C0+------------------------+               |                       |
	 |                        |      #80000278+-----------------------+
	 |                        |               | arbiterws             |
	 | main                   |               |                       |
	 |                        |      #800001D8+-----------------------+
         |                        |               | bufws[2]              |
#80000188+------------------------+      #80000188+-----------------------+
         | parameters to main     |               | bufws[1]              |
#80000148+------------------------+      #80000138+-----------------------+
         | stack space for main   |               | unused                |
         +------------------------+               +-----------------------+
         | not used               |               | bufws[0]              |
#80000070+------------------------+      #80000070+-----------------------+
         | reserved               |               | reserved              |
#80000000+------------------------+      #80000000+-----------------------+
*/
/*}}}  */

/*{{{  define constants*/
#define TRUE  1
#define FALSE 0

#define JOBCOM 0
#define PRBCOM 1
#define DATCOM 2
#define RSLCOM 3
#define FLHCOM 4

#define MAXPIX  64
#define BUFSIZE 19
#define PRBSIZE 12

/*#define JOBWSZ (69*4)*/
#define JOBWSZ (40*4)   /* actual size is bigger but some of the space that*/
                        /* that contained the code for main is over written*/
#define BUFWSZ (20*4)
#define FEDWSZ (24*4)
#define ARBWSZ (38*4)
#define SELWSZ (29*4)

#define THRESH 2.0e-7
#define loop for (;;)

/*}}}  */
/*{{{  lights macro*/
/*#pragma define LIGHTS*/

#pragma asm
        .T800
        .NOEMULATE
#pragma endasm

#pragma macro lon()
#pragma ifdef LIGHTS
#pragma asm
        LDC     0
        LDC     0
        STNL    0
#pragma endasm
#pragma endif
#pragma endmacro

#pragma macro loff()
#pragma ifdef LIGHTS
#pragma asm
        LDC     1
        LDC     0
        STNL    0
#pragma endasm
#pragma endif
#pragma endmacro
/*}}}  */
/*{{{  struct type def*/

typedef struct {
    int id;
    void *minint;
    void *memstart;
    Channel *up_in;
    Channel *dn_out[3];
    void *ldstart;
    void *entryp;
    void *wspace;
    void *ldaddr;
    void *trantype;
} LOADGB;

/*}}}  */
/*{{{  main(...)*/

main(ld)
LOADGB *ld;
{
    int i;

    Channel *si,*so[5],*sr[5],*ai[5];
    extern int job(),buffer(),feed(),arbiter(),selector();
    extern int jobws[JOBWSZ/4];
    extern int selws[SELWSZ/4];
    extern int fedws[FEDWSZ/4];
    int *bufws[3], *arbws;

    bufws[0] = (int*) ld->memstart;
    bufws[1] = ((int*) main) - (BUFWSZ/4);
    bufws[2] = (int*) main;
    arbws    = ((int*) main) + (BUFWSZ/4);

    sr[0] = ChanAlloc();
    so[0] = ChanAlloc();
    ai[0] = ChanAlloc();
    PRun(PSetup(jobws,job,JOBWSZ,3,sr[0],so[0],ai[0])|1);
    if (ld->id)
        si = ld->up_in;
    else  {
        si = ChanAlloc();
        PRun(PSetup(fedws,feed,FEDWSZ,3,ld->up_in,si));
        }
    for (i = 0; i < 3 && ld->dn_out[i]; i++)
        {
        sr[i+1] = ChanAlloc();
        so[i+1] = ChanAlloc();
        ai[i+1] = ld->dn_out[i]+4;
        PRun(PSetup(bufws[i],buffer,BUFWSZ,3,sr[i+1],so[i+1],ld->dn_out[i]));
	}
    sr[i+1] = so[i+1] = ai[i+1] = 0;
    PRun(PSetup(selws,selector,SELWSZ,3,si,sr,so));
    PRun(PSetup(arbws,arbiter,ARBWSZ,2,ai,ld->up_in-4));
    PStop();
}
/*}}}  */
/*{{{  job(...)*/

int jobws[JOBWSZ/4];

job(req_out,job_in,rsl_out)
Channel *req_out,*job_in,*rsl_out;
{
    int (*iter)();
    int i,len,pixvec,maxcnt;
    int ilox,igapx,iloy,igapy;
    double dlox,dgapx,dloy,dgapy;
    int buf[BUFSIZE];
    unsigned char *pbuf = (unsigned char *)(buf+3);

    loop
        {
        ChanOutInt(req_out,0);
	len = ChanInInt(job_in);
        ChanIn(job_in,(char *)buf,len);
        if (buf[0] == JOBCOM)
            {  int x,y;
            lon();
            pixvec = buf[3];
            if (igapx && igapy)
                {
                y = igapy*buf[2]+iloy;
                for (i = 0; i < pixvec; i++)
                    {
                    x = igapx*(buf[1]+i)+ilox;
		    pbuf[i] = iterFIX(x,y,maxcnt);
                    }
                }
            else
                {
                for (i = 0; i < pixvec; i++) pbuf[i] = 1;
                }
            len = pixvec+3*4;
            buf[0] = RSLCOM;
            loff();
            }
        else if (buf[0] == DATCOM)
	    {
            maxcnt = buf[2];
            ilox = fix(buf+3);
            iloy = fix(buf+5);
            igapx = fix(buf+7);
            igapy = fix(buf+9);
            continue;
            }
        ChanOutInt(rsl_out,len);
        ChanOut(rsl_out,(char *)buf,len);
        }
}

/*}}}  */
/*{{{  int iterFIX(...)*/
int iterFIX(cx,cy,maxcnt)
int cx,cy,maxcnt;
{
    int cnt,zx,zy,zx2,zy2,tmp;

#pragma asm
        .VAL    maxcnt, ?ws+3
        .VAL    cy,     ?ws+2
        .VAL    cx,     ?ws+1
	.VAL    tmp,    ?ws-1
        .VAL    zy2,    ?ws-2
        .VAL    zx2,    ?ws-3
        .VAL    zy,     ?ws-4
        .VAL    zx,     ?ws-5
        .VAL    cnt,    ?ws-6
        ; zx = zy = zx2 = zy2 = cnt = 0;
        LDC     0
        STL     zx
        LDC     0
        STL     zy
        LDC     0
	STL     zx2
        LDC     0
        STL     zy2
        LDC     0
        STL     cnt
LOOP:
        ; cnt++;
        LDL     cnt
        ADC     1
        STL     cnt
        ; if (cnt >= maxcnt) goto END;
        LDL     maxcnt
	LDL     cnt
        GT
        CJ      @END
        ; tmp = zx2 - zy2 + cx;
        LDL     zx2
        LDL     zy2
        SUB
        LDL     cx
        ADD
        STL     tmp
        ; zy = zx * zy * 2 + cy;
        LDL     zx
	LDL     zy
        FMUL
        XDBLE
        LDC 3
        LSHL
        CSNGL
        LDL     cy
        ADD
        STL     zy
        ; tmp = zx;
        LDL     tmp
        STL     zx
	; zx2 = zx * zx;
        LDL     zx
        LDL     zx
        FMUL
        XDBLE
        LDC 2
        LSHL
        CSNGL
        STL     zx2
        ; zy2 = zy * zy;
        LDL     zy
        LDL     zy
	FMUL
        XDBLE
        LDC     2
        LSHL
        CSNGL
        STL     zy2
        ; if (zx2 + zy2 >= 4.0) goto END;
        LDL     zx2
        LDL     zy2
        ADD
        TESTERR
        CJ      @END
	J       @LOOP
END:
        ; if (cnt != maxcnt) return(cnt);
        LDL     cnt
        LDL     maxcnt
        DIFF
        CJ      @RETN
        LDL     cnt
        .RETF   ?ws
RETN:
        ; return(0);
        .RETF   ?ws
#pragma endasm
}

/*}}}  */
/*{{{  int fix(x)*/
int fix(x)
int *x;
{
    int e,tmp;

#pragma asm
        .VAL    x,?ws+1
	.VAL    tmp,?ws-1
        .VAL    e,?ws-2
        ; e = ((x[1] >> 20) & 0x7ff) - 1023 + 9;
        LDL     x
        LDNL    1
        .LDC    20
        SHR
        .LDC    0x7ff
        AND
        ADC     (-1023)+(+9)
        STL     e
        ; reg = (x[1] & 0xfffff) | 0x100000;
	LDL     x
        LDNL    1
        .LDC    0xfffff
        AND
        .LDC    0x100000
        OR
        ; if (e < 0) { F2;
        .LDC    0
        LDL     e
        GT
        CJ      @F2
        ; if (e > -21) { F1;
	LDL     e
        .LDC    -21
        GT
        CJ      @F1
        ; reg >>= -e;
        LDL     e
        NOT
        ADC     1
        SHR
F1:
        ; } else reg = 0;
        .LDC    0
	CJ      @F4
F2:
        ; } else if (e < 11) { F3;
        ADD
        .LDC    +11
        LDL     e
        GT
        CJ      @F3
        ; lreg <<= e;
        LDL     x
        LDNL    0
        LDL     e
	LSHL
        STL     tmp
        .LDC    0
        CJ      @F4
F3:
        ; } else reg = 0x7fffffff;
        .LDC    0x7fffffff
        .LDC    0
F4:
        ; if (x[1] < 0) return(-reg);
        LDL     x
        LDNL    1
	GT
        CJ      @F5
        NOT
        ADC     1
        .RETF   ?ws
F5:
        ; return(reg);
        ADD
        .RETF   ?ws
#pragma endasm
}

/*}}}  */
/*{{{  buffer(...)*/
buffer(req_out,buf_in,buf_out)
Channel *req_out,*buf_in,*buf_out;
{
    int len;
    int buf[PRBSIZE-1];

    loop {
        ChanOutInt(req_out,0);
        len = ChanInInt(buf_in);
        ChanIn(buf_in,(char *)buf,len);
	ChanOutInt(buf_out,len);
        ChanOut(buf_out,(char *)buf,len);
    }
}

/*}}}  */
/*{{{  arbiter(...)*/
arbiter(arb_in,arb_out)
Channel **arb_in,*arb_out;
{
    int i,len,cnt,pri;
    int buf[BUFSIZE];

    cnt = pri = 0;
    loop {
        i = ProcPriAltList(arb_in,pri);
        pri = (arb_in[pri+1]) ? pri+1 : 0;
        len = ChanInInt(arb_in[i]);
        ChanIn(arb_in[i],(char *)buf,len);
        if (buf[0] == FLHCOM)
            if (arb_in[++cnt]) continue; else cnt = 0;
        ChanOutInt(arb_out,len);
        ChanOut(arb_out,(char *)buf,len);
    }
}

/*}}}  */
/*{{{  selector(...)*/
int selws[SELWSZ/4];


selector(sel_in,req_in,dn_out)
Channel *sel_in,**req_in,**dn_out;
{
    int i,len;
    int buf[PRBSIZE-1];

    loop {
        len = ChanInInt(sel_in);
        ChanIn(sel_in,(char *)buf,len);
        if (buf[0] == JOBCOM) {
            i = ProcPriAltList(req_in,0);
            ChanInInt(req_in[i]);
            ChanOutInt(dn_out[i],len);
            ChanOut(dn_out[i],(char *)buf,len);
        } else {
            for (i = 0; req_in[i]; i++) {
                ChanInInt(req_in[i]);
                ChanOutInt(dn_out[i],len);
                ChanOut(dn_out[i],(char *)buf,len);
            }
        }
    }
}

/*}}}  */
/*{{{  feed(...)*/
int fedws[FEDWSZ/4];

feed(fd_in,fd_out)
Channel *fd_in,*fd_out;
{
    int len;
    int buf[PRBSIZE];

    loop {
        len = ChanInInt(fd_in);
        ChanIn(fd_in,(char *)buf,len);
        if (buf[0] == PRBCOM) {
            /*{{{  */
            int width,height,multiple;
            width = buf[1];
            height = buf[2];
            buf[1] = DATCOM;
            ChanOutInt(fd_out,(PRBSIZE-1)*4);
            ChanOut(fd_out,(char *)&buf[1],(PRBSIZE-1)*4);
            buf[0] = JOBCOM;
            multiple = width/MAXPIX*MAXPIX;
            for (buf[2] = 0; buf[2] < height; buf[2]++) {
                for (buf[1] = 0; buf[1] < width; buf[1]+=MAXPIX) {
                    buf[3] = (buf[1] < multiple) ? MAXPIX : width-multiple;
                    ChanOutInt(fd_out,4*4);
                    ChanOut(fd_out,(char *)buf,4*4);
                }
            }
            buf[0] = FLHCOM;
            len = 1*4;
            }
            /*}}}  */
        ChanOutInt(fd_out,len);
        ChanOut(fd_out,(char *)buf,len);
    }
}
/*}}}  */
