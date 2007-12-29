;**************************************************************************
; arch/z80/src/common/up_restoreusercontext.asm
;
;   Copyright (C) 2007, 2008 Gregory Nutt. All rights reserved.
;   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in
;    the documentation and/or other materials provided with the
;    distribution.
; 3. Neither the name NuttX nor the names of its contributors may be
;    used to endorse or promote products derived from this software
;    without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
; FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
; COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
; OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
; AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
; ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
;**************************************************************************

	; Register save area layout

	XCPT_I	==  0		; Saved I w/interrupt state in carry
	XCPT_AF	==  2		; Saved AF register
	XCPT_BC	==  4		; Saved BC register
	XCPT_DE	==  6		; Saved DE register
	XCPT_HL	==  8		; Saved HL register
	XCPT_IX	== 10		; Saved IX register
	XCPT_IY	== 12		; Saved IY register
	XCPT_SP	== 14		; Offset to SP at time of interrupt
	XCPT_PC	== 16		; Offset to PC at time of interrupt

;**************************************************************************
; up_restoreusercontext
;**************************************************************************

	.org   0x0038			; Int mode 1
        .area   TEXT    (ABS,OVR)
up_restoreusercontext:
	; On entry, stack contains return address (not used), then address
	; of the register save structure

	; Discard the return address, we won't be returning

	pop	hl

	; Get the address of the beginning of the state save area.  Each
	; pop will increment to the next element of the structure

	pop	hl		; BC = Address of save structure
	ld	sp, hl		; SP points to top of storage area

	; Disable interrupts while we muck with the alternative registers

	; Restore registers.  HL points to the beginning of the reg structure to restore

	ex	af, af'			; Select alternate AF
	pop	af			; Offset 0: AF' = I with interrupt state in carry
	pop	bc			; Offset 1: BC
	pop	de			; Offset 2: DE
	pop	ix			; Offset 3: IX
	pop	iy			; Offset 4: IY
	exx				;   Use alternate BC/DE/HL
	pop	hl			; Offset 5: HL' = Stack pointer at time of interrupt
	exx
	pop	hl			; Offset 6: HL
	pop	af			; Offset 7: AF

	; Restore the stack pointer

	exx
	ld	sp, hl
	exx

	; Restore interrupt state

	ex	af, af'			; Recover interrupt state
	jnc	noinrestore		; No carry, IFF2=0, means disabled
	ex	af, af'			; Restore AF (before enabling interrupts)
	ei				; yes
	ret
noinrestore:
	ex	af, af'			; Restore AF
	ret
