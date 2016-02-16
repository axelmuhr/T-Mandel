/******************************* MLIBP.C ************************************
*  (C) Copyright 1987-1993  Computer System Architects, Provo UT.           *
*  This  program is the property of Logical System and is provided with     *
*  their permission. This is only an example of a transputer/PC program for *
*  use  with CSA's Transputer Education Kit and other transputer products.  *
*  You may freely distribute copies or modifiy the program as a whole or in *
*  part, provided you insert in each copy appropriate copyright notices and *
*  disclaimer of warranty and send to CSA a copy of any modifications which *
*  you plan to distribute.						    *
*  This program is provided as is without warranty of any kind. CSA is not  *
*  responsible for any damages arising out of the use of this program.      *

****************************************************************************/
/****************************************************************************
* This program MLIB.C provide functions for managing processes, channel and *
* memory.                                                                   *
* This program should be compiled using Logical System's C 		    *
****************************************************************************/
/* compile: tcx file -cp0rv */

#include        "c:\trans\lsc\89\include\conc.h"
#include        "c:\trans\lsc\89\include\string.h"
#include        "c:\trans\lsc\89\include\stdlib.h"

/* This function takes as array of channel pointer and return an index of
  the channel  that is ready for communication . */
int
ProcPriAltList(clist, pchan)
	Channel	**clist;
	int	pchan;
	{
	int	reserve;
	int	idx;
	int	cnt;
	Channel	**next;

	#pragma asm
	.VAL	pchan,	?ws+2
	.VAL	clist,	?ws+1
	.VAL	next,	?ws-1
	.VAL	cnt,	?ws-2
	.VAL	idx,	?ws-3
	LDL	clist			;next = clist
	STL	next
	ALT
L2
	LDL	next			;while (*next)
	LDNL	0
	CJ	@L3
	LDL	next			;load *next++
	LDL	next
	LDNLP	1
	STL	next
	LDNL	0
	LDC	1			;load 1
	ENBC				;enable channel
	J	@L2
L3
	LDL	next			;cnt = next-clist
	LDL	clist
	SUB
	WCNT
	STL	cnt
	LDL	pchan			;next = clist+pchan
	LDL	clist
	WSUB
	STL	next
	ALTWT
L4
	LDL	cnt			;while (cnt)
	CJ	@L5
	LDL	next			;load *next
	LDNL	0
	LDC	1			;load 1
	LDC	0			;load 0
	DISC				;disable channel
	CJ	@L6
	LDL	next			;idx = selected alternative
	LDL	clist
	SUB
	WCNT
	STL	idx
L6
	LDL	cnt			;cnt--
	ADC	-1
	STL	cnt
	LDL	next			;next++
	LDNLP	1
	STL	next
	LDL	next			;if (*next == 0)
	LDNL	0
	EQC	0
	CJ	@L4
	LDL	clist			;next = clist
	STL	next
	J	@L4
L5
	ALTEND
	LDL	idx			;return(idx)
	#pragma endasm
	}

/* Setup process structure */
PDes
PSetupA(int (*func)(), int wsize, int psize, ...)
	{
	PDes ws;

	ws = (PDes) malloc(wsize);
	psize <<= 2;			/* # of bytes of actual parameters */
	ws = (PDes) ((int) ws + wsize - sizeof(int) - psize);
	((int *) ws)[-1] = (int) func;
	bcopy((&psize + 1), ((int *) ws + 1), psize);
	return (ws);
	}

PDes
PSetup(void *ws, int (*func)(), int wsize, int psize, ...)
	{
	psize <<= 2;			/* # of bytes of actual parameters */
	ws = (int *) ((int) ws + wsize - sizeof(int) - psize);
	((int *) ws)[-1] = (int) func;
	bcopy((&psize + 1), ((int *) ws + 1), psize);
	return ((PDes) ws);
	}

/* Allocate channel pointer and initialise it . */
Channel	*
ChanAlloc(void)
	{
	Channel	*c;

	c = (Channel *) malloc(sizeof(Channel));
	*c = NOPROCESS;
	return(c);
	}

void	*
malloc(size)
	size_t	size;
	{
	char	*hp;
	extern	size_t	ho;
	extern	int	hs;

	hp = (char *)(&hs)+ho;
        ho += size; /*(size+3)/4*4;*/
	return((void *)hp);
	}

static	size_t	ho;
static	int	hs;
