;********************************** LKIO.ASM ********************************
;  (C) Copyright 1987-1993  Computer System Architects, Provo UT.           *
;  This  program is the property of Computer System Architects (CSA)        *
;  and is provided only as an example of a transputer/PC program for        *
;  use  with CSA's Transputer Education Kit and other transputer products.  *
;  You may freely distribute copies or modifiy the program as a whole or in *
;  part, provided you insert in each copy appropriate copyright notices and *
;  disclaimer of warranty and send to CSA a copy of any modifications which *
;  you plan to distribute.						    *
;  This program is provided as is without warranty of any kind. CSA is not  *
;  responsible for any damages arising out of the use of this program.      *
;***************************************************************************/
;****************************************************************************
; This program  is a driver program written for transputer/pc
; interface card in assembly. It provides necessary functions to reset,
; analyze, send and receive data from the transputer. Primarily it is
; written for CSA's PART.0 link interface.
;***************************************************************************
	   TITLE   lkio

_TEXT      SEGMENT BYTE PUBLIC 'CODE'
_TEXT      ENDS
_DATA      SEGMENT WORD PUBLIC 'DATA'
_DATA      ENDS
CONST      SEGMENT WORD PUBLIC 'CONST'
CONST      ENDS
_BSS       SEGMENT WORD PUBLIC 'BSS'
_BSS       ENDS
DGROUP     GROUP   CONST, _BSS, _DATA
           ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

TRUE       EQU     1
FALSE      EQU     0
RST_CNT    EQU     20000               ; 42  millisec with 8 MHz 286
ANAL_CNT   EQU     60000               ; 126 millisec with 8 MHz 286
T1_CNT     EQU     64000               ; 472 millisec with 8 MHz 286 & T2 = 2
T2_CNT     EQU     2

DEF_BASE   EQU     0150H

IN_OFF     EQU     0
OUT_OFF    EQU     1
ISTAT_OFF  EQU     2
OSTAT_OFF  EQU     3
RST_OFF    EQU     16
ERR_OFF    EQU     16
ANAL_OFF   EQU     17
IDEN_OFF   EQU     18
ODEN_OFF   EQU     19

STAT_REG   EQU     8
MASK_REG   EQU     10
MODE_REG   EQU     11
FLOP_REG   EQU     12

CHX_WTMD   EQU     44H                 ; addr inc, no autoinit, write
CHX_RDMD   EQU     48H                 ; addr inc, no autoinit, read
CHX_SETM   EQU     04H                 ; set ch x mask

CH1_ADDR   EQU     2
CH1_COUNT  EQU     3
CH1_CLRM   EQU     01H                 ; clear ch 1 mask
CH1_TC     EQU     02H                 ; terminal count on ch 1
CH1_PREG   EQU     83H                 ; ch 1 page register

CH2_ADDR   EQU     4
CH2_COUNT  EQU     5
CH2_CLRM   EQU     02H                 ; clear ch 2 mask
CH2_TC     EQU     04H                 ; terminal count on ch 2
CH2_PREG   EQU     81H                 ; ch 2 page register

CH3_ADDR   EQU     6
CH3_COUNT  EQU     7
CH3_CLRM   EQU     03H                 ; clear ch 3 mask
CH3_TC     EQU     08H                 ; terminal count on ch 3
CH3_PREG   EQU     82H                 ; ch 3 page register

_DATA      SEGMENT
count      DW      0
in_port    DW      DEF_BASE + IN_OFF
out_port   DW      DEF_BASE + OUT_OFF
in_stat    DW      DEF_BASE + ISTAT_OFF
out_stat   DW      DEF_BASE + OSTAT_OFF
rst_port   DW      DEF_BASE + RST_OFF
err_port   DW      DEF_BASE + ERR_OFF
anal_port  DW      DEF_BASE + ANAL_OFF
in_den     DW      DEF_BASE + IDEN_OFF
out_den    DW      DEF_BASE + ODEN_OFF
in_mode    DB      CHX_WTMD + 1
out_mode   DB      CHX_RDMD + 1
in_setm    DB      CHX_SETM + 1
out_setm   DB      CHX_SETM + 1
in_jadd    DW      dma_ch1
out_jadd   DW      dma_ch1
jadd_arr   DW      dma_ch1, dma_ch1, dma_ch2, dma_ch3
_DATA      ENDS

; MACRO chkkey
; Input parameters:
;   None
; Output parameters:
;   al is the status byte
; Registers used:
;   al - returned value
;   ah - corrupted
; Action:
;   The keyboard status is returned
chkkey     MACRO                       ; Get keyboard status
           mov     ah,0BH
           int     21H
           ENDM

; MACRO waitlp
; Input parameters:
;   cnt is the number of times to loop
; Output parameters:
;   None
; Registers used:
;   cx - corrupted
; Action:
;   waits for cnt times around the loop
waitlp     MACRO   cnt
           LOCAL   loop
           mov     cx,cnt
loop:      dec     cx                  ; Wait a while
           jnz     loop
           ENDM

; MACRO ibyte
; Input parameters:
;   None
; Output parameters:
;   reg contains the byte read from the link
; Variables used:
;   al, dx - used to read the byte
;   count - used for time out to chkkey
; Action:
;   The link adaptor input status register is polled until a byte is
;   available on the link.
ibyte      MACRO   reg                 ; Read byte from link adaptor to reg
           LOCAL   loop,ready
           mov     dx,in_stat          ; First poll input status register
loop:
           in      al,dx               ; Read status of link adaptor
           test    al,1                ; Test bit zero
           jnz     ready               ; Ready to read byte
           dec     count
           jnz     loop                ; Keep polling if no time out
           chkkey                      ; Check for abort
           jmp     loop                ; Keep polling
ready:
           mov     dx,in_port          ; Link adaptor read data port
           in      al,dx               ; Read a byte
           mov     reg,al              ; Put the byte in reg
           ENDM

; MACRO obyte
; Input parameters:
;   reg contains the byte to be written to the link
; Output parameters:
;   None
; Variables used:
;   al, dx - used to write the byte
;   count - used for time out to chkkey
; Action:
;   The link adaptor output status register is polled until the link
;   is ready to accept a byte.
obyte      MACRO   reg                 ; Write byte in reg to link adaptor
           LOCAL   loop,ready
           mov     dx,out_stat         ; First poll output status register
loop:
           in      al,dx               ; Read status of link
           test    al,1                ; Test bit zero
           jnz     ready               ; Ready to write byte
           dec     count
           jnz     loop                ; Keep polling if no time out
           chkkey                      ; Check for abort
           jmp     loop                ; Keep polling
ready:
           mov     dx,out_port         ; Link adaptor write data port
           mov     al,reg              ; Put the byte to be written in al
           out     dx,al               ; Write it
           ENDM

; MACRO itbyte
; Input parameters:
;   abort gives the address to which control is passed if
;   time out occurs
; Output parameters:
;   reg contains the byte read from the link
; Variables used:
;   al, dx - used to access the link adaptor ports
;   cx, count - used for time out wait
itbyte     MACRO   reg,abort
           LOCAL   loop,ready
           mov     count,T1_CNT        ; count is used as a time out
           mov     dx,in_stat          ; First poll input status register
loop:
           in      al,dx               ; Read status
           test    al,1                ; Test bit zero
           jnz     ready               ; Ready to read byte
           waitlp  T2_CNT              ; Not ready
           dec     count
           jnz     loop                ; Keep polling
           jmp     SHORT abort         ; Time out reached
ready:
           mov     dx,in_port          ; Link adaptor read data port
           in      al,dx               ; Read a byte
           mov     reg,al              ; Put the byte in reg
           ENDM

; MACRO otbyte
; Input parameters:
;   reg gives the location of the byte to be written to the link
;   abort gives the address to which control is passed if
;   time out occurs
; Output parameters:
;   None
; Variables used:
;   al, dx - used to access the link adaptor ports
;   cx, count - used for time out wait
otbyte     MACRO   reg,abort
           LOCAL   loop,ready
           mov     count,T1_CNT        ; count is used as a time out
           mov     dx,out_stat         ; First poll output status register
loop:
           in      al,dx               ; Read status
           test    al,1                ; Test bit zero
           jnz     ready               ; Ready to write byte
           waitlp  T2_CNT              ; Not ready
           dec     count
           jnz     loop                ; Keep polling
           jmp     SHORT abort         ; Time out reached
ready:
           mov     dx,out_port         ; Link adaptor write data port
           mov     al,reg              ; Put the byte to be written in al
           out     dx,al               ; Write it
           ENDM

; MACRO caladr
; Input parameters:
;   ds is data segment
;   off parameter is offset from ds
; Output parameters:
;   bl is page value
;   ax is base address in page
; Registers used:
;   ax, bx - returned values
;   cx - corrupted
; Action:
;   Uses ds:off to compute page:base
caladr     MACRO   off
           LOCAL   shift
           mov     ax,ds
           xor     bx,bx
           mov     cx,4
shift:
           sal     ax,1
           rcl     bx,1
           loop    shift

           add     ax,off
           adc     bx,0
           ENDM

_TEXT      SEGMENT

; int err_flag()

           PUBLIC  _err_flag
_err_flag  PROC    NEAR
           mov     dx,err_port
           in      al,dx
           and     ax,1
           xor     ax,1
           ret
_err_flag  ENDP

; int busy_in()

           PUBLIC  _busy_in
_busy_in   PROC    NEAR
           mov     dx,in_stat          ; First poll input status register
           in      al,dx               ; Read status of link adaptor
           test    al,1                ; Test bit zero
           jnz     biready             ; Ready to read byte
           mov     ax,0FFFFH
           ret
biready:
           mov     dx,in_port          ; Link adaptor read data port
           in      al,dx               ; Read a byte
           mov     ah,0
           ret
_busy_in   ENDP

; int busy_out(b)
; int b;

           PUBLIC  _busy_out
_busy_out  PROC    NEAR
           mov     dx,out_stat         ; First poll output status register
           in      al,dx               ; Read status of link
           test    al,1                ; Test bit zero
           jnz     boready             ; Ready to write byte
           mov     ax,0
           ret
boready:
           push    bp
           mov     bp,sp
           mov     dx,out_port         ; Link adaptor write data port
           mov     al,[bp+4]           ; al = b
           out     dx,al               ; Write it
           mov     ax,1
           pop     bp
           ret
_busy_out  ENDP

; int byte_in()

           PUBLIC  _byte_in
_byte_in   PROC    NEAR
           ibyte   al
           mov     ah,0
           ret
_byte_in   ENDP

; byte_out(b)
; int b;

           PUBLIC  _byte_out
_byte_out  PROC    NEAR
           push    bp
           mov     bp,sp
           mov     bl,[bp+4]           ; bl = b
           obyte   bl
           pop     bp
           ret
_byte_out  ENDP

; int tbyte_in()

           PUBLIC  _tbyte_in
_tbyte_in  PROC    NEAR
           itbyte  al,tiabort
           mov     ah,0
           ret
tiabort:
           mov     ax,0FFFFH
           ret
_tbyte_in  ENDP

; int tbyte_out(b)
; int b;

           PUBLIC  _tbyte_out
_tbyte_out PROC    NEAR
           push    bp
           mov     bp,sp
           mov     bl,[bp+4]           ; bl = b
           otbyte  bl,toabort
           mov     ax,1
           pop     bp
           ret
toabort:
           mov     ax,0
           pop     bp
           ret
_tbyte_out ENDP

; long word_in()

           PUBLIC  _word_in
_word_in   PROC    NEAR
           ibyte   bl
           ibyte   bh
           ibyte   cl
           ibyte   ch
           mov     ax,bx
           mov     dx,cx
           ret
_word_in   ENDP

; word_out(w)
; long w;

           PUBLIC  _word_out
_word_out  PROC    NEAR
           push    bp
           mov     bp,sp
           mov     bx,[bp+4]           ; bx = w.lo
           mov     cx,[bp+6]           ; cx = w.hi
           obyte   bl
           obyte   bh
           obyte   cl
           obyte   ch
           pop     bp
           ret
_word_out  ENDP

; chan_in(buf,bcnt)
; char *buf;
; unsigned int bcnt;

           PUBLIC  _chan_in
_chan_in   PROC    NEAR
           push    bp
           mov     bp,sp
           push    si
           mov     si,[bp+4]           ; si = buf
           mov     cx,[bp+6]           ; cx = bcnt
           jcxz    chani2
chani1:
           ibyte   [si]                ; Receive a byte
           inc     si                  ; Increment pointer
           loop    chani1
chani2:
           pop     si
           pop     bp
           ret
_chan_in   ENDP

; chan_out(buf,bcnt)
; char *buf;
; unsigned int bcnt;

           PUBLIC  _chan_out
_chan_out  PROC    NEAR
           push    bp
           mov     bp,sp
           push    si
           mov     si,[bp+4]           ; si = buf
           mov     cx,[bp+6]           ; cx = bcnt
           jcxz    chano2
chano1:
           obyte   [si]                ; Send a byte
           inc     si                  ; Increment pointer
           loop    chano1
chano2:
           pop     si
           pop     bp
           ret
_chan_out  ENDP

; ax = base address
; bl = page register
; bh = mode value
; cx = byte count - 1
; dx = jump address for channel

dma_tran   PROC    NEAR
           cli                         ; Disable interrupts
           out     FLOP_REG,al         ; Clear byte pointer flip-flop
           jmp     dx
dma_ch1:
           out     CH1_ADDR,al         ; Output low address
           xchg    ah,al
           out     CH1_ADDR,al         ; Output high address
           mov     al,cl
           out     CH1_COUNT,al        ; Output low  count
           mov     al,ch
           out     CH1_COUNT,al        ; Output high count
           mov     al,bl
           out     CH1_PREG,al         ; Output page value
           mov     dx,CH1_CLRM SHL 8 OR CH1_TC
           jmp     SHORT dma_md
dma_ch2:
           out     CH2_ADDR,al         ; Output low address
           xchg    ah,al
           out     CH2_ADDR,al         ; Output high address
           mov     al,cl
           out     CH2_COUNT,al        ; Output low  count
           mov     al,ch
           out     CH2_COUNT,al        ; Output high count
           mov     al,bl
           out     CH2_PREG,al         ; Output page value
           mov     dx,CH2_CLRM SHL 8 OR CH2_TC
           jmp     SHORT dma_md
dma_ch3:
           out     CH3_ADDR,al         ; Output low address
           xchg    ah,al
           out     CH3_ADDR,al         ; Output high address
           mov     al,cl
           out     CH3_COUNT,al        ; Output low  count
           mov     al,ch
           out     CH3_COUNT,al        ; Output high count
           mov     al,bl
           out     CH3_PREG,al         ; Output page value
           mov     dx,CH3_CLRM SHL 8 OR CH3_TC
dma_md:
           mov     al,bh
           out     MODE_REG,al         ; Output mode value
           sti                         ; Re-enable interrupts
           mov     al,dh
           out     MASK_REG,al         ; Clear mask bit
dma_tc:
           in      al,STAT_REG
           test    al,dl
           jnz     dma_done            ; Done, terminal count reached
           dec     count
           jnz     dma_tc              ; Keep polling if no time out
           chkkey                      ; Check for abort
           jmp     dma_tc              ; Keep polling
dma_done:
           ret
dma_tran   ENDP

; dma_in(buf,bcnt)
; char *buf;
; unsigned int bcnt;

           PUBLIC  _dma_in
_dma_in    PROC    NEAR
           push    bp
           mov     bp,sp
           mov     al,1
           mov     dx,in_den           ; dx = dma enable port
           out     dx,al               ; Enable adaptor DMA
           mov     al,2
           mov     dx,in_stat          ; dx = interrupt enable port
           out     dx,al               ; Enable adaptor interrupts
           caladr  [bp+4]              ; bl = page, ax = offset
           mov     bh,in_mode          ; bh = write mode
           mov     cx,[bp+6]           ; cx = bcnt
           jcxz    din2
           dec     cx

           mov     dx,ax
           add     dx,cx               ; carry set if cross page bound
           mov     dx,in_jadd          ; dx = in channel jump address
           jnc     din1

           mov     cx,0FFFFH
           sub     cx,ax               ; bcnt1 = 0FFFFH - offset
           call    dma_tran            ; First block to page bound

           mov     ax,[bp+6]           ; ax = bcnt
           sub     ax,2
           sub     ax,cx
           mov     cx,ax               ; bcnt2 = (bcnt-2) - bcnt1
           xor     ax,ax               ; Base address = 0000H
           inc     bx                  ; Next page
           mov     dx,in_jadd          ; dx = in channel jump address
din1:
           call    dma_tran            ; All or second block
din2:
           xor     al,al
           mov     dx,in_stat          ; dx = interrupt enable port
           out     dx,al               ; Disable adaptor interrupts
           mov     dx,in_den           ; dx = dma enable port
           out     dx,al               ; Disable adaptor DMA
           pop     bp
           ret
_dma_in    ENDP

; dma_out(buf,bcnt)
; char *buf;
; unsigned int bcnt;

           PUBLIC  _dma_out
_dma_out   PROC    NEAR
           push    bp
           mov     bp,sp
           mov     al,1
           mov     dx,out_den          ; dx = dma enable port
           out     dx,al               ; Enable adaptor DMA
           mov     al,2
           mov     dx,out_stat         ; dx = interrupt enable port
           out     dx,al               ; Enable adaptor interrupts
           caladr  [bp+4]              ; bl = page, ax = offset
           mov     bh,out_mode         ; bh = read mode
           mov     cx,[bp+6]           ; cx = bcnt
           jcxz    dout2
           dec     cx

           mov     dx,ax
           add     dx,cx               ; carry set if cross page bound
           mov     dx,out_jadd         ; dx = out channel jump address
           jnc     dout1

           mov     cx,0FFFFH
           sub     cx,ax               ; bcnt1 = 0FFFFH - offset
           call    dma_tran            ; First block to page bound

           mov     ax,[bp+6]           ; ax = bcnt
           sub     ax,2
           sub     ax,cx
           mov     cx,ax               ; bcnt2 = (bcnt-2) - bcnt1
           xor     ax,ax               ; Base address = 0000H
           inc     bx                  ; Next page
           mov     dx,out_jadd         ; dx = out channel jump address
dout1:
           call    dma_tran
dout2:
           xor     al,al
           mov     dx,out_stat         ; dx = interrupt enable port
           out     dx,al               ; Disable adaptor interrupts
           mov     dx,out_den          ; dx = dma enable port
           out     dx,al               ; Disable adaptor DMA
           pop     bp
           ret
_dma_out   ENDP

; dma_on()

           PUBLIC  _dma_on
_dma_on    PROC    NEAR
           mov     al,in_setm
           out     MASK_REG,al         ; Set in channel mask bit
           mov     al,out_setm
           out     MASK_REG,al         ; Set out channel mask bit
           in      al,STAT_REG         ; Clear status reg
           mov     dx,in_port
           out     dx,al
           ret
_dma_on    ENDP

; dma_off()

           PUBLIC  _dma_off
_dma_off   PROC    NEAR
           mov     dx,out_port
           in      al,dx
           ret
_dma_off   ENDP

; rst_adpt(reset)
; int reset;

           PUBLIC  _rst_adpt
_rst_adpt  PROC    NEAR
           push    bp
           mov     bp,sp
           waitlp  RST_CNT             ; Wait a while
           mov     dx,anal_port
           mov     al,FALSE
           out     dx,al               ; Deassert analyse
           mov     dx,rst_port
           mov     al,FALSE
           out     dx,al               ; Deassert reset
           waitlp  RST_CNT             ; Wait a while

           cmp     WORD PTR [bp+4],FALSE ; reset FALSE?
           jne     SHORT rst1
           mov     dx,anal_port
           mov     al,TRUE
           out     dx,al               ; Assert analyse
           waitlp  ANAL_CNT            ; Wait a while
rst1:
           mov     dx,rst_port
           mov     al,TRUE
           out     dx,al               ; Assert reset
           waitlp  RST_CNT             ; Wait a while
           mov     al,FALSE
           out     dx,al               ; Deassert reset

           cmp     WORD PTR [bp+4],FALSE ; reset FALSE?
           jne     SHORT rst2
           waitlp  RST_CNT             ; Wait a while
           mov     dx,anal_port
           mov     al,FALSE
           out     dx,al               ; Deassert analyse
rst2:
           pop     bp
           ret
_rst_adpt  ENDP

; int init_lkio(base,dmai,dmao)
; int base,dmai,dmao;

           PUBLIC _init_lkio
_init_lkio PROC    NEAR
           push    bp
           mov     bp,sp
           mov     bx,[bp+4]           ; bx = base
           lea     ax,[bx+IN_OFF]      ; Setup I/O port addresses
           mov     in_port,ax
           lea     ax,[bx+OUT_OFF]
           mov     out_port,ax
           lea     ax,[bx+ISTAT_OFF]
           mov     in_stat,ax
           lea     ax,[bx+OSTAT_OFF]
           mov     out_stat,ax
           lea     ax,[bx+RST_OFF]
           mov     rst_port,ax
           lea     ax,[bx+ERR_OFF]
           mov     err_port,ax
           lea     ax,[bx+ANAL_OFF]
           mov     anal_port,ax
           lea     ax,[bx+IDEN_OFF]
           mov     in_den,ax
           lea     ax,[bx+ODEN_OFF]
           mov     out_den,ax

           mov     bl,[bp+6]           ; bl = dma in channel
           and     bx,3
           mov     al,bl
           or      al,CHX_WTMD
           mov     in_mode,al
           mov     al,bl
           or      al,CHX_SETM
           mov     in_setm,al
           shl     bx,1
           mov     ax,jadd_arr[bx]
           mov     in_jadd,ax

           mov     bl,[bp+8]           ; bl = dma out channel
           and     bx,3
           mov     al,bl
           or      al,CHX_RDMD
           mov     out_mode,al
           mov     al,bl
           or      al,CHX_SETM
           mov     out_setm,al
           shl     bx,1
           mov     ax,jadd_arr[bx]
           mov     out_jadd,ax

           xor     al,al
           mov     dx,in_stat
           out     dx,al               ; Disable adaptor interrupts (input)
           mov     dx,out_stat
           out     dx,al               ; Disable adaptor interrupts (output)
           mov     dx,in_den
           out     dx,al               ; Disable adaptor DMA (input)
           mov     dx,out_den
           out     dx,al               ; Disable adaptor DMA (output)

           mov     dx,in_stat          ; Check if adaptor found
           in      al,dx
           test    al,0FEH
           jz      init1
           mov     ax,0                ; Return 0 if not found
           jmp     SHORT init2
init1:
           mov     ax,1                ; Return 1 if found
init2:
           pop     bp
           ret
_init_lkio ENDP

_TEXT      ENDS
           END
