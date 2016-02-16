
/*
 *  Function declarations for the Link I/O interface to the link adaptor.
 *  Copyright (C) Computer System Architects, 1988
 */

int  err_flag(void);
int  busy_in(void);
int  busy_out(int);
int  byte_in(void);
void byte_out(int);
int  tbyte_in(void);
int  tbyte_out(int);
long word_in(void);
void word_out(long);
void chan_in(char *,unsigned int);
void chan_out(char *,unsigned int);
void dma_in(char *,unsigned int);
void dma_out(char *,unsigned int);
void dma_on(void);
void dma_off(void);
void rst_adpt(int);
int  init_lkio(int,int,int);
