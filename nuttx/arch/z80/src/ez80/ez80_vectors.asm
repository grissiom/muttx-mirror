;**************************************************************************
; arch/z80/src/ez80/ez80_vectors.asm
;
;   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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

;**************************************************************************
; Constants
;**************************************************************************

NVECTORS EQU 64		; max possible interrupt vectors

;**************************************************************************
; Global symbols used
;**************************************************************************

	xref	_ez80_startup
	xdef	_ez80_reset
	xdef	_ez80_initvectors
	xdef	_ez80_handlers
	xdef	_ez80_rstcommon
	xdef	_ez80_initvectors
	xdef	_ez80_vectable

;**************************************************************************
; Macros
;**************************************************************************

; Define one reset handler
;  1. Disable interrupts
;  2. Dlear mixed memory mode (MADL) flag
;  3. jump to initialization procedure with jp.lil to set ADL
rstvector: macro
	di
	rsmix
	jp.lil	_ez80_startup
	endmac	rstvector

; Define one interrupt handler
irqhandler: macro vectno
	; Save AF on the stack, set the interrupt number and jump to the
	; common reset handling logic.
					; Offset 8: Return PC is already on the stack
	push	af			; Offset 7: AF (retaining flags)
	ld	a, #vectno		; A = vector number
	jr	_ez80_rstcommon		; Remaining RST handling is common
	endmac	irqhandler

; Save Interrupt State
irqsave: macro
	ld	a, i			; sets parity bit to value of IEF2
	push af
	di				; disable interrupts while loading table 
	endmac	irqsave

; Restore Interrupt State
irqrestore: macro
	pop	af
	jp	po, $+5			; parity bit is IEF2
	ei
	endmac	irqrestore

;**************************************************************************
; Reset entry points
;**************************************************************************

	define	.RESET, space = ROM
	segment	.RESET

_ez80_reset:
_rst0:
	rstvector
_rst8:
	rstvector
_rst10:
	rstvector
_rst18:
	rstvector
_rst20:
	rstvector
_rst28:
	rstvector
_rst30:
	rstvector
_rst38:
	rstvector
	ds %26
_nmi:
	retn

;**************************************************************************
; Startup logic
;**************************************************************************

	define .STARTUP, space = ROM
	segment .STARTUP
	.assume ADL=1

;**************************************************************************
; Interrupt Vector Handling
;**************************************************************************

_ez80_handlers:
	irqhandler	 0
	handlersize equ . - _ez80handlers
	irqhandler	 1
	irqhandler	 2
	irqhandler	 3
	irqhandler	 4
	irqhandler	 5
	irqhandler	 6
	irqhandler	 7
	irqhandler	 8
	irqhandler	 9
	irqhandler	10
	irqhandler	11
	irqhandler	12
	irqhandler	13
	irqhandler	14
	irqhandler	15
	irqhandler	16
	irqhandler	17
	irqhandler	18
	irqhandler	19
	irqhandler	20
	irqhandler	21
	irqhandler	22
	irqhandler	23
	irqhandler	24
	irqhandler	25
	irqhandler	26
	irqhandler	27
	irqhandler	28
	irqhandler	29
	irqhandler	30
	irqhandler	31
	irqhandler	32
	irqhandler	33
	irqhandler	34
	irqhandler	35
	irqhandler	36
	irqhandler	37
	irqhandler	38
	irqhandler	39
	irqhandler	40
	irqhandler	41
	irqhandler	42
	irqhandler	43
	irqhandler	44
	irqhandler	45
	irqhandler	46
	irqhandler	47
	irqhandler	48
	irqhandler	49
	irqhandler	50
	irqhandler	51
	irqhandler	52
	irqhandler	53
	irqhandler	54
	irqhandler	55
	irqhandler	56
	irqhandler	57
	irqhandler	58
	irqhandler	59
	irqhandler	60
	irqhandler	61
	irqhandler	62
	irqhandler	63

;**************************************************************************
; Common Interrupt handler
;**************************************************************************

_ez80_rstcommon::
	; Create a register frame.  SP points to top of frame + 4, pushes
	; decrement the stack pointer.  Already have
	;
	;   Offset 8: Return PC is already on the stack
	;   Offset 7: AF (retaining flags)
	;
	; IRQ number is in A

	push	hl			; Offset 6: HL
	ld	hl, #(3*2)		;    HL is the value of the stack pointer before
	add	hl, sp			;    the interrupt occurred
	push	hl			; Offset 5: Stack pointer
	push	iy			; Offset 4: IY
	push	ix			; Offset 3: IX
	push	de			; Offset 2: DE
	push	bc			; Offset 1: BC

	ld	b, a			;   Save the reset number in B
	ld	a, i			;   Carry bit holds interrupt state
	push	af			; Offset 0: I with interrupt state in carry
	di

	; Call the interrupt decode logic. SP points to the beggining of the reg structure

	ld	hl, #0			; Argument #2 is the beginning of the reg structure
	add	hl, sp			;
	push	hl			; Place argument #2 at the top of stack
	push	bc			; Argument #1 is the Reset number
	inc	sp			; (make byte sized)
	call	_up_doirq		; Decode the IRQ

	; On return, HL points to the beginning of the reg structure to restore
	; Note that (1) the arguments pushed on the stack are not popped, and (2) the
	; original stack pointer is lost.  In the normal case (no context switch),
	; HL will contain the value of the SP before the arguments wer pushed.

	ld	sp, hl			; Use the new stack pointer

	; Restore registers.  HL points to the beginning of the reg structure to restore

	ex	af, af'			; Select alternate AF
	pop	af			; Offset 0: AF' = I with interrupt state in carry
	ex	af, af'			;   Restore original AF
	pop	bc			; Offset 1: BC
	pop	de			; Offset 2: DE
	pop	ix			; Offset 3: IX
	pop	iy			; Offset 4: IY
	exx				;   Use alternate BC/DE/HL
	ld	hl, #-2			;   Offset of SP to account for ret addr on stack
	pop	de			; Offset 5: HL' = Stack pointer after return
	add	hl, de			;   HL = Stack pointer value before return
	exx				;   Restore original BC/DE/HL
	pop	hl			; Offset 6: HL
	pop	af			; Offset 7: AF

	; Restore the stack pointer

	exx				; Use alternate BC/DE/HL
	ld	sp, hl			; Set SP = saved stack pointer value before return
	exx				; Restore original BC/DE/HL

	; Restore interrupt state

	ex	af, af'			; Recover interrupt state
	jr	nc, nointenable		; No carry, IFF2=0, means disabled
	ex	af, af'			; Restore AF (before enabling interrupts)
	ei				; yes
	reti
nointenable::
	ex	af, af'			; Restore AF
	reti

;**************************************************************************
; Vector Setup Logic
;**************************************************************************

_ez80_initvectors:
	; Initialize the vector table

	ld	hl, _vector_table
	ld	b, NVECTORS
	ld	iy, _ez80_handlers
	ld	a, 0
$1:
	ld	(iy), hl	; Store IRQ handler
	ld	(iy+3), a	; Pad to 4 bytes
	add	hl, handlersize	; Point to next handler
	add	iy, 4		; Point to next entry in vector table
	djnz	$2		; Loop until all vectors have been written

	; Select interrupt mode 2
	
	im	2		; Interrupt mode 2

	; Write the address of the vector table into the interrupt vector base

	ld	hl, _ez80_vectable >> 8
	ld	i, hl
	ret

;**************************************************************************
; Vector Table
;**************************************************************************
; This segment must be aligned on a 512 byte boundary anywhere in RAM
; Each entry will be a 3-byte address in a 4-byte space

	define	.IVECTS, space = RAM, align = 200h
	segment	.IVECTS

_ez80_vectable:
	ds NVECTORS * 4
