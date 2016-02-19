/********************************* MAN.C ************************************
*  (C) Copyright 1987-1993  Computer System Architects, Provo UT.           *
*  This Mandelbrot program is the property of Computer System Architects    *
*  (CSA) and is provided only as an example of a transputer/PC program for  *
*  use  with CSA's Transputer Education Kit and other transputer products.  *
*  You may freely distribute copies or modifiy the program as a whole or in *
*  part, provided you insert in each copy appropriate copyright notices and *
*  disclaimer of warranty and send to CSA a copy of any modifications which *
*  you plan to distribute.                                                  *
*  This program is provided as is without warranty of any kind. CSA is not  *
*  responsible for any damages arising out of the use of this program.      *
****************************************************************************/
/****************************************************************************
* This program man.c is the main part of mandelbrot. It includes setting up *
* the VGA/CGA screen and user options. It calls appropiate driver functions *
* to reset and send data to and from network of transputers.                *
****************************************************************************/


#include <dos.h>
#include <stdio.h>
#include <math.h>
#include "lkio.h"
#include "pchrt.h"


#define TRUE  1
#define FALSE 0

#define LINK_BASE 0x150
#define DMA_CHAN  1

#define JOBCOM 0L
#define PRBCOM 1L
#define DATCOM 2L
#define RSLCOM 3L
#define FLHCOM 4L

#define F1   0x13b
#define F2   0x13c
#define F3   0x13d
#define F4   0x13e
#define F5   0x13f
#define F6   0x140
#define F7   0x141
#define F8   0x142
#define LARW 0x14b
#define RARW 0x14d
#define UARW 0x148
#define DARW 0x150
#define PGUP 0x149
#define PGDN 0x151
#define HOME 0x147
#define END  0x14f
#define INS  0x152
#define DEL  0x153
#define ESC  0x1b
#define ENTR 0x0d

#define INSM    0x200
#define MAXPIX  64
#define BUFSIZE 20
#define REAL    double
#define loop    for (;;)

#define AUTOFILE "man.dat"
#define PAUSE    3

#define HIR  2.5
#define LOR  3.0e-14
#define f(x) sqrt(log(x)-log(LOR)+1.0)

typedef unsigned int uint;

void com_loop(void);
void auto_loop(void);
void scan_tran(void);
void scan_host(void);
int iterate(REAL,REAL,int);
int calc_iter(double);
void init_window(void);
void save_coord(void);
void zoom(int *);
void region(int *, int *, int *, int *, int *);
int counts(void);
int get_key(void);
void waitsec(int);
void draw_box(int,int,int,int);
void ibm_mode(int);
void hrc_graph_mode(void);
void hrc_text_mode(void);
void ega_init_pal(void);
void ega_set_pal(int,int);
void cga_set_pal(int,int);
void h_line(int,int,int,int);
void v_line(int,int,int,int);
void clear_scr(void);
void ega_rect(uint,uint,uint,uint,uint);
void cga_rect(uint,uint,uint,uint,uint);
void hrc_rect(uint,uint,uint,uint,uint);
void boot_mandel(void);
int load_buf(char *, int);
extern exit(int);
extern getch(void);
extern kbhit(void);
extern time(long *);
extern void hrc_vect();
extern void cga_vect();
extern void ega_vect();
extern void vga_vect();
/*
extern scan_tran(),scan_host();
*/


static autz;
static verbose;
static host;
static hicnt = 1024;
static locnt = 150;
static mxcnt;
static lb = LINK_BASE;
static dc = 0;
static ps = PAUSE;
static color = -1;
static screen_w;
static screen_h;
static void (*scan)(void) = scan_host;
static void (*data_in)(char *, uint) = chan_in;
static void (*rect)(uint,uint,uint,uint,uint),(*vect)(int,int,int,char *);
static FILE *fpauto;

static union REGS regs;
static unsigned lt;
static D[2][2] = {
    {0,2},
    {3,1}
};





main(argc,argv)
int argc;
char *argv[];
{
    int i,aok = 1;
    char *s;

    printf("CSA Mandelzoom Version 2.1 for PC\n");
    printf("(C) Copyright 1988 Computer System Architects Provo, Utah\n");
    printf("Enhanced by Axel Muhr \(geekdot.com\), 2009, 2015\n");
    printf("This is a free software and you are welcome to redistribute it\n\n");

    for (i = 1; i < argc && argv[i][0] == '-'; i++)
   {
   for (s = argv[i]+1; *s != '\0'; s++)
       {
       switch (*s)
      
      {
      case 'a':

          t_request(5);               /* ask for 5 timers */
          if (t_start() != TRUE)      /* init TCHRT first thing */
          {
         printf("Insufficient heap for timers.\n");
         exit(0);
          }

          if (sscanf(s+1,"%d",&ps) == 1) s++;
          if ((ps < 0) || (ps > 9)) ps = PAUSE;
          autz = 1;
          break;
          
      case 'b':
          
          if (i >= argc) {aok = 0; break;}
          aok &= sscanf(argv[++i],"%i",&lb) == 1 && *(s+1) == '\0';
          break;
          
      case 'c':
          
          color = 1;
          break;
          
      case 'd':
          
          aok &= sscanf(s+1,"%d",&dc) == 1;
          if (aok) s++;
          aok &= (dc >= 1) && (dc <= 3);
          break;
          
      case 'e':
          
          color = 2;
          break;
          
      case 'h':

          color = 0;
          break;
          
      case 'i':

          if (i >= argc) {aok = 0; break;}
          aok &= sscanf(argv[++i],"%i",&mxcnt) == 1 && *(s+1) == '\0';
          break;
          
      case 'p':
          
          data_in = dma_in;
          dc = DMA_CHAN;
          break;
          
      case 't':
          
          host = 1;
          break;
          
      case 'v':
          
          color = 3;
          break;
          
      case 'x':
          
          verbose = 1;
          break;                    
          
      default:
          
          aok = 0;
          break;
          
      }
      
       }
   }
    
    aok &= i == argc;
    if (!aok)
   
   {
   printf("Usage: man [-b #] [-i #] [-a[#]cd#ehpt]\n");
   printf("  -a  auto-zoom, coord. from file 'man.dat', # sec. pause\n");
   printf("  -b  link adaptor base address, # is base (default 0x150)\n");
   printf("  -c  use cga graphics\n");
   printf("  -e  use ega graphics\n");
   printf("  -d  DMA channel, # can be 1-3 (default 1)\n");
   printf("  -h  use hercules graphics\n");
   printf("  -i  max. iteration count, # is iter. (default variable)\n");
   printf("  -p  use program I/O, no DMA (default use DMA)\n");
   printf("  -t  use host, no transputers (default transputers)\n");
   printf("  -v  use vga graphics\n");
   printf("  -x  print verbose messages during initialisation\n");
   exit(1);
   }
   
    if (color == -1)
   
   {
   printf("h)ercules c)ga e)ga v)ga: ");
   switch (getchar())
       {
       case 'h': 
       case 'H': color = 0; break;
       case 'E':
       case 'e': color = 2; break;
       case 'V':
       case 'v': color = 3; break;
       default:  color = 1; break;
       }
   }
   
   if (!host) {
    init_lkio(lb,dc,dc);
    t_entry(0);         /* we use timer 0 for T-Booting bench */
    boot_mandel();
    t_exit(0);
    scan = scan_tran;
   }
   
	if (autz) {
	 if ((fpauto = fopen(AUTOFILE,"r")) == NULL) {
       printf(" -- can't open file: %s\n",AUTOFILE);
       exit(1);
	 }
	} else {
		printf("\nAfter frame is displayed:\n\n");
		printf("Home - display zoom box, use arrow keys to move & size, ");
		printf("Home again to zoom\n");
		printf("Ins  - toggle between size and move zoom box\n");
		printf("PgUp - reset to outermost frame\n");
		printf("PgDn - save coord. and iter. to file 'man.dat'\n");
		printf("End  - quit\n");
		printf("\n -- Press a key to continue --\n"); 
		getch();
	}
   
    
	if (color == 0) {
		hrc_graph_mode();
		rect = hrc_rect;
		vect = hrc_vect;
		screen_w = 720; screen_h = 348;
	} else if (color == 1) {
		ibm_mode(4);
		cga_set_pal(1,1);
		rect = cga_rect;
		vect = cga_vect;
		screen_w = 320; screen_h = 200;
	} else if (color == 2) {
		ibm_mode(16);
		ega_init_pal();
		rect = ega_rect;
		vect = ega_vect;
		screen_w = 640; screen_h = 350;
	} else if (color == 3) {
		ibm_mode(18);
		ega_init_pal();
		rect = ega_rect;
		vect = vga_vect;
		screen_w = 640; screen_h = 480;
	}
    
    clear_scr();
    init_window();

    if (dc) dma_on();
    if (autz) auto_loop(); else com_loop();
    if (dc) dma_off();

    ibm_mode(3);
    if (!color) hrc_text_mode();
    if (autz) {
	   fclose(fpauto);
   
       t_set_report(NONZERO);                      /* specify report type */
       t_rname("Benchmark results");               /* report title        */
       t_name(0,"Booting Transputer(s)");          /* give each timer a name */
       t_name(1,"1st run");
       t_name(2,"2nd run");
       t_name(3,"3rd run");
       t_name(4,"4th run");
       t_report(0);              /* do report - (0) goes to CRT */
       t_stop();                 /* shut down TCHRT and free heap */
  }
  return(0);
}




#define ASPECT_R  0.75
#define MIN_SPAN  2.5
#define CENTER_R -0.75
#define CENTER_I  0.0



int esw,esh;
double center_r,center_i;
double prop_fac,scale_fac;



void com_loop(void)
{
    int esc;

    esc = FALSE;
    loop
   {
   if (!esc) (*scan)();
   while (kbhit()) getch();
   switch (get_key())
       {
       case HOME: zoom(&esc); break;
       case PGUP: init_window(); esc = FALSE; break;
       case PGDN: save_coord(); esc = TRUE; break;
       case END : return;
       default: esc = TRUE;
       }
   }
}



void auto_loop(void)
{
    int res;
    double rng;
    int run = 1;
    
    loop
	{
	   res = fscanf(fpauto," x:%lf y:%lf range:%lf iter:%d",
			   &center_r,&center_i,&rng,&mxcnt);
	   if (res == EOF) {rewind(fpauto); continue;}
	   if (res != 4) return;   /* End if anything else returned */
	   scale_fac = rng/(esw-1);
	   t_entry(run);           /* start timer */
	   (*scan)();              /* this does the work */
	   t_exit(run);            /* stop timer */
	   run++;
	   if (kbhit()) break;
	   waitsec(ps);
	   if (kbhit()) break;
	}
    do getch(); while (kbhit());
}



void scan_tran(void)
{
    register int len;
    double xrange,yrange;
	struct{
	   long com;
	   long width;
	   long height;
	   long maxcnt;
	   double lo_r;
	   double lo_i;
	   double gapx;
	   double gapy;
    } prob_st;
    
	long buf[BUFSIZE];

    xrange = scale_fac*(esw-1);
    yrange = scale_fac*(esh-1);
    prob_st.com = PRBCOM;
    prob_st.width = screen_w;
    prob_st.height = screen_h;
    prob_st.maxcnt = calc_iter(xrange);
    prob_st.lo_r = center_r - (xrange/2.0);
    prob_st.lo_i = center_i - (yrange/2.0);
    prob_st.gapx = xrange / (screen_w-1);
    prob_st.gapy = yrange / (screen_h-1);
    word_out(12L*4);
    chan_out((char *)&prob_st,12*4);

	loop
	{
		(*data_in)((char *)buf,len = (int)word_in());
		if (len == 4) break;
		(*vect)((int)buf[1],(int)buf[2],len-3*4,(char *)&buf[3]);
	}
}



void scan_host(void)
{
    register int i,pixvec;
    int x,y,maxcnt,multiple;
    double xrange,yrange;
    double lo_r;
    double lo_i;
    double gapx;
    double gapy;
    REAL cx,cy;
    unsigned char buf[MAXPIX];

    xrange = scale_fac*(esw-1);
    yrange = scale_fac*(esh-1);
    maxcnt = calc_iter(xrange);
    lo_r = center_r - (xrange/2.0);
    lo_i = center_i - (yrange/2.0);
    gapx = xrange / (screen_w-1);
    gapy = yrange / (screen_h-1);

    multiple = screen_w/MAXPIX*MAXPIX;
    for (y = 0; y < screen_h; y++) {
	 cy = y*gapy+lo_i;
	 for (x = 0; x < screen_w; x+=MAXPIX) {
       pixvec = (x < multiple) ? MAXPIX : screen_w-multiple;
       for (i = 0; i < pixvec; i++) {
		cx = (x+i)*gapx+lo_r;
		buf[i] = iterate(cx,cy,maxcnt);
       }
	 (*vect)(x,y,pixvec,buf);
	 if (kbhit()) return;
	 }
	}
}



int iterate(cx,cy,maxcnt)
REAL cx,cy;
int maxcnt;
{
	int cnt;
	REAL zx,zy,zx2,zy2,tmp,four;
	
	four = 4.0;
	zx = zy = zx2 = zy2 = 0.0;
	cnt = 0;
	do {
			tmp = zx2-zy2+cx;
			zy = zx*zy*2.0+cy;
			zx = tmp;
			zx2 = zx*zx;
			zy2 = zy*zy;
			cnt++;
	} while (cnt < maxcnt && zx2+zy2 < four);
	if (cnt != maxcnt) return(cnt);
	return(0);
}



int calc_iter(r)
double r;
{
    if (mxcnt) return(mxcnt);
    if (r <= LOR) return(hicnt);
    return((int)((hicnt-locnt)/(f(LOR)-f(HIR))*(f(r)-f(HIR))+locnt+0.5));
}



void init_window(void)
{
    prop_fac = (ASPECT_R/((double)screen_h/(double)screen_w));
    esw = screen_w;
    esh = screen_h*prop_fac;
    if (esh <= esw) scale_fac = MIN_SPAN/(esh-1);
    else scale_fac = MIN_SPAN/(esw-1);
    center_r = CENTER_R;
    center_i = CENTER_I;
}





void save_coord(void)
{
    double xrange;

    if (!autz) {
   if ((fpauto = fopen(AUTOFILE,"a")) == NULL) return;
   autz = TRUE;
    }
    xrange = scale_fac*(esw-1);
    fprintf(fpauto,"x:%17.14f y:%17.14f range:%e iter:%d\n",
       center_r,center_i,xrange,calc_iter(xrange));
}



void zoom(esc)
int *esc;
{
    int bx,by,lx,ly,sw,sh;
    double xfac,yfac;

    region(&bx,&by,&lx,&ly,esc);
    if (*esc) return;
    by = by*prop_fac;
    ly = ly*prop_fac;
    sw = ((bx < lx) ? lx-bx : bx-lx) + 1;
    sh = ((by < ly) ? ly-by : by-ly) + 1;
    center_r += (((bx+lx) - (esw-1))/2.0)*scale_fac;
    center_i += (((by+ly) - (esh-1))/2.0)*scale_fac;
    xfac = (double)(sw-1)/(double)(esw-1);
    yfac = (double)(sh-1)/(double)(esh-1);
    scale_fac *= (xfac > yfac) ? xfac : yfac;
}



void region(bx,by,lx,ly,esc)
int *bx,*by,*lx,*ly,*esc;
{
    int c,tmp,dx,dy,jmp,boxw,boxh,insert;

    insert = 0;
    jmp = 1;
    boxw = screen_w/10;
    boxh = screen_h/10;
    *esc = FALSE;
    *bx = ((screen_w - boxw)/2);
    *by = ((screen_h - boxh)/2);
    *lx = *bx + (boxw-1);
    *ly = *by + (boxh-1);
    draw_box(*bx,*by,*lx,*ly);
    do {
   c = get_key()+insert;
   if (counts() < 5) jmp++; else jmp = 1;
   draw_box(*bx,*by,*lx,*ly);
   switch (c) {
   case LARW:
       tmp = (*bx >= jmp) ? jmp : *bx;
       *bx -= tmp; *lx -= tmp;
       break;
   case RARW:
       tmp = (*lx+jmp < screen_w) ? jmp : (screen_w-1) - *lx;
       *bx += tmp; *lx += tmp;
       break;
   case UARW:
       tmp = (*ly+jmp < screen_h) ? jmp : (screen_h-1) - *ly;
       *by += tmp; *ly += tmp;
       break;
   case DARW:
       tmp = (*by >= jmp) ? jmp : *by;
       *by -= tmp; *ly -= tmp;
       break;
   case LARW+INSM:
   case DARW+INSM:
       tmp = *lx-*bx;
       dx = (tmp > jmp) ? tmp-jmp : 1;
       dy = (double)screen_h / (double)screen_w * (double)dx;
       *bx += (tmp>>1)-(dx>>1); *lx = *bx+dx;
       *by += ((*ly-*by)>>1)-(dy>>1); *ly = *by+dy;
       break;
   case RARW+INSM:
   case UARW+INSM:
       tmp = *lx-*bx;
       dx = (tmp+jmp < screen_w) ? tmp+jmp : screen_w-1;
       dy = (double)screen_h / (double)screen_w * (double)dx;
       *bx += (tmp>>1)-(dx>>1); *lx = *bx+dx;
       *by += ((*ly-*by)>>1)-(dy>>1); *ly = *by+dy;
       if (*bx < 0) {
      *lx -= *bx; *bx = 0;
       } else if (*lx >= screen_w) {
      *bx -= *lx-(screen_w-1); *lx = screen_w-1;
       }
       if (*by < 0) {
      *ly -= *by; *by = 0;
       } else if (*ly >= screen_h) {
      *by -= *ly-(screen_h-1); *ly = screen_h-1;
       }
       break;
   case INS:
   case INS+INSM:
       insert ^= INSM;
       break;
   case ESC:
   case ESC+INSM:
       *esc = TRUE;
       break;
   }
   draw_box(*bx,*by,*lx,*ly);
    } while ((c & (INSM-1)) != HOME && !*esc);
    draw_box(*bx,*by,*lx,*ly);
}




int counts(void)
{
    unsigned cnts;

    regs.x.ax = 0;
    int86(0x1A,&regs,&regs);
    cnts = regs.x.dx - lt;
    lt = regs.x.dx;
    return(cnts);
}



int get_key(void)
{
    int c;

    if ((c = getch())) return(c);
    return(0x100 + getch());
}



void waitsec(s)
int s;
{
    long start,now;

    time(&start);
    do time(&now); while ((now-start) < s);
}





#define INVTC 0x8f

void draw_box(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
    int x,y,w,h;

    if (x1 < x2) {x = x1; w = x2-x1+1;}
    else {x = x2; w = x1-x2+1;}
    if (y1 < y2) {y = y1; h = y2-y1+1;}
    else {y = y2; h = y1-y2+1;}
    h_line(x,y1,w,INVTC);
    h_line(x,y2,w,INVTC);
    v_line(x1,y,h,INVTC);
    v_line(x2,y,h,INVTC);
}



void ibm_mode(m)
int m;
{
    regs.h.ah = 0;
    regs.h.al = m;
    int86(0x10,&regs,&regs);
}



void hrc_graph_mode(void)
{
    hrc_rect(0,0,348,720,0);
    outp(0x3bf,3);
    outp(0x3b8,0x0a);
    outp(0x3b4,0); outp(0x3b5,53);
    outp(0x3b4,1); outp(0x3b5,45);
    outp(0x3b4,2); outp(0x3b5,46);
    outp(0x3b4,3); outp(0x3b5,6);
    outp(0x3b4,4); outp(0x3b5,91);
    outp(0x3b4,5); outp(0x3b5,2);
    outp(0x3b4,6); outp(0x3b5,87);
    outp(0x3b4,7); outp(0x3b5,87);
    outp(0x3b4,8); outp(0x3b5,2);
    outp(0x3b4,9); outp(0x3b5,3);
    outp(0x3b4,10); outp(0x3b5,0);
    outp(0x3b4,11); outp(0x3b5,0);
    outp(0x3b4,12); outp(0x3b5,0);
    outp(0x3b4,13); outp(0x3b5,0);
    outp(0x3b4,14); outp(0x3b5,0);
    outp(0x3b4,15); outp(0x3b5,0);
}



void hrc_text_mode(void)
{
    outp(0x3bf,0);
    outp(0x3b8,0x28);
    outp(0x3b4,0); outp(0x3b5,95);
    outp(0x3b4,1); outp(0x3b5,80);
    outp(0x3b4,2); outp(0x3b5,82);
    outp(0x3b4,3); outp(0x3b5,11);
    outp(0x3b4,4); outp(0x3b5,25);
    outp(0x3b4,5); outp(0x3b5,3);
    outp(0x3b4,6); outp(0x3b5,25);
    outp(0x3b4,7); outp(0x3b5,25);
    outp(0x3b4,8); outp(0x3b5,2);
    outp(0x3b4,9); outp(0x3b5,13);
    outp(0x3b4,10); outp(0x3b5,13);
    outp(0x3b4,11); outp(0x3b5,13);
    outp(0x3b4,12); outp(0x3b5,0);
    outp(0x3b4,13); outp(0x3b5,0);
    outp(0x3b4,14); outp(0x3b5,0);
    outp(0x3b4,15); outp(0x3b5,0);
}



void ega_init_pal(void)
{
    ega_set_pal( 0,000);
    ega_set_pal( 1,004);
    ega_set_pal( 2,044); /* red */
    ega_set_pal( 3,064);
    ega_set_pal( 4,046);
    ega_set_pal( 5,066); /* yellow */
    ega_set_pal( 6,026);
    ega_set_pal( 7,022); /* green */
    ega_set_pal( 8,023);
    ega_set_pal( 9,033); /* cyan */
    ega_set_pal(10,013);
    ega_set_pal(11,031);
    ega_set_pal(12,011); /* blue */
    ega_set_pal(13,015);
    ega_set_pal(14,075); /* magenta */
    ega_set_pal(15,077);
}



void ega_set_pal(idx,color)
int idx,color;
{
    regs.h.ah = 16;
    regs.h.al = 0;
    regs.h.bl = idx;
    regs.h.bh = color;
    int86(0x10,&regs,&regs);
}



void cga_set_pal(id,val)
int id,val;
{
    regs.h.ah = 11;
    regs.h.bh = id;
    regs.h.bl = val;
    int86(0x10,&regs,&regs);
}



void h_line(x,y,w,c)
int x,y,w,c;
{
    int r;

    r = (screen_h-1)-y;
    (*rect)(r,x,r+1,x+w,c);
}



void v_line(x,y,h,c)
int x,y,h,c;
{
    int r;

    r = (screen_h-1)-y;
    (*rect)(r-(h-1),x,r+1,x+1,c);
}



void clear_scr(void)
{
    (*rect)(0,0,screen_h,screen_w,0);
}



#define MAXROWS 350
#define MAXCOLS 640
#define COLBYTES MAXCOLS/8
#define GRAFOUT(index,value) {outp(0x3Ce,index); outp(0x3Cf,value);}



/*
ega_vect(x,y,w,line)
int x,y,w;
char *line;
{
    register col,mask;
    int r1 = (MAXROWS-1)-y;
    int c2 = x+w;
    char far *addr = (char far*) (0xa0000000L + COLBYTES * r1 + (x >> 3));

    mask = 0x80 >> (x & 7);
    GRAFOUT(3,0);
    GRAFOUT(5,2);
    for (col = x; col < c2; col++) {
   GRAFOUT(8,mask);
   r1 = *addr;
   *addr = *line++;
   mask >>= 1;
   if (!mask) {addr++; mask = 0x80;}
    }
    GRAFOUT(8,0xff);
    GRAFOUT(5,0);
}
*/


/*
cga_vect(x,y,w,line)
int x,y,w;
char *line;
{
    register col,mask;
    int r1 = 199-y;
    int c2 = x+w;
    char far *addr =
   (char far*) (0xb8000000L
   + (0x2000 * (r1 & 1)) + (80 * (r1 >> 1))
   + (x >> 2));

    mask = 0xc0 >> ((x & 3)*2);
    for (col = x; col < c2; col++) {
   *addr = *addr & ~mask | ((*line++ & 3) * 0x55) & mask;
   mask >>= 2;
   if (!mask) {addr++; mask = 0xc0;}
    }
}
*/


/*
hrc_vect(x,y,w,line)
int x,y,w;
char *line;
{
    register col,mask;
    int r1 = 347-y;
    int c2 = x+w;
    int (*dither)[2] = D+(r1 & 1);
    char far *addr =
   (char far*) (0xb0000000L
   + (0x2000 * (r1 & 3)) + (90 * (r1 >> 2))
   + (x >> 3));

    mask = 0x80 >> (x & 7);
    for (col = x; col < c2; col++) {
   if ((*line++ % 5) > (*dither)[col & 1]) *addr |= mask;
   else *addr &= ~mask;
   mask >>= 1;
   if (!mask) {addr++; mask = 0x80;}
    }
}
*/



void ega_rect(r1,c1,r2,c2,color)
unsigned int r1,c1,r2,c2,color;
{
    char far *addr = (char far*) (0xa0000000L + COLBYTES * r1 + (c1 >> 3));
    int numrows = r2 - r1;
    int numcols = (c2 >> 3) - (c1 >> 3) - 1;
    char lmask  =   0xff >> (c1 & 7);
    char rmask  = ~(0xff >> (c2 & 7));
    register row,col;

    if (color & 0x80) GRAFOUT(3,0x18)
    else GRAFOUT(3,0)
    if (numcols < 0) {
   lmask &= rmask;
   rmask = numcols = 0;
    }
    GRAFOUT(0,color);
    GRAFOUT(1,0x0f);
    for (row = 0; row < numrows; row++) {
   GRAFOUT(8,lmask);
   *(addr++) &= 1;
   GRAFOUT(8,0xff);
   for (col = 0; col < numcols; col++) *(addr++) &= 1;
   GRAFOUT(8,rmask);
   *(addr++) &= 1;
   addr += COLBYTES - 2 - numcols;
    }
    GRAFOUT(0,0);
    GRAFOUT(1,0);
    GRAFOUT(8,0xff);
}



void cga_rect(r1,c1,r2,c2,color)
unsigned int r1,c1,r2,c2,color;
{
    char far *base = (char far*) (0xb8000000L + (c1 >> 2));
    char far *addr;
    int numcols = (c2 >> 2) - (c1 >> 2) - 1;
    char lmask  =   0xff >> ((c1 & 3)*2);
    char rmask  = ~(0xff >> ((c2 & 3)*2));
    int c;
    register row,col;

    if (numcols < 0) {
   lmask &= rmask;
   rmask = numcols = 0;
    }
    if (color & 0x80) {
   for (row = r1; row < r2; row++) {
       addr = base + (0x2000 * (row & 1)) + (80 * (row >> 1));
       *(addr++) ^= lmask;
       for (col = 0; col < numcols; col++) *(addr++) ^= 0xff;
       *(addr++) ^= rmask;
   }
    } else {
   c = ((color & 3) * 0x55);
   for (row = r1; row < r2; row++) {
       addr = base + (0x2000 * (row & 1)) + (80 * (row >> 1));
       *addr = *addr & ~lmask | c & lmask; addr++;
       for (col = 0; col < numcols; col++) *(addr++) = c;
       *addr = *addr & ~rmask | c & rmask;
   }
    }
}



void hrc_rect(r1,c1,r2,c2,color)
unsigned int r1,c1,r2,c2,color;
{
    char far *base = (char far*) (0xb0000000L + (c1 >> 3));
    char far *addr;
    int numcols = (c2 >> 3) - (c1 >> 3) - 1;
    char lmask  =   0xff >> (c1 & 7);
    char rmask  = ~(0xff >> (c2 & 7));
    int c;
    register row,col;

    if (numcols < 0) {
   lmask &= rmask;
   rmask = numcols = 0;
    }
    if (color & 0x80) {
   for (row = r1; row < r2; row++) {
       addr = base + (0x2000 * (row & 3)) + (90 * (row >> 2));
       *(addr++) ^= lmask;
       for (col = 0; col < numcols; col++) *(addr++) ^= 0xff;
       *(addr++) ^= rmask;
   }
    } else {
   c = ((color & 1) * 0xff);
   for (row = r1; row < r2; row++) {
       addr = base + (0x2000 * (row & 3)) + (90 * (row >> 2));
       *addr = *addr & ~lmask | c & lmask; addr++;
       for (col = 0; col < numcols; col++) *(addr++) = c;
       *addr = *addr & ~rmask | c & rmask;
   }
    }
}





#include "sreset.arr"


#include "flboot.arr"


#include "flload.arr"


#include "ident.arr"


#include "mandel.arr"


#include "smallman.arr"




void boot_mandel(void)
{   int fxp, only_2k, nnodes;

    rst_adpt(TRUE);
    if (verbose) printf("Resetting Transputers...");
    if (!load_buf(sreset,sizeof(sreset))) exit(1);
    if (verbose) printf("Booting...");
    if (!load_buf(flboot,sizeof(flboot))) exit(1);
    if (verbose) printf("Loading...");      
    if (!load_buf(flload,sizeof(flload))) exit(1);
    if (verbose) printf("ID'ing...\n");
    if (!load_buf(ident,sizeof(ident))) exit(1);
    if (!tbyte_out(0))
   {
	   printf(" -- timeout sending execute\n");
	   exit(1);
   }
    if (!tbyte_out(0))
   {
	   printf(" -- timeout sending id\n");
	   exit(1);
   }
    if (verbose) printf("Done. Getting data for \"only_2k\",");        
    only_2k = (int)word_in();
    if (verbose) printf("\"num_nodes\",");
    nnodes  = (int)word_in();
    if (verbose) printf("\"fpx\",");
    fxp     = (int)word_in();
    printf("\nnodes found: %d",nnodes);
    if (only_2k) printf("\n%d nodes have only 2K RAM");
    printf("\nusing %s-point arith.\n",
        fxp ? "fixed" : "floating");
    if (only_2k)
   {
   if (verbose) printf("Sending 2k mandel-code\n");
   if (!load_buf(smallman,sizeof(smallman))) exit(1);
   if (!tbyte_out(0))
       {
       printf(" -- timeout sending execute\n");
       exit(1);
       }
   }
    else
   {
   if (verbose) printf("Sending mandel-code\n");	   
   if (!load_buf(mandel,sizeof(mandel))) exit(1);
   if (!tbyte_out(0))
       {
       printf(" -- timeout sending execute\n");
       exit(1);
       }
   word_in();    /*throw away nnodes*/
   word_in();    /*throw away fixed point*/
   }
}



/* return TRUE if loaded ok, FALSE if error. */

int load_buf(buf,bcnt)
char *buf;
int bcnt;
{
    int wok,len;

    do {
   len = (bcnt > 255) ? 255 : bcnt;
   bcnt -= len;
   if ((wok = tbyte_out(len)))
       while (len-- && (wok = tbyte_out(*buf++)));
    } while (bcnt && wok);
    if (!wok) printf(" -- timeout loading network\n");
    return(wok);
}


