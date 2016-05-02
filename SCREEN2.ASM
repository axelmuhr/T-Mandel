;********************************** SCREEN2.ASM *****************************
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
; This program  is a screen driver program written for madelbrot.
;***************************************************************************

	   TITLE   screen

_TEXT      SEGMENT  BYTE PUBLIC 'CODE'
_TEXT      ENDS
_DATA      SEGMENT  WORD PUBLIC 'DATA'
_DATA      ENDS
CONS      SEGMENT WORD PUBLIC 'CONS'
CONS      ENDS
_BSS       SEGMENT WORD PUBLIC 'BSS'
_BSS       ENDS
DGROUP     GROUP   CONS, _BSS, _DATA
           ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

gout       MACRO   idx,val
	   mov     dx,03CEH
           mov     al,idx
           out     dx,al
           mov     dx,03CFH
	   mov     al,val
	   out     dx,al
           ENDM

_TEXT      SEGMENT

; vga_vect(x,y,w,line)
; int x,y,w;
; char *line;

           PUBLIC  _vga_vect
_vga_vect  PROC    NEAR
           ; x = bp + 4
           ; y = bp + 6
           ; w = bp + 8
           ; line = bp + 10
	   ; register bl = mask
           ; register cx = veclen
           ; register si = line
           ; register es:di = addr
	   push    bp
	   mov     bp,sp
           push    di
           push    si

           mov     ax,479
           sub     ax,[bp+6]
           mov     bx,80
           mul     bx
           mov     di,[bp+4]
           mov     cl,3
           shr     di,cl
           add     di,ax               ; di = (349-y)*80+(x >> 3)
           mov     ax,0A000H
           mov     es,ax               ; es = 0xA000

	   mov     bl,80H
           mov     cl,[bp+4]
           and     cl,7
           shr     bl,cl               ; mask = 0x80 >> (x & 7)

	   mov     cx,[bp+8]           ; cx = w
           mov     si,[bp+10]          ; si = line

           gout    3,0                 ; gout(3,0)
           gout    5,2                 ; gout(5,2)
           mov     dx,3CEH
           mov     al,8
           out     dx,al
           mov     dx,3CFH
vv1:
           mov     al,bl
           out     dx,al               ; gout(8,mask)
           mov     al,es:[di]          ; latch data
           lodsb                       ; al = *line++
           mov     es:[di],al          ; *addr = al
           dec     cx                  ; cx--
           jz      vv2                 ; if (cx == 0) break
           shr     bl,1                ; mask >>= 1
           jnz     vv1                 ; if (!mask)
	   inc     di                  ; addr++
	   mov     bl,80H              ; mask = 0x80
           jmp     vv1
vv2:
           mov     al,0FFH
           out     dx,al               ; gout(8,0xFF)
           gout    5,0                 ; gout(5,0)

           pop     si
           pop     di
           pop     bp
           ret
_vga_vect  ENDP


_TEXT      ENDS
	   END
