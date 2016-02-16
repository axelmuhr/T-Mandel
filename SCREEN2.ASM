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
CONST      SEGMENT WORD PUBLIC 'CONST'
CONST      ENDS
_BSS       SEGMENT WORD PUBLIC 'BSS'
_BSS       ENDS
DGROUP     GROUP   CONST, _BSS, _DATA
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

; ega_vect(x,y,w,line)
; int x,y,w;
; char *line;

           PUBLIC  _ega_vect
_ega_vect  PROC    NEAR
	   ; x = 4
	   ; y = 6
           ; w = 8
           ; line = 10
           ; register bl = mask
           ; register cx = veclen
           ; register si = line
           ; register es:di = addr
           push    bp
           mov     bp,sp
           push    di
           push    si

           mov     ax,349
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
ev1:
           mov     al,bl
	   out     dx,al               ; gout(8,mask)
	   mov     al,es:[di]          ; latch data
           lodsb                       ; al = *line++
           mov     es:[di],al          ; *addr = al
           dec     cx                  ; cx--
           jz      ev2                 ; if (cx == 0) break
           shr     bl,1                ; mask >>= 1
           jnz     ev1                 ; if (!mask)
           inc     di                  ; addr++
           mov     bl,80H              ; mask = 0x80
           jmp     ev1
ev2:
           mov     al,0FFH
           out     dx,al               ; gout(8,0xFF)
           gout    5,0                 ; gout(5,0)

           pop     si
           pop     di
           pop     bp
           ret
_ega_vect  ENDP

; cga_vect(x,y,w,line)
; int x,y,w;
; char *line;

           PUBLIC  _cga_vect
_cga_vect  PROC    NEAR
           ; x = 4
           ; y = 6
           ; w = 8
           ; line = 10
           ; register bl = mask
           ; register cx = veclen
           ; register si = line
           ; register es:di = addr
           push    bp
           mov     bp,sp
           push    di
           push    si

	   mov     ax,199
           sub     ax,[bp+6]
           mov     dx,ax
           and     ax,1
           jz      cv1
           mov     ax,2000H            ; ax = ((199-y) & 1) * 0x2000
cv1:
           mov     di,[bp+4]
           shr     di,1
           shr     di,1                ; di = x >> 2
           add     di,ax               ; di = di + ax
           mov     ax,dx
           shr     ax,1
           mov     bx,80
           mul     bx
           add     di,ax               ; di = di + (dx >> 1) * 80
           mov     ax,0B800H
           mov     es,ax               ; es = 0xB800

	   mov     bl,0C0H
	   mov     cl,[bp+4]
           and     cl,3
           shl     cl,1
           shr     bl,cl               ; mask = 0xC0 >> ((x & 3) << 1)

           mov     cx,[bp+8]           ; cx = w
           mov     si,[bp+10]          ; si = line
cv2:
           lodsb
           and     al,3
           mov     bh,55H
           mul     bh
           and     al,bl
           mov     ah,al               ; ah = ((*line++ & 3) * 0x55) & mask
           mov     bh,bl
           not     bh
           mov     al,es:[di]
           and     al,bh
           or      al,ah
	   mov     es:[di],al          ; *addr = *addr & ~mask | ah
	   dec     cx
           jz      cv3                 ; if (cx == 0) break
           shr     bl,1
           shr     bl,1                ; mask >>= 2
           jnz     cv2                 ; if (!mask)
           inc     di                  ; addr++
           mov     bl,0C0H             ; mask = 0xC0
           jmp     cv2
cv3:
           pop     si
           pop     di
           pop     bp
           ret
_cga_vect  ENDP

; hrc_vect(x,y,w,line)
; int x,y,w;
; char *line;

	   PUBLIC  _hrc_vect
_hrc_vect  PROC    NEAR
           ; x = 4
           ; y = 6
           ; w = 8
           ; line = 10
           ; register bl = mask
           ; register cx = veclen
           ; register si = line
           ; register es:di = addr
           push    bp
           mov     bp,sp
           push    di
           push    si

           mov     ax,347
           sub     ax,[bp+6]
           mov     si,ax
           and     ax,3
           mov     bx,2000H
	   mul     bx                  ; ax = ((347-y) & 3) * 0x2000
	   mov     di,[bp+4]
           mov     cl,3
           shr     di,cl               ; di = x >> 3
           add     di,ax               ; di = di + ax
           mov     ax,si
           shr     ax,1
           shr     ax,1
           mov     bx,90
           mul     bx
           add     di,ax               ; di = di + (dx >> 2) * 90
           mov     ax,0B000H
           mov     es,ax               ; es = 0xB000

           mov     bl,80H
           mov     cl,[bp+4]
           and     cl,7
           shr     bl,cl               ; mask = 0x80 >> (x & 7)

           mov     dx,0200H            ; dx = 0x02,0x00
	   and     si,1
	   jz      hv1
           mov     dx,0103H            ; if (y & 1) dx = 0x01,0x03
hv1:
           and     cl,1
           jz      hv2
           xchg    dh,dl               ; if (x & 1) dx = swapbytes(dx)
hv2:
           mov     cx,[bp+8]           ; cx = w
           mov     si,[bp+10]          ; si = line
hv3:
           lodsb
           sub     ah,ah
           mov     bh,5
           div     bh
           cmp     dl,ah
           jge     hv4                 ; if (*line++ % 5 > dl)
           or      es:[di],bl          ; *addr = *addr | mask
           jmp     SHORT hv5
hv4:                                   ; else
	   mov     bh,bl
	   not     bh
           and     es:[di],bh          ; *addr = *addr & ~mask
hv5:
           dec     cx                  ; cx--
           jz      hv6                 ; if (cx == 0) break
           xchg    dh,dl               ; dx = swapbytes(dx)
           shr     bl,1                ; mask >>= 1
           jnz     hv3                 ; if (!mask)
           inc     di                  ; addr++
           mov     bl,80H              ; mask = 0x80
           jmp     hv3
hv6:
           pop     si
           pop     di
           pop     bp
           ret
_hrc_vect  ENDP

_TEXT      ENDS
	   END
