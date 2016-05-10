/* #include <dos.h>
#include <conio.h> */

#include <bios.h>

#define IRQ0 0x8
#define PIT0 0x40
#define PIT1 0x41
#define PIT2 0x42
#define PITMODE 0x43
#define PITCONST 1193180L
#define PIT0DEF 18.2067597
#define KBCTRL 0x61
#define NEW8H 1

char           *get_cmostime(void);
void interrupt  new8h(void);
void            init8h(unsigned int);
void            quit8h(void);
unsigned long   time8h(unsigned long, unsigned long);
void            delay8h(unsigned int);
void            sound8h(int, int);

extern volatile unsigned long int ticks_8h;

static float    tick_per_ms = 0.0182068;
static float    ms_per_tick = 54.9246551;
static float    freq8h = 18.2067597;
static unsigned char flag8h = 0;
static void     interrupt(*old8h) (void);

volatile int    counter_8h;
volatile int    counter_reset;
volatile unsigned long int ticks_8h;

void            init8h(unsigned int Hz)
{
    unsigned int    pit0_set,
                    pit0_value;

    if (flag8h != NEW8H) {
        disable();
        old8h = getvect(IRQ0);
        setvect(IRQ0, new8h);
        outportb(PITMODE, 0x36);
        pit0_value = PITCONST / Hz;
        pit0_set = (pit0_value & 0x00ff);
        outportb(PIT0, pit0_set);
        pit0_set = (pit0_value >> 8);
        outportb(PIT0, pit0_set);
        enable();

        flag8h = NEW8H;
        freq8h = Hz;
        counter_8h = 0;
        counter_reset = freq8h / PIT0DEF;
        tick_per_ms = freq8h / 1000;
        ms_per_tick = 1000 / freq8h;
    }
}

void            quit8h(void)
{
    unsigned int    pit0_set,
                    pit0_value;
    long            tick;
    char           *cmostime;

    if (flag8h == NEW8H) {
        disable();
        outportb(PITMODE, 0x36);
        outportb(PIT0, 0x00);
        outportb(PIT0, 0x00);
        setvect(IRQ0, old8h);
        enable();

        cmostime = get_cmostime();
        tick = PIT0DEF *
            (
             (((float) *cmostime) * 3600) +
             (((float) *(cmostime + 1)) * 60) +
             (((float) *(cmostime + 2)))
            );
        biostime(1, tick);

        flag8h = 0;
        freq8h = PIT0DEF;
        counter_reset = freq8h / PIT0DEF;
        tick_per_ms = freq8h / 1000;
        ms_per_tick = 1000 / freq8h;
    }
}

void            interrupt
                new8h(void)
{
    ticks_8h++;
    counter_8h++;

    if (counter_8h == counter_reset) {
        counter_8h = 0;
        old8h();
    } else {
        disable();
        outportb(0x20, 0x20);
    }
}

unsigned long   time8h(unsigned long start, unsigned long stop)
{
    unsigned long   duration,
                    millisec;

    if (stop < start)
        return 0;
    else {
        duration = stop - start;
        millisec = duration * ms_per_tick;
        return millisec;
    }
}

void            delay8h(unsigned int delayms)
{
    unsigned long int delaybegin = 0;
    unsigned long int delayend = 0;
    unsigned int    delaytick;

    delaytick = delayms * tick_per_ms;

    if (flag8h == NEW8H)
        delaybegin = ticks_8h;
    else
        biostime(0, (long) delaybegin);

    do {
        if (flag8h == NEW8H)
            delayend = ticks_8h;
        else
            biostime(0, (long) delayend);
    } while ((delayend - delaybegin) < delaytick);
}

void            sound8h(int freq, int duration)
{
    int             byte;
    unsigned int    freq1;

    freq1 = PITCONST / freq;
    outportb(PITMODE, 0xb6);
    byte = (freq1 & 0xff);
    outportb(PIT2, byte);
    byte = (freq1 >> 8);
    outportb(PIT2, byte);
    byte = inportb(KBCTRL);
    outportb(KBCTRL, (byte | 3));

    delay8h(duration);
    outportb(KBCTRL, (byte & 0xfc));
}

char           *get_cmostime(void)
{
    union REGS      inreg;
    union REGS      outreg;
    char           *buff;
    static char     buffer[6];
    char            ch;

    buff = buffer;
    inreg.h.ah = 0x02;
    int86(0x1a, &inreg, &outreg);

    ch = outreg.h.ch;
    buffer[0] = (char) ((int) (ch & 0x0f) + (int) ((ch >> 4) & 0x0f) * 10);
    ch = outreg.h.cl;
    buffer[1] = (char) ((int) (ch & 0x0f) + (int) ((ch >> 4) & 0x0f) * 10);
    ch = outreg.h.dh;
    buffer[2] = (char) ((int) (ch & 0x0f) + (int) ((ch >> 4) & 0x0f) * 10);
    buffer[3] = outreg.h.dl;
    buffer[4] = (char) (outreg.x.cflag & 0x0001);
    buffer[5] = 0x00;

    return (buff);
}
