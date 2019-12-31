!
!	setup.s		(C) 1991 Linus Torvalds
!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! This code asks the bios for memory/disk/other parameters, and
! puts them in a "safe" place: 0x90000-0x901FF, ie where the
! boot-block used to be. It is then up to the protected mode
! system to read them from there before the area is overwritten
! for buffer-blocks.
!

! NOTE! These had better be the same as in bootsect.s!

INITSEG  = 0x9000	! we move boot here - out of the way
SYSSEG   = 0x1000	! system loaded at 0x10000 (65536).
SETUPSEG = 0x9020	! this is the current segment

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

entry start
start:

	
! Print entry message of setup.s
	mov	ax,#SETUPSEG
	mov	es,ax

	mov	ah,#0x03		! read cursor pos
	xor	bh,bh
	int	0x10
	
	sub dh,#3
	mov	cx,#63
	mov	bx,#0x007		; page 0, normal attribute
	mov	bp,#msg_entry
	mov	ax,#0x1301		! write string, move cursor
	int	0x10

! ok, the read went well so we get current cursor position and save it for
! posterity.

	mov	ax,#INITSEG	! this is done in bootsect already, but...
	mov	ds,ax
	mov	ah,#0x03	! read cursor pos
	xor	bh,bh
	int	0x10		! save it in known place, con_init fetches
	mov	[0],dx		! it from 0x90000.

	mov bx, #msg_cursor_pos
	call print_string

	call print_nl
	call print_tab

	mov bx, #msg_cursor_pos_line_number
	call print_string
	mov	ax,#INITSEG
	mov	ds,ax
	xor dx,dx
	mov dl,[1]
	call print_hex

	call print_nl
	call print_tab

	mov bx, #msg_cursor_pos_column_number
	call print_string
	mov	ax,#INITSEG
	mov	ds,ax
	xor dx,dx
	mov dl,[0]
	call print_hex

suspend: 	
	sti
	mov ah,#0
	int 0x16
	j suspend


print_hex: ; set dx
    mov    cx,#4
print_digit:
    rol    dx,#4
    mov    ax,#0xe0f
    and    al,dl
    add    al,#0x30
    cmp    al,#0x3a
    jl    outp
    add    al,#0x07
outp: 
    int    0x10
    loop    print_digit
    ret


print_nl: ;void
    mov    ax,#0xe0d     ! CR
    int    0x10
    mov    al,#0xa     ! LF
    int    0x10
    ret


print_tab: ;void
	mov    ax,#0xe09    ! Tab
    int    0x10
	ret


print_string: ;set bx
	push bx
	mov	ax,#SETUPSEG
	mov ds,ax
	mov	es,ax

	mov	ah,#0x03		! read cursor pos
	xor	bh,bh
	int	0x10

	pop	bx
	mov bp,bx
	mov	cx,(bx-2)
	mov	ax,#0x1301		! write string, move cursor
	mov	bx,#0x007		; page 0, normal attribute
	int	0x10
	ret

.word 63
msg_entry:
	.byte 13,10
	.ascii "Project1: Control the boot ..."
	.byte 13,10,13,10
	.byte 13,10
	.ascii "Now we are in setup.s"
	.byte 13,10,13,10

.word 17
msg_cursor_pos:
	.ascii "Cursor position: "

.word 15
msg_cursor_pos_line_number:
	.ascii "Line number: 0x"

.word 17
msg_cursor_pos_column_number:
	.ascii "Column number: 0x"



.text
endtext:
.data
enddata:
.bss
endbss:
