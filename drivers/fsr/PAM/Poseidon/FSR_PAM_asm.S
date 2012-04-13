@; Copyright (C) 2003-2010 Samsung Electronics      
@; This program is free software; you can redistribute it and/or modify
@; it under the terms of the GNU General Public License version 2 as      
@; published by the Free Software Foundation.      
@; version     %VERSION%

@;	.section C$$code @;@,CODE,READONLY
	.text
	.code 32	
	.globl		memcpy32
memcpy32:
@; Check for zero length or source and target being the same

    cmp     r2, #0
    cmpne   r0, r1
    addeq   r0, r0, r2
    moveq   pc, lr @; __JUMP(eq,lr);

@;   If <16 bytes, just do byte moves

    cmp     r2, #15
    bhi     large_copy

    cmp     r1, r0
    bhi     small_copy_fwd
    add     r0, r0, r2
    add     r1, r1, r2
    mov     r3, r0
    add     r12, pc, #small_copy_back_end-.-8
    sub     pc, r12, r2, lsl #3

    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    ldrb    r12, [r1, #-1]!
    strb    r12, [r3, #-1]!
    
small_copy_back_end:
    mov     pc, lr @;__JUMP(,lr)

small_copy_fwd:
    add     r12, pc, #small_copy_fwd_end-.-8
    sub     pc, r12, r2, lsl #3

    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1
    ldrb    r12, [r1], #1
    strb    r12, [r0], #1

small_copy_fwd_end:
    mov     pc, lr    @;__JUMP(,lr)

large_copy:

@; Check for possible overlap of source and target

    cmp     r1, r0
    bcc     copy_back
    stmfd   sp!, {r4-r11,lr}

@; Do byte moves if necessary to word-align destination

    movs    r3, r0, lsl #30
    rsbne   r3, r3, #0          @; r3=number of bytes to align destination
    msr     cpsr_f, r3
    ldrmib  lr, [r1], #1        @; move bytes to align destination
    strmib  lr, [r0], #1
    ldrmib  lr, [r1], #1
    strmib  lr, [r0], #1
    ldreqb  lr, [r1], #1
    streqb  lr, [r0], #1
    sub     r2, r2, r3, lsr #30 @; adjust length

@; Normal copy forwards. r0 should point to end address on exit
@; Destination now word-aligned; if source is also word-aligned, do aligned copy.

    ands    r12, r1, #3     @; r12=alignment of source
    beq     same_alignment_copy_fore

    bic     r1, r1, #3      @; align source
    ldr     r11, [r1], #4   @; get first word

    cmp     r12, #2
    beq     start_copy_fwd_nonaligned_2
    bhi     start_copy_fwd_nonaligned_3
    
start_copy_fwd_nonaligned_1:

    movs    lr, r2, lsr #5  @; lr=number of 32-byte blocks
    beq     copy_fwd_smaller_1
copy_fwd_nonaligned_1:
    mov     r3, r11, lsr #8
    ldmia   r1!, {r4-r11}
    orr     r3, r3, r4, lsl #24
    mov     r4, r4, lsr #8
    orr     r4, r4, r5, lsl #24
    mov     r5, r5, lsr #8
    orr     r5, r5, r6, lsl #24
    mov     r6, r6, lsr #8
    orr     r6, r6, r7, lsl #24
    mov     r7, r7, lsr #8
    orr     r7, r7, r8, lsl #24
    mov     r8, r8, lsr #8
    orr     r8, r8, r9, lsl #24
    mov     r9, r9, lsr #8
    orr     r9, r9, r10, lsl #24
    mov     r10, r10, lsr #8
    orr     r10, r10, r11, lsl #24
    stmia   r0!, {r3-r10}
    subs    lr, lr, #1
    bne     copy_fwd_nonaligned_1
    
copy_fwd_smaller_1:
    mov     r2, r2, lsl #26
    msr     cpsr_f, r2
    moveq   r3, r11, lsr #8
    ldmeqia r1!, {r4-r6,r11}
    orreq   r3, r3, r4, lsl #24
    moveq   r4, r4, lsr #8
    orreq   r4, r4, r5, lsl #24
    moveq   r5, r5, lsr #8
    orreq   r5, r5, r6, lsl #24
    moveq   r6, r6, lsr #8
    orreq   r6, r6, r11, lsl #24
    stmeqia r0!, {r3-r6}
    movcs   r3, r11, lsr #8
    ldmcsia r1!, {r4,r11}
    orrcs   r3, r3, r4, lsl #24
    movcs   r4, r4, lsr #8
    orrcs   r4, r4, r11, lsl #24
    stmcsia r0!, {r3,r4}
    movvs   r3, r11, lsr #8
    ldmvsia r1!, {r11}
    orrvs   r3, r3, r11, lsl #24
    stmvsia r0!, {r3}
    b       copy_fwd_remainder
start_copy_fwd_nonaligned_2:
    movs    lr, r2, lsr #5  @; lr=number of 32-byte blocks
    beq     copy_fwd_smaller_2
copy_fwd_nonaligned_2:
    mov     r3, r11, lsr #16
    ldmia   r1!, {r4-r11}
    orr     r3, r3, r4, lsl #16
    mov     r4, r4, lsr #16
    orr     r4, r4, r5, lsl #16
    mov     r5, r5, lsr #16
    orr     r5, r5, r6, lsl #16
    mov     r6, r6, lsr #16
    orr     r6, r6, r7, lsl #16
    mov     r7, r7, lsr #16
    orr     r7, r7, r8, lsl #16
    mov     r8, r8, lsr #16
    orr     r8, r8, r9, lsl #16
    mov     r9, r9, lsr #16
    orr     r9, r9, r10, lsl #16
    mov     r10, r10, lsr #16
    orr     r10, r10, r11, lsl #16
    stmia   r0!, {r3-r10}
    subs    lr, lr, #1
    bne     copy_fwd_nonaligned_2
    
copy_fwd_smaller_2:
    mov     r2, r2, lsl #26
    msr     cpsr_f, r2
    moveq   r3, r11, lsr #16
    ldmeqia r1!, {r4-r6,r11}
    orreq   r3, r3, r4, lsl #16
    moveq   r4, r4, lsr #16
    orreq   r4, r4, r5, lsl #16
    moveq   r5, r5, lsr #16
    orreq   r5, r5, r6, lsl #16
    moveq   r6, r6, lsr #16
    orreq   r6, r6, r11, lsl #16
    stmeqia r0!, {r3-r6}
    movcs   r3, r11, lsr #16
    ldmcsia r1!, {r4,r11}
    orrcs   r3, r3, r4, lsl #16
    movcs   r4, r4, lsr #16
    orrcs   r4, r4, r11, lsl #16
    stmcsia r0!, {r3,r4}
    movvs   r3, r11, lsr #16
    ldmvsia r1!, {r11}
    orrvs   r3, r3, r11, lsl #16
    stmvsia r0!, {r3}
    b       copy_fwd_remainder
    
start_copy_fwd_nonaligned_3:
    movs    lr, r2, lsr #5   @;lr=number of 32-byte blocks
    beq     copy_fwd_smaller_3 
copy_fwd_nonaligned_3:
    mov     r3, r11, lsr #24
    ldmia   r1!, {r4-r11}
    orr     r3, r3, r4, lsl #8
    mov     r4, r4, lsr #24
    orr     r4, r4, r5, lsl #8
    mov     r5, r5, lsr #24
    orr     r5, r5, r6, lsl #8
    mov     r6, r6, lsr #24
    orr     r6, r6, r7, lsl #8
    mov     r7, r7, lsr #24
    orr     r7, r7, r8, lsl #8
    mov     r8, r8, lsr #24
    orr     r8, r8, r9, lsl #8
    mov     r9, r9, lsr #24
    orr     r9, r9, r10, lsl #8
    mov     r10, r10, lsr #24
    orr     r10, r10, r11, lsl #8
    stmia   r0!, {r3-r10}
    subs    lr, lr, #1
    bne     copy_fwd_nonaligned_3
    
copy_fwd_smaller_3:
    mov     r2, r2, lsl #26
    msr     cpsr_f, r2
    moveq   r3, r11, lsr #24
    ldmeqia r1!, {r4-r6,r11}
    orreq   r3, r3, r4, lsl #8
    moveq   r4, r4, lsr #24
    orreq   r4, r4, r5, lsl #8
    moveq   r5, r5, lsr #24
    orreq   r5, r5, r6, lsl #8
    moveq   r6, r6, lsr #24
    orreq   r6, r6, r11, lsl #8
    stmeqia r0!, {r3-r6}
    movcs   r3, r11, lsr #24
    ldmcsia r1!, {r4,r11}
    orrcs   r3, r3, r4, lsl #8
    movcs   r4, r4, lsr #24
    orrcs   r4, r4, r11, lsl #8
    stmcsia r0!, {r3,r4}
    movvs   r3, r11, lsr #24
    ldmvsia r1!, {r11}
    orrvs   r3, r3, r11, lsl #8
    stmvsia r0!, {r3}

copy_fwd_remainder:
    sub     r1, r1, #4
    mov     r2, r2, lsl #2
    msr     cpsr_f, r2
    add     r1, r1, r12     @; true source address (nonaligned)
    ldrcsb  lr, [r1], #1        @; do byte copies to finish
    strcsb  lr, [r0], #1 
    ldrcsb  lr, [r1], #1 
    strcsb  lr, [r0], #1 
    ldrvsb  lr, [r1], #1
    strvsb  lr, [r0], #1
    ldmfd   sp!, {r4-r11, pc} @;__POPRET("r4-r11,")
    

@; Source is before target, so need to copy backwards incase they overlap

copy_back:
    add     r0, r0, r2          @; r0=last dest address+1
    add     r1, r1, r2              @; r1=last source address+1
    stmfd   sp!, {r0,r4-r11,lr}     @; save registers and return value
    mov     r3, r0, lsl #30         @; destination alignment into r3
    msr     cpsr_f, r3          @; and into N,Z flags
    ldrmib  lr, [r1, #-1]!          @; do byte moves to align destination
    strmib  lr, [r0, #-1]!
    ldrmib  lr, [r1, #-1]!
    strmib  lr, [r0, #-1]!
    ldreqb  lr, [r1, #-1]!
    streqb  lr, [r0, #-1]!
    sub     r2, r2, r3, lsr #30 @; adjust length

    ands    r12, r1, #3                 @; r12=alignment of source
    beq     same_alignment_copy_back    @; if source also aligned, do aligned copy

    bic     r1, r1, #3      @; align source
    ldr     r3, [r1]        @; get first word

    cmp     r12, #2
    beq     start_copy_back_nonaligned_2
    bhi     start_copy_back_nonaligned_3
start_copy_back_nonaligned_1:
    movs    lr, r2, lsr #5  @; lr=number of 32-byte blocks
    beq     copy_back_smaller_1
copy_back_nonaligned_1:
    mov     r11, r3, lsl #24
    ldmdb   r1!, {r3-r10}
    orr     r11, r11, r10, lsr #8
    mov     r10, r10, lsl #24
    orr     r10, r10, r9, lsr #8
    mov     r9, r9, lsl #24
    orr     r9, r9, r8, lsr #8
    mov     r8, r8, lsl #24
    orr     r8, r8, r7, lsr #8
    mov     r7, r7, lsl #24
    orr     r7, r7, r6, lsr #8
    mov     r6, r6, lsl #24
    orr     r6, r6, r5, lsr #8
    mov     r5, r5, lsl #24
    orr     r5, r5, r4, lsr #8
    mov     r4, r4, lsl #24
    orr     r4, r4, r3, lsr #8
    stmdb   r0!, {r4-r11}
    subs    lr, lr, #1
    bne     copy_back_nonaligned_1
copy_back_smaller_1:
    mov     r2, r2, lsl #26
    msr     cpsr_f, r2
    moveq   r7, r3, lsl #24
    ldmeqdb r1!, {r3-r6}
    orreq   r7, r7, r6, lsr #8
    moveq   r6, r6, lsl #24
    orreq   r6, r6, r5, lsr #8
    moveq   r5, r5, lsl #24
    orreq   r5, r5, r4, lsr #8
    moveq   r4, r4, lsl #24
    orreq   r4, r4, r3, lsr #8
    stmeqdb r0!, {r4-r7}
    movcs   r5, r3, lsl #24
    ldmcsdb r1!, {r3,r4}
    orrcs   r5, r5, r4, lsr #8
    movcs   r4, r4, lsl #24
    orrcs   r4, r4, r3, lsr #8
    stmcsdb r0!, {r4,r5}
    movvs   r4, r3, lsl #24
    ldmvsdb r1!, {r3}
    orrvs   r4, r4, r3, lsr #8
    stmvsdb r0!, {r4}
    b       copy_back_remainder
start_copy_back_nonaligned_2:
    movs    lr, r2, lsr #5  @; lr=number of 32-byte blocks
    beq     copy_back_smaller_2
copy_back_nonaligned_2:
    mov     r11, r3, lsl #16
    ldmdb   r1!, {r3-r10}
    orr     r11, r11, r10, lsr #16
    mov     r10, r10, lsl #16
    orr     r10, r10, r9, lsr #16
    mov     r9, r9, lsl #16
    orr     r9, r9, r8, lsr #16
    mov     r8, r8, lsl #16
    orr     r8, r8, r7, lsr #16
    mov     r7, r7, lsl #16
    orr     r7, r7, r6, lsr #16
    mov     r6, r6, lsl #16
    orr     r6, r6, r5, lsr #16
    mov     r5, r5, lsl #16
    orr     r5, r5, r4, lsr #16
    mov     r4, r4, lsl #16
    orr     r4, r4, r3, lsr #16
    stmdb   r0!, {r4-r11}
    subs    lr, lr, #1
    bne     copy_back_nonaligned_2
copy_back_smaller_2:
    mov     r2, r2, lsl #26
    msr     cpsr_f, r2
    moveq   r7, r3, lsl #16
    ldmeqdb r1!, {r3-r6}
    orreq   r7, r7, r6, lsr #16
    moveq   r6, r6, lsl #16
    orreq   r6, r6, r5, lsr #16
    moveq   r5, r5, lsl #16
    orreq   r5, r5, r4, lsr #16
    moveq   r4, r4, lsl #16
    orreq   r4, r4, r3, lsr #16
    stmeqdb r0!, {r4-r7}
    movcs   r5, r3, lsl #16
    ldmcsdb r1!, {r3,r4}
    orrcs   r5, r5, r4, lsr #16
    movcs   r4, r4, lsl #16
    orrcs   r4, r4, r3, lsr #16
    stmcsdb r0!, {r4,r5}
    movvs   r4, r3, lsl #16
    ldmvsdb r1!, {r3}
    orrvs   r4, r4, r3, lsr #16
    stmvsdb r0!, {r4}
    b       copy_back_remainder
start_copy_back_nonaligned_3:
    movs    lr, r2, lsr #5 @;lr=number of 32-byte blocks
    beq     copy_back_smaller_3
copy_back_nonaligned_3:
    mov     r11, r3, lsl #8
    ldmdb   r1!, {r3-r10}
    orr     r11, r11, r10, lsr #24
    mov     r10, r10, lsl #8
    orr     r10, r10, r9, lsr #24
    mov     r9, r9, lsl #8
    orr     r9, r9, r8, lsr #24
    mov     r8, r8, lsl #8
    orr     r8, r8, r7, lsr #24
    mov     r7, r7, lsl #8
    orr     r7, r7, r6, lsr #24
    mov     r6, r6, lsl #8
    orr     r6, r6, r5, lsr #24
    mov     r5, r5, lsl #8
    orr     r5, r5, r4, lsr #24
    mov     r4, r4, lsl #8
    orr     r4, r4, r3, lsr #24
    stmdb   r0!, {r4-r11}
    subs    lr, lr, #1
    bne     copy_back_nonaligned_3
copy_back_smaller_3:
    mov     r2, r2, lsl #26
    msr     cpsr_f, r2
    moveq   r7, r3, lsl #8
    ldmeqdb r1!, {r3-r6}
    orreq   r7, r7, r6, lsr #24
    moveq   r6, r6, lsl #8
    orreq   r6, r6, r5, lsr #24
    moveq   r5, r5, lsl #8
    orreq   r5, r5, r4, lsr #24
    moveq   r4, r4, lsl #8
    orreq   r4, r4, r3, lsr #24
    stmeqdb r0!, {r4-r7}
    movcs   r5, r3, lsl #8
    ldmcsdb r1!, {r3,r4}
    orrcs   r5, r5, r4, lsr #24
    movcs   r4, r4, lsl #8
    orrcs   r4, r4, r3, lsr #24
    stmcsdb r0!, {r4,r5}
    movvs   r4, r3, lsl #8
    ldmvsdb r1!, {r3}
    orrvs   r4, r4, r3, lsr #24
    stmvsdb r0!, {r4}

copy_back_remainder:
    mov     r2, r2, lsl #2
    msr     cpsr_f, r2
    add     r1, r1, r12     @; true source address (nonaligned)
    ldrcsb  lr, [r1, #-1]!          @; do byte copies to finish
    strcsb  lr, [r0, #-1]!
    ldrcsb  lr, [r1, #-1]!
    strcsb  lr, [r0, #-1]!
    ldrvsb  lr, [r1, #-1]!
    strvsb  lr, [r0, #-1]!
    ldmfd   sp!, {r0, r4-r11, pc} @;__POPRET("r0,r4-r11,")
    

@; We are now word aligned
 
same_alignment_copy_fore:
mem_move_fore:
    movs    ip, r2, lsr #8  @; Get number of 256 blocks to copy
    beq     its_smaller_fore @; Less than 256 bytes to copy

copy_256_bytes_fore:
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    subs    ip, ip, #1
    bne     copy_256_bytes_fore
    ands    r2, r2, #255
    beq     mem_copy_end

@; Less than 256 bytes to go...

its_smaller_fore:

    movs    r2, r2, asl #24
    msr     cpsr_f, r2

    ldmmiia r1!, {r3-r10}       @; Copy 128 bytes
    stmmiia r0!, {r3-r10}
    ldmmiia r1!, {r3-r10}
    stmmiia r0!, {r3-r10}
    ldmmiia r1!, {r3-r10}
    stmmiia r0!, {r3-r10}
    ldmmiia r1!, {r3-r10}
    stmmiia r0!, {r3-r10}

    ldmeqia r1!, {r3-r10}       @; Copy 64
    stmeqia r0!, {r3-r10} 
    ldmeqia r1!, {r3-r10}
    stmeqia r0!, {r3-r10} 

    ldmcsia r1!, {r3-r10}       @; Copy 32
    stmcsia r0!, {r3-r10} 

    ldmvsia r1!, {r3-r6}        @; Copy 16
    stmvsia r0!, {r3-r6} 

@; Check if more to go
 
    movs    r2, r2, asl #4
    beq     mem_copy_end

@; Finish off. less than 16 bytes to go...

    msr     cpsr_f, r2
    ldmmiia r1!, {r3,r4}
    stmmiia r0!, {r3,r4}
    ldmeqia r1!, {r3}
    stmeqia r0!, {r3}
    ldrcsb  r3, [r1], #1
    strcsb  r3, [r0], #1
    ldrcsb  r3, [r1], #1
    strcsb  r3, [r0], #1
    ldrvsb  r3, [r1], #1
    strvsb  r3, [r0], #1

@; All done!

mem_copy_end:
    ldmfd   sp!, {r4-r11, pc} @;__POPRET("r4-r11,");

@; Backwards copying, addresses have same alignment
 
same_alignment_copy_back:
mem_move_back:

@; We are now word aligned. Do a fast copy until less than 4 bytes

    movs    ip, r2, lsr #8      @; get number of 256 blocks to copy
    beq     its_smaller_back

copy_256_bytes_back:
    ldmdb   r1!, {r3-r10}       @; Copy 256 bytes
    stmdb   r0!, {r3-r10}
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10} 
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10} 
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10}
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10}
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10}
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10}
    ldmdb   r1!, {r3-r10}
    stmdb   r0!, {r3-r10}
    subs    ip, ip, #1
    bne     copy_256_bytes_back

    ands    r2, r2, #255
    beq     mem_copy_end2

its_smaller_back:
    movs    r2, r2, asl #24
    msr     cpsr_f, r2
    ldmmidb r1!, {r3-r10}   @; Copy 128 bytes
    stmmidb r0!, {r3-r10}
    ldmmidb r1!, {r3-r10}
    stmmidb r0!, {r3-r10}
    ldmmidb r1!, {r3-r10}
    stmmidb r0!, {r3-r10}
    ldmmidb r1!, {r3-r10}
    stmmidb r0!, {r3-r10}

    ldmeqdb r1!, {r3-r10}       @; Copy 64
    stmeqdb r0!, {r3-r10} 
    ldmeqdb r1!, {r3-r10}
    stmeqdb r0!, {r3-r10} 

    ldmcsdb r1!, {r3-r10}       @; Copy 32
    stmcsdb r0!, {r3-r10}

    ldmvsdb r1!, {r3-r6}        @; Copy 16
    stmvsdb r0!, {r3-r6}

@; Check if more to go
 
    movs    r2, r2, asl #4
    beq     mem_copy_end2

@; Finish off. less than 16 bytes to go

    msr     cpsr_f, r2
    ldmmidb r1!, {r3,r4}        @; copy 8
    stmmidb r0!, {r3,r4}

    ldreq   r3, [r1, #-4]!  @; copy 4
    streq   r3, [r0, #-4]! 

    ldrcsb  r3, [r1, #-1]!      @; copy 2
    strcsb  r3, [r0, #-1]!
    ldrcsb  r3, [r1, #-1]!
    strcsb  r3, [r0, #-1]!

    ldrvsb  r3, [r1, #-1]!  @; copy 1
    strvsb  r3, [r0, #-1]! 

@; All done (hopefully :-)

mem_copy_end2:
    ldmfd   sp!, {r0,r4-r11,pc} @;__POPRET("r0,r4-r11,");


