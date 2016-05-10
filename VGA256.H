/*

	The VESA VBE routines for 256 colors at higher resolutions
	Part of CSA Mandelbrot "V3" packagage
	Axel Muhr, 2016, www.geekdot.com

*/

#include <math.h>

/*---------------------- VESA Macro and type definitions -----------------------*/
/* SuperVGA information block */
struct
{
    char    VESASignature[4];       /* 'VESA' 4 byte signature          */
    short   VESAVersion;            /* VBE version number               */
    char    far *OEMStringPtr;      /* Pointer to OEM string            */
    long    Capabilities;           /* Capabilities of video card       */
    unsigned far *VideoModePtr;     /* Pointer to supported modes       */
    short   TotalMemory;            /* Number of 64kb memory blocks     */
    char    reserved[236];          /* Pad to 256 byte block size       */
} VbeInfoBlock;

/* SuperVGA mode information block */
struct
{
    unsigned short ModeAttributes;      /* Mode attributes                 */
    unsigned char  WinAAttributes;      /* Window A attributes             */
    unsigned char  WinBAttributes;      /* Window B attributes             */
    unsigned short WinGranularity;      /* Window granularity in k         */
    unsigned short WinSize;             /* Window size in k                */
    unsigned short WinASegment;         /* Window A segment                */
    unsigned short WinBSegment;         /* Window B segment                */
    void (far *WinFuncPtr)(void);       /* Pointer to window function      */
    unsigned short BytesPerScanLine;    /* Bytes per scanline              */
    unsigned short XResolution;         /* Horizontal resolution           */
    unsigned short YResolution;         /* Vertical resolution             */
    unsigned char  XCharSize;           /* Character cell width            */
    unsigned char  YCharSize;           /* Character cell height           */
    unsigned char  NumberOfPlanes;      /* Number of memory planes         */
    unsigned char  BitsPerPixel;        /* Bits per pixel                  */
    unsigned char  NumberOfBanks;       /* Number of CGA style banks       */
    unsigned char  MemoryModel;         /* Memory model type               */
    unsigned char  BankSize;            /* Size of CGA style banks         */
    unsigned char  NumberOfImagePages;  /* Number of images pages          */
    unsigned char  res1;                /* Reserved                        */
    unsigned char  RedMaskSize;         /* Size of direct color red mask   */
    unsigned char  RedFieldPosition;    /* Bit posn of lsb of red mask     */
    unsigned char  GreenMaskSize;       /* Size of direct color green mask */
    unsigned char  GreenFieldPosition;  /* Bit posn of lsb of green mask   */
    unsigned char  BlueMaskSize;        /* Size of direct color blue mask  */
    unsigned char  BlueFieldPosition;   /* Bit posn of lsb of blue mask    */
    unsigned char  RsvdMaskSize;        /* Size of direct color res mask   */
    unsigned char  RsvdFieldPosition;   /* Bit posn of lsb of res mask     */
    unsigned char  DirectColorModeInfo; /* Direct color mode attributes    */
    unsigned char  res2[216];           /* Pad to 256 byte block size      */
} ModeInfoBlock;
typedef enum
{
    memPL       = 3,                /* Planar memory model              */
    memPK       = 4,                /* Packed pixel memory model        */
    memRGB      = 6,                /* Direct color RGB memory model    */
    memYUV      = 7,                /* Direct color YUV memory model    */
} memModels;

typedef struct
{
    unsigned int red;
    unsigned int green;
    unsigned int blue;
} vgaColor;

/*--------------------------- VESA Global Variables ----------------------------*/
char mystr[256];
char *get_str();
int     xres,yres;                  /* Resolution of video mode used    */
int     bytesperline;               /* Logical CRT scanline length      */
int     curBank;                    /* Current read/write bank          */
unsigned int bankShift;             /* Bank granularity adjust factor   */
int     oldMode;                    /* Old video mode number            */
char    far *screenPtr;             /* Pointer to start of video memory */
void    (far *bankSwitch)(void);    /* Direct bank switching function   */

#define PAL_WRITE_ADDR (0x3c8)      // palette write address
#define PAL_READ_ADDR  (0x3c7)      // palette write address
#define PAL_DATA       (0x3c9)      // palette data register


// VBE functions (256 colors)

void putPixel(int,int,int);

extern int screen_w;
extern int screen_h;

/*

		VESA graphics routines

*/

void vesa_vect(int x, int y, int w, char *line) {

int i;
	for (i=0; i < w; i++) {
 	 putPixel(x+i, y, line[i]);
	}
}

/*------------------------ VBE Interface Functions ------------------------*/
/* Get SuperVGA information, returning true if VBE found */
int getVbeInfo(void)
{
    union REGS in,out;
    struct SREGS segs;
    char far *VbeInfo = (char far *)&VbeInfoBlock;
    in.x.ax = 0x4F00;
    in.x.di = FP_OFF(VbeInfo);
    segs.es = FP_SEG(VbeInfo);
    int86x(0x10, &in, &out, &segs);
    return (out.x.ax == 0x4F);
}

/* Get video mode information given a VBE mode number. We return 0 if the mode
 * is not available, or if it is not a 256 color packed pixel mode. */
int getModeInfo(int mode)
{
    union REGS in,out;
    struct SREGS segs;
    char far *modeInfo = (char far *)&ModeInfoBlock;
    if (mode < 0x100) return 0;     /* Ignore non-VBE modes             */
    in.x.ax = 0x4F01;
    in.x.cx = mode;
    in.x.di = FP_OFF(modeInfo);
    segs.es = FP_SEG(modeInfo);
    int86x(0x10, &in, &out, &segs);
    if (out.x.ax != 0x4F) return 0;
    if ((ModeInfoBlock.ModeAttributes & 0x1)
            && ModeInfoBlock.MemoryModel == memPK
            && ModeInfoBlock.BitsPerPixel == 8
            && ModeInfoBlock.NumberOfPlanes == 1)
        return 1;
    return 0;
}

/* Set a VBE video mode */
void setVBEMode(int mode)
{
    union REGS in,out;
    in.x.ax = 0x4F02; in.x.bx = mode;
    int86(0x10,&in,&out);
}

/* Return the current VBE video mode */
int getVBEMode(void)
{
    union REGS in,out;
    in.x.ax = 0x4F03;
    int86(0x10,&in,&out);
    return out.x.bx;
}

/* Set new read/write bank. Set both Window A and Window B, as many VBE's have
 * these set as separately available read and write windows. We also use a 
 * simple (but very effective) optimization of checking if the requested bank 
 * is currently active. */
void setBank(int bank)
{
    union REGS  in,out;
    if (bank == curBank) return;    /* Bank is already active           */
    curBank = bank;                 /* Save current bank number         */
    bank <<= bankShift;             /* Adjust to window granularity     */
#ifdef  DIRECT_BANKING
    setbxdx(0,bank);
    bankSwitch();
    setbxdx(1,bank);
    bankSwitch();
#else
    in.x.ax = 0x4F05; in.x.bx = 0;  in.x.dx = bank;
    int86(0x10, &in, &out);
    in.x.ax = 0x4F05; in.x.bx = 1;  in.x.dx = bank;
    int86(0x10, &in, &out);
#endif
}

/*-------------------------- Application Functions ------------------------*/
void vgaSetPalette( int start,  int count, vgaColor *p)
{
    int i;

    if (start < 0 || (start + count - 1) > 255)
    {
        return;
    }

    while(!(inp(0x3da) & 0x08));    // wait vertical retrace

    outp(PAL_WRITE_ADDR, start);
    for (i = 0; i < count; i++)
    {
        outp(PAL_DATA, p->red);
        outp(PAL_DATA, p->green);
        outp(PAL_DATA, p->blue);
        p++;
    }
}

void setPalette332(void)
{
    unsigned int r, g, b, c;
    vgaColor p[256];


     c = 0;
    for (r = 0; r <= 64; r += 9)
    {
        for (g = 0; g <= 64; g += 13)
        {
            for (b = 0; b < 64; b += 21)
            {
                p[c].red = r;
                p[c].green = g;
                p[c].blue = b;
                c++;
            }
        }
    }
    
		p[255].red = 63;
		p[255].green = 63;
		p[255].blue = 63; 
		
    vgaSetPalette(0, 256, p);
}

/* Plot a pixel at location (x,y) in specified color (8 bit modes only) */
void putPixel(int x,int y,int color)
{
    long addr = (long)y * bytesperline + x;
    setBank((int)(addr >> 16));
    *(screenPtr + (addr & 0xFFFF)) = (char)color;
}

/* Invert a pixel at given location - this is needed for rubbebanding */
void invPixel(int x,int y)
{
    long addr = (long)y * bytesperline + x;
    setBank((int)(addr >> 16));
    *(screenPtr + (addr & 0xFFFF)) ^= 0xFF;
}


/* Return NEAR pointer to FAR string pointer*/
char *get_str(char far *p)
{
    int i;
    char *q=mystr;
    for(i=0;i<255;i++)
    {
       if(*p) *q++ = *p++;
       else break;
    }
    *q = '\0';
    return(mystr);
}
/* Display a list of available resolutions. Be careful with calls to function
 * 00h to get SuperVGA mode information. Many VBE's build the list of video
 * modes directly in this information block, so if you are using a common 
 * buffer (which we aren't here, but in protected mode you will), then you will
 * need to make a local copy of this list of available modes. */
 
void availableModes(void)
{
    unsigned far    *p;
    if (!getVbeInfo())
    {
        printf("No VESA VBE detected\n");
        exit(1);
    }
    printf("VESA VBE Version %d.%d detected (%s)\n\n",
        VbeInfoBlock.VESAVersion >> 8, VbeInfoBlock.VESAVersion & 0xF,
        get_str(VbeInfoBlock.OEMStringPtr));
    printf("Available 256 color video modes:\n");
    for (p = VbeInfoBlock.VideoModePtr; *p !=(unsigned)-1; p++)
    {
        if (getModeInfo(*p))
        {
            printf("0x%04X : %4d by %4d (%dbpp)\n",
                *p, ModeInfoBlock.XResolution, ModeInfoBlock.YResolution,
                ModeInfoBlock.BitsPerPixel);
        }
    }
    exit(1);
}
/* Initialize the specified video mode. Notice how we determine a shift factor
 * for adjusting the Window granularity for bank switching. This is much faster
 * than doing it with a multiply (especially with direct banking enabled). */
 
void initGraphics(unsigned int mode) //(unsigned int x, unsigned int y)
{
    unsigned far    *p;
    if (!getVbeInfo())
    {
        printf("No VESA VBE detected\n");
        exit(1);
    }
    for (p = VbeInfoBlock.VideoModePtr; *p != (unsigned)-1; p++)
    {
        //if (getModeInfo(*p) && ModeInfoBlock.XResolution == x && ModeInfoBlock.YResolution == y)
        if (getModeInfo(*p) && *p == mode)
        {
           //xres = x;   yres = y;
           screen_w = ModeInfoBlock.XResolution; 
           screen_h = ModeInfoBlock.YResolution;
           bytesperline = ModeInfoBlock.BytesPerScanLine;
           bankShift = 0;
           while ((unsigned)(64 >> bankShift) != ModeInfoBlock.WinGranularity)
               bankShift++;
           bankSwitch = ModeInfoBlock.WinFuncPtr;
           curBank = -1;
           screenPtr = (char far *)( ((long)0xA000)<<16 | 0);
           oldMode = getVBEMode();
           setVBEMode(*p);
           return;
        }
    }
    printf("Valid video mode not found\n");
    exit(1);
}